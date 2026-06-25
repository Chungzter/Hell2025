#pragma once
#include "Hell/Physics/Physics.h"

#include "RagdollTypes.h"

namespace RagdollUtil {

    inline PxCombineMode::Enum ToPxCombineMode(RdCombineMode mode) {
        return mode == RdCombineMode::kAverage  ? PxCombineMode::eAVERAGE :
               mode == RdCombineMode::kMin      ? PxCombineMode::eMIN :
               mode == RdCombineMode::kMultiply ? PxCombineMode::eMULTIPLY :
                                                   PxCombineMode::eMAX;
    }

    inline float GetPhysicsSceneScale(const RagdollSolver& solver) {
        return solver.applySceneScaleToPhysics ? static_cast<float>(solver.sceneScale) : 1.0f;
    }

    inline RdVector toPhysicalScale(const RdVector value, RdScalar scale) {
        return value * scale;
    }

    inline RdQuaternion toQuaternion(const RdEulerRotation& euler) {
        // XYZ rotation order
        return RdQuaternion::rotation((RdRadians)euler.z(), RdVector::zAxis()) *
               RdQuaternion::rotation((RdRadians)euler.y(), RdVector::yAxis()) *
               RdQuaternion::rotation((RdRadians)euler.x(), RdVector::xAxis());
    }

    inline PxQuat toPxQuat(RdEulerRotation euler) {
        const RdQuaternion quat = toQuaternion(euler);
        const RdVector vec = quat.vector();

        return PxQuat(
            static_cast<float>(vec.x()),
            static_cast<float>(vec.y()),
            static_cast<float>(vec.z()),
            static_cast<float>(quat.scalar())
        );
    }

    inline PxVec3 toPxVec3(RdPoint point) {
        return PxVec3(
            static_cast<float>(point.x()),
            static_cast<float>(point.y()),
            static_cast<float>(point.z())
        );
    }

    inline PxShape* CreateConvexShape(RagdollMarker& marker, PxMaterial& material, float sceneScale) {
        PxPhysics* pxPhysics = Hell::Physics::GetPxPhysics();
        const RdGeometryDescriptionComponent& desc = marker.geometryDescriptionComponent;

        std::vector<PxVec3> convexVerts;
        convexVerts.reserve(marker.convexMeshVertices.size());
        for (const RdPoint& point : marker.convexMeshVertices) {
            convexVerts.push_back(toPxVec3(point));
        }

        if (convexVerts.size() < 3) {
            return nullptr;
        }

        PxConvexMeshDesc convexDesc;
        convexDesc.points.count = static_cast<PxU32>(convexVerts.size());
        convexDesc.points.stride = sizeof(PxVec3);
        convexDesc.points.data = convexVerts.data();
        convexDesc.vertexLimit = desc.vertexLimit;
        convexDesc.flags = PxConvexFlag::eCOMPUTE_CONVEX | PxConvexFlag::eSHIFT_VERTICES;

        if (desc.quantisedCount > 3) {
            convexDesc.quantizedCount = static_cast<short>(desc.quantisedCount);
            convexDesc.flags |= PxConvexFlag::eQUANTIZE_INPUT;
        }

        if (desc.checkZeroAreaTriangles) {
            convexDesc.flags |= PxConvexFlag::eCHECK_ZERO_AREA_TRIANGLES;
        }

        PxCookingParams params{ pxPhysics->getTolerancesScale() };
        params.areaTestEpsilon = 0.00001f;

        PxConvexMeshCookingResult::Enum result;
        PxConvexMesh* convexMesh = PxCreateConvexMesh(
            params,
            convexDesc,
            pxPhysics->getPhysicsInsertionCallback(),
            &result
        );

        if (!convexMesh) {
            return nullptr;
        }

        if (result != PxConvexMeshCookingResult::eSUCCESS &&
            result != PxConvexMeshCookingResult::ePOLYGONS_LIMIT_REACHED) {
            convexMesh->release();
            return nullptr;
        }

        PxConvexMeshGeometry geometry{ convexMesh, PxMeshScale(sceneScale) };
        if (!geometry.isValid()) {
            convexMesh->release();
            return nullptr;
        }

        return pxPhysics->createShape(geometry, material, true);
    }

    inline PxShape* CreateShape(RagdollMarker& marker, RagdollSolver& solver) {
        PxPhysics* pxPhysics = Hell::Physics::GetPxPhysics();
        PxShape* pxShape = nullptr;

        PxMaterial* material = pxPhysics->createMaterial(
            static_cast<float>(marker.friction),
            static_cast<float>(marker.friction),
            static_cast<float>(marker.restitution)
        );
        material->setFrictionCombineMode(ToPxCombineMode(marker.frictionCombineMode));
        if (marker.contactStiffness > 0) {
            const float stiffness = static_cast<float>(marker.contactStiffness * solver.contactStiffness);
            const float damping = static_cast<float>(marker.contactStiffness * marker.contactDampingRatio * solver.contactDamping);
            material->setRestitutionCombineMode(ToPxCombineMode(marker.contactCombineMode));
            material->setFlag(PxMaterialFlag::eCOMPLIANT_CONTACT, marker.contactAccelerationSpring);
            material->setRestitution(-stiffness);
            material->setDamping(damping);
        }

        RdGeometryDescriptionComponent desc = marker.geometryDescriptionComponent;
        RdScaleComponent scale = marker.scaleComponent;
        const float sceneScale = GetPhysicsSceneScale(solver);

        if (sceneScale != 1.0f) {
            desc.length *= sceneScale;
            desc.radius *= sceneScale;
            desc.radiusEnd *= sceneScale;
            desc.extents *= sceneScale;
        }

        // Box
        if (desc.type == RdGeometryType::kBox) {
            const auto geometry = PxBoxGeometry{
                std::max(0.001f, static_cast<float>(desc.extents.x() * scale.absolute.x()) * 0.5f),
                std::max(0.001f, static_cast<float>(desc.extents.y() * scale.absolute.y()) * 0.5f),
                std::max(0.001f, static_cast<float>(desc.extents.z() * scale.absolute.z()) * 0.5f)
            };
            pxShape = pxPhysics->createShape(geometry, *material);
        }

        // Sphere
        if (desc.type == RdGeometryType::kSphere) {
            const double radiusScale = scale.absolute.x();
            const auto geometry = PxSphereGeometry{ static_cast<float>(desc.radius * radiusScale) };
            pxShape = pxPhysics->createShape(geometry, *material);
        }

        // Capsule
        if (desc.type == RdGeometryType::kCapsule) {
            RdVector absScale = scale.absolute;

            if (desc.capsuleLengthAlongY) {
                absScale.z() = absScale.x();
                absScale.x() = absScale.y();
                absScale.y() = absScale.z();
            }

            const float halfHeight = desc.length * 0.5f * static_cast<float>(absScale.x());
            const float radius = desc.radius * static_cast<float>(absScale.y());

            PxCapsuleGeometry geometry{ radius, halfHeight };
            pxShape = pxPhysics->createShape(geometry, *material);
        }

        // Convex Mesh
        else if (desc.type == RdGeometryType::kConvexHull) {
            pxShape = CreateConvexShape(marker, *material, sceneScale);
        }

        if (pxShape && desc.type != RdGeometryType::kConvexHull) {
            const RdVector localOffset = desc.offset * scale.value;
            const PxTransform pxtm{ toPxVec3(toPhysicalScale(localOffset, sceneScale)), toPxQuat(desc.rotation) };
            pxShape->setLocalPose(pxShape->getLocalPose().transform(pxtm));
        }
        if (pxShape) {
            const PxTolerancesScale tolerancesScale = pxPhysics->getTolerancesScale();
            float contactOffset = static_cast<float>(marker.thickness) * tolerancesScale.length;
            if (marker.useSceneScaleForThickness && sceneScale != 0.0f) {
                contactOffset /= sceneScale;
            }
            pxShape->setContactOffset(contactOffset);
        }
        return pxShape;
    }

}
