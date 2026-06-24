#include "Physics.h"
#include <unordered_map>
#include <vector>
#include "Util.h"

#include <iostream> // TODO: cleanup logging

namespace Physics {

    PxShape* CreateConvexShapeFromVertexList(std::span<Vertex>& vertices) {
        PxPhysics* pxPhysics = GetPxPhysics();
        PxMaterial* defaultMaterial = GetDefaultMaterial();

        std::vector<PxVec3> pxVertices;
        for (Vertex& vertex : vertices) {
            pxVertices.push_back(Physics::GlmVec3toPxVec3(vertex.position));
        }

        PxConvexMeshDesc convexDesc;
        convexDesc.points.count = pxVertices.size();
        convexDesc.points.stride = sizeof(PxVec3);
        convexDesc.points.data = pxVertices.data();
        convexDesc.flags = PxConvexFlag::eSHIFT_VERTICES | PxConvexFlag::eCOMPUTE_CONVEX;
        //  s
        PxTolerancesScale scale;
        PxCookingParams params(scale);

        PxDefaultMemoryOutputStream buf;
        PxConvexMeshCookingResult::Enum result;
        if (!PxCookConvexMesh(params, convexDesc, buf, &result)) {
            std::cout << "some convex mesh shit failed\n";
            return 0;
        }
        PxDefaultMemoryInputData input(buf.getData(), buf.getSize());
        PxConvexMesh* convexMesh = pxPhysics->createConvexMesh(input);
        PxConvexMeshGeometryFlags flags(~PxConvexMeshGeometryFlag::eTIGHT_BOUNDS);
        PxConvexMeshGeometry geometry(convexMesh, PxMeshScale(PxVec3(1.0f)), flags);

        PxShape* pxShape = pxPhysics->createShape(geometry, *defaultMaterial);
        return pxShape;
    }

    PxShape* CreateBoxShape(float width, float height, float depth, Transform shapeOffset, PxMaterial* material) {
        if (material == NULL) {
            material = Physics::GetDefaultMaterial();
        }
        PxShape* shape = Physics::GetPxPhysics()->createShape(PxBoxGeometry(width, height, depth), *material, true);
        PxMat44 localShapeMatrix = GlmMat4ToPxMat44(shapeOffset.to_mat4());
        PxTransform localShapeTransform(localShapeMatrix);
        shape->setLocalPose(localShapeTransform);
        return shape;
    }

    PxRigidDynamic* CreateRigidDynamic(Transform worldTransform, PhysicsFilterData physicsFilterData, PxShape* shape, Transform shapeOffset) {
        PxQuat quat = GlmQuatToPxQuat(glm::quat(worldTransform.rotation));
        PxTransform pxTransform = PxTransform(PxVec3(worldTransform.position.x, worldTransform.position.y, worldTransform.position.z), quat);
        PxRigidDynamic* body = Physics::GetPxPhysics()->createRigidDynamic(pxTransform);

        // You are passing in a PxShape pointer and any shape offset will affects that actually object, wherever the fuck it is up the function chain.
        // Maybe look into this when you can be fucked, possibly you can just set the isExclusive bool to true, where and whenever the fuck that is and happens.
        PxFilterData filterData;
        filterData.word0 = (PxU32)physicsFilterData.raycastGroup;
        filterData.word1 = (PxU32)physicsFilterData.collisionGroup;
        filterData.word2 = (PxU32)physicsFilterData.collidesWith;
        shape->setQueryFilterData(filterData);       // ray casts
        shape->setSimulationFilterData(filterData);  // collisions
        PxMat44 localShapeMatrix = GlmMat4ToPxMat44(shapeOffset.to_mat4());
        PxTransform localShapeTransform(localShapeMatrix);
        shape->setLocalPose(localShapeTransform);

        body->attachShape(*shape);
        PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);
        Physics::GetPxScene()->addActor(*body);
        return body;
    }

    PxRigidDynamic* CreateRigidDynamic(PxShape* shape, glm::mat4 worldMatrix, glm::mat4 shapeOffsetMatrix, PhysicsFilterData filterData) {
        // Warning, you haven't vetted if this world matrix stuff is identical to the transform version above!!!
        // Warning, you haven't vetted if this world matrix stuff is identical to the transform version above!!!
        // Warning, you haven't vetted if this world matrix stuff is identical to the transform version above!!!

        PxTransform pxTransfrom = PxTransform(GlmMat4ToPxMat44(worldMatrix));
        PxRigidDynamic* body = Physics::GetPxPhysics()->createRigidDynamic(pxTransfrom);

        // You are passing in a PxShape pointer and any shape offset will affects that actually object, wherever the fuck it is up the function chain.
        // Maybe look into this when you can be fucked, possibly you can just set the isExclusive bool to true, where and whenever the fuck that is and happens.
        PxFilterData pxFilterData;
        pxFilterData.word0 = (PxU32)filterData.raycastGroup;
        pxFilterData.word1 = (PxU32)filterData.collisionGroup;
        pxFilterData.word2 = (PxU32)filterData.collidesWith;
        shape->setQueryFilterData(pxFilterData);       // ray casts
        shape->setSimulationFilterData(pxFilterData);  // collisions
        PxMat44 localShapeMatrix = GlmMat4ToPxMat44(shapeOffsetMatrix);
        PxTransform localShapeTransform(localShapeMatrix);
        shape->setLocalPose(localShapeTransform);

        body->attachShape(*shape);
        PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);
        Physics::GetPxScene()->addActor(*body);
        return body;
    }

    void Physics::Destroy(PxRigidDynamic*& rigidDynamic) {
        if (rigidDynamic) {
            if (rigidDynamic->userData) {
                //delete static_cast<PhysicsObjectData*>(rigidDynamic->userData);
                rigidDynamic->userData = nullptr;
            }
            Physics::GetPxScene()->removeActor(*rigidDynamic);
            rigidDynamic->release();
            rigidDynamic = nullptr;
        }
    }

    void Physics::Destroy(PxRigidStatic*& rigidStatic) {
        if (rigidStatic) {
            if (rigidStatic->userData) {
                //delete static_cast<PhysicsObjectData*>(rigidStatic->userData);
                rigidStatic->userData = nullptr;
            }
            Physics::GetPxScene()->removeActor(*rigidStatic);
            rigidStatic->release();
            rigidStatic = nullptr;
        }
    }

    void Physics::Destroy(PxShape*& shape) {
        if (shape) {
            if (shape->userData) {
                //delete static_cast<PhysicsObjectData*>(shape->userData);
                shape->userData = nullptr;
            }
            shape->release();
            shape = nullptr;
        }
    }

    void Physics::Destroy(PxRigidBody*& rigidBody) {
        if (rigidBody) {
            if (rigidBody->userData) {
                //delete static_cast<PhysicsObjectData*>(rigidBody->userData);
                rigidBody->userData = nullptr;
            }
            Physics::GetPxScene()->removeActor(*rigidBody);
            rigidBody->release();
            rigidBody = nullptr;
        }
    }

    void Physics::Destroy(PxTriangleMesh*& triangleMesh) {
        if (triangleMesh) {
            triangleMesh = nullptr;
        }
    }
}

