#include "Ragdoll.h"

#include "RagdollUtil.h"

#include "Hell/Logging.h"
#include "Hell/Physics/PhysicsIds.h"
#include "Hell/ResourceManagement/ResourceManager.h"

#include <cmath>
#include <iostream>

inline PxTransform PxTransformFromRest(const RdMatrix& restM, float sceneScale) {
    PxMat44 M = RdMatrixToPxMat44(restM);

    // Extract basis columns + translation from PxMat44
    PxVec3 x(M.column0.x, M.column0.y, M.column0.z);
    PxVec3 y(M.column1.x, M.column1.y, M.column1.z);
    PxVec3 z(M.column2.x, M.column2.y, M.column2.z);
    PxVec3 t(M.column3.x, M.column3.y, M.column3.z);

    // Descaling / orthonormalization
    x = x.getNormalized();
    y = (y - x * x.dot(y)).getNormalized();
    z = x.cross(y);

    // Ensure right handed basis (no mirroring)
    if (x.cross(y).dot(z) < 0.0f) z = -z;

    // Build rotation from the orthonormal columns
    PxMat33 R(x, y, z);          // PxMat33 takes columns
    PxQuat  q(R);                // quaternion from 3x3

    // Scale TRANSLATION only
    PxVec3 p = t * sceneScale;

    return PxTransform(p, q);
}

inline PxU32 GetSolverIterationCount(RdUint solverIterations, RdUint rigidIterations) {
    return static_cast<PxU32>(std::min(255U, solverIterations * rigidIterations));
}

inline PxU32 GetSelfCollisionFilterWord(uint64_t ragdollId, const RagdollMarker& marker) {
    if (marker.resolvedCollisionGroup < 256 || marker.groupIndex < 0) {
        return 0;
    }

    const PxU32 ragdollBits = static_cast<PxU32>(ragdollId & 0x0000ffff);
    const PxU32 groupBits = static_cast<PxU32>((marker.groupIndex + 1) & 0xff);
    return RAGDOLL_SELF_COLLISION_FILTER_TAG | (ragdollBits << 8) | groupBits;
}

inline float ScaleJointSpring(float value) {
    const float maxValue = std::numeric_limits<float>::max();
    return value < maxValue / 1000.0f ? value * 1000.0f : value;
}

void Ragdoll::Init(const glm::vec3& spawnPosition, const glm::vec3& spawnEulerRotation, const std::string& ragdollName, uint64_t ragdollId, uint64_t parentObjectId, PhysicsFilterData filterData) {
    RagdollData* ragdollData = Hell::ResourceManager::GetRagdollDataByName(ragdollName);
    if (!ragdollData) return;

    RagdollSolver& solver = ragdollData->m_solver;

    m_ragdollId = ragdollId;
    m_scale = RagdollUtil::GetPhysicsSceneScale(solver);
    m_ragdollName = ragdollName;
    m_spawnTransform.position = spawnPosition;
    m_spawnTransform.rotation = spawnEulerRotation;
    m_markedForRemoval = false;

    CleanUp();

    PxTransform rootPose(Hell::Physics::GlmMat4ToPxMat44(m_spawnTransform.to_mat4()));

    PxPhysics* physics = Hell::Physics::GetPxPhysics();
    PxScene* scene = Hell::Physics::GetPxScene();

    for (RagdollMarker& marker : ragdollData->m_markers) {
        // Store color for rendering
        glm::vec3 color = glm::vec3(marker.color.x(), marker.color.g(), marker.color.b());
        m_markerColors.push_back(color);

        // Store bone name for skinning
        m_markerBoneNames.push_back(marker.boneName);

        // Create mesh for rendering from shape
        AddMarkerMeshData(marker, ragdollData->m_solver);

        PxTransform restTransform = PxTransformFromRest(marker.originMatrix, m_scale);
        PxRigidDynamic* pxrigid = physics->createRigidDynamic(rootPose.transform(restTransform));

        // Kinematic/dynamic/inherit
        const bool kinematic = (marker.resolvedInputType == (RdEnum)RdBehaviour::kKinematic);

        PxShape* shape = RagdollUtil::CreateShape(marker, ragdollData->m_solver);

        if (shape) {
            PxFilterData pxFilterData;
            pxFilterData.word0 = (PxU32)filterData.raycastGroup;
            pxFilterData.word1 = (PxU32)filterData.collisionGroup;
            pxFilterData.word2 = (PxU32)filterData.collidesWith;
            pxFilterData.word3 = GetSelfCollisionFilterWord(m_ragdollId, marker);
            shape->setQueryFilterData(pxFilterData);       // ray casts
            shape->setSimulationFilterData(pxFilterData);  // collisions

            pxrigid->attachShape(*shape);
            shape->release();

            // Mass/inertia
            if (marker.densityCustom <= 0.0f) {
                PxRigidBodyExt::setMassAndUpdateInertia(*pxrigid, PxReal(marker.mass));
            }
            else {
                PxRigidBodyExt::updateMassAndInertia(*pxrigid, PxReal(marker.densityCustom));
            }

            if (marker.enableCCD) {
                pxrigid->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, true);
            }

            pxrigid->setLinearDamping((float)marker.linearDamping);
            pxrigid->setAngularDamping((float)marker.angularDamping);
            pxrigid->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_GYROSCOPIC_FORCES, true);
            pxrigid->setSleepThreshold((float)marker.sleepThreshold);
            pxrigid->setStabilizationThreshold(0.01f);
            pxrigid->setSolverIterationCounts(
                GetSolverIterationCount(solver.positionIterations, marker.positionIterations),
                GetSolverIterationCount(solver.velocityIterations, marker.velocityIterations)
            );

            float wakeCounter = std::numeric_limits<float>::max();
            if (marker.wakeCounter > 1) {
                wakeCounter = (1.0f / 24.0f) * static_cast<float>(solver.timeMultiplier) * static_cast<float>(marker.wakeCounter - 1);
            }
            pxrigid->setWakeCounter(wakeCounter);
            pxrigid->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, kinematic);

            if (marker.maxContactImpulse > 0) {
                pxrigid->setMaxContactImpulse(marker.maxContactImpulse);
            }
            else {
                pxrigid->setMaxContactImpulse(PX_MAX_F32);
            }

            if (marker.maxDepenetrationVelocity > 0) {
                pxrigid->setMaxDepenetrationVelocity(marker.maxDepenetrationVelocity);
            }
            else {
                pxrigid->setMaxDepenetrationVelocity(PX_MAX_F32);
            }

            scene->addActor(*pxrigid);
            m_pxRigidDynamics.emplace_back(pxrigid);

            // User data
            PhysicsUserData physicsUserData;
            physicsUserData.physicsType = PhysicsType::RIGID_DYNAMIC;
            //physicsUserData.objectType = ObjectType::RAGDOLL_V2;
            physicsUserData.physicsId = Hell::Physics::CreatePhysicsId(Hell::Physics::PhysicsObjectType::RAGDOLL);
            physicsUserData.objectId = parentObjectId;
            pxrigid->userData = new PhysicsUserData(physicsUserData);
        }
    }

    std::unordered_map<std::string, PxRigidDynamic*> actorByMarker;
    for (int i = 0; i < ragdollData->m_markers.size(); i++) {
        actorByMarker[ragdollData->m_markers[i].name] = m_pxRigidDynamics[i];
    }

    const float sceneScale = RagdollUtil::GetPhysicsSceneScale(ragdollData->m_solver);

    #define LOCK_NEGATIVE_LINEAR 1

    auto setLinearAxis = [&](PxD6Joint* d6, PxD6Axis::Enum axis, float lim, const PxSpring& linearSpring) {
        #if LOCK_NEGATIVE_LINEAR
        if (lim > 0.0f) {
            d6->setMotion(axis, PxD6Motion::eLIMITED);
            d6->setLinearLimit(axis, PxJointLinearLimitPair(-lim, lim, linearSpring));
        }
        else if (lim < 0.0f) {
            d6->setMotion(axis, PxD6Motion::eLOCKED);
        }
        #else
        if (lim > 0.0f) {
            d6->setMotion(axis, PxD6Motion::eLIMITED);
            d6->setLinearLimit(axis, PxJointLinearLimitPair(-lim, lim, linearSpring));
        }
        else if (lim == 0.0f) {
            d6->setMotion(axis, PxD6Motion::eLOCKED);
        }
        else {
            d6->setMotion(axis, PxD6Motion::eFREE);
        }
        #endif
    };

    for (RagdollJoint& j : ragdollData->m_joints)
    {
        auto itP = actorByMarker.find(j.parentName);
        auto itC = actorByMarker.find(j.childName);
        if (itP == actorByMarker.end() || itC == actorByMarker.end()) {
            Logging::Warning() << "[D6] Missing actors for joint " << j.name;
            continue;
        }

        PxRigidActor* parent = itP->second;
        PxRigidActor* child = itC->second;

        // parent/child local frames from JSON, scale translation only
        PxTransform lp(Hell::Physics::GlmMat4ToPxMat44(RdMatrixToGlmMat4(j.parentFrame)));
        PxTransform lc(Hell::Physics::GlmMat4ToPxMat44(RdMatrixToGlmMat4(j.childFrame)));
        if (sceneScale != 1.0f) { lp.p *= sceneScale; lc.p *= sceneScale; }

        PxD6Joint* d6 = PxD6JointCreate(*physics, parent, lp, child, lc);
        if (!d6) { Logging::Error() << "[D6] Create failed for " << j.name; continue; }

        // Start FREE on all axes
        d6->setMotion(PxD6Axis::eX, PxD6Motion::eFREE);
        d6->setMotion(PxD6Axis::eY, PxD6Motion::eFREE);
        d6->setMotion(PxD6Axis::eZ, PxD6Motion::eFREE);
        d6->setMotion(PxD6Axis::eTWIST, PxD6Motion::eFREE);
        d6->setMotion(PxD6Axis::eSWING1, PxD6Motion::eFREE);
        d6->setMotion(PxD6Axis::eSWING2, PxD6Motion::eFREE);

        if (j.limitEnabled) {
            const PxSpring linearSpring(
                ScaleJointSpring(static_cast<float>(j.limitLinearStiffness)),
                ScaleJointSpring(static_cast<float>(j.limitLinearDamping))
            );
            const PxSpring angularSpring(
                ScaleJointSpring(static_cast<float>(j.limitAngularStiffness)),
                ScaleJointSpring(static_cast<float>(j.limitAngularDamping))
            );

            // Linear limits
            setLinearAxis(d6, PxD6Axis::eX, (float)j.limitLinear.x(), linearSpring);
            setLinearAxis(d6, PxD6Axis::eY, (float)j.limitLinear.y(), linearSpring);
            setLinearAxis(d6, PxD6Axis::eZ, (float)j.limitLinear.z(), linearSpring);

            // Angular limits
            const float twist = (float)j.limitRange.x(); // radians
            const float swing1 = (float)j.limitRange.y();
            const float swing2 = (float)j.limitRange.z();

            if (twist > 0.0f) {
                d6->setMotion(PxD6Axis::eTWIST, PxD6Motion::eLIMITED);
                d6->setTwistLimit(PxJointAngularLimitPair(-twist, twist, angularSpring));
            }
            else if (twist < 0.0f) {
                d6->setMotion(PxD6Axis::eTWIST, PxD6Motion::eLOCKED);
            }

            if (swing1 > 0.0f && swing2 > 0.0f) {
                d6->setMotion(PxD6Axis::eSWING1, PxD6Motion::eLIMITED);
                d6->setMotion(PxD6Axis::eSWING2, PxD6Motion::eLIMITED);
                d6->setSwingLimit(PxJointLimitCone(swing1, swing2, angularSpring));
            }
            else {
                if (swing1 < 0.0f) d6->setMotion(PxD6Axis::eSWING1, PxD6Motion::eLOCKED);
                if (swing2 < 0.0f) d6->setMotion(PxD6Axis::eSWING2, PxD6Motion::eLOCKED);
                // don't set swing limit
            }
        }

        if (!j.disableCollision) {
            d6->setConstraintFlag(PxConstraintFlag::eCOLLISION_ENABLED, true);
        }

        m_pxD6Joints.push_back(d6);
    }

    DisableSimulation();
    RecalculateRigidMass();
}

void Ragdoll::Update() {
    //for (PxRigidDynamic* pxRigidDynamic : m_pxRigidDynamics) {
    //    PxTransform pxTransform = pxRigidDynamic->getGlobalPose();
    //    PxMat44 pxMatrix(pxTransform);
    //    glm::mat4 matrix = Hell::Physics::PxMat44ToGlmMat4(pxMatrix);
    //}
}

void Ragdoll::RecalculateRigidMass() {
    RagdollData* ragdollData = Hell::ResourceManager::GetRagdollDataByName(m_ragdollName);
    if (!ragdollData) return;

    const size_t count = std::min(m_pxRigidDynamics.size(), ragdollData->m_markers.size());
    for (size_t i = 0; i < count; ++i) {
        const RagdollMarker& marker = ragdollData->m_markers[i];
        if (marker.geometryDescriptionComponent.type == RdGeometryType::kConvexHull) {
            continue;
        }

        PxRigidDynamic* pxRigidDynamic = m_pxRigidDynamics[i];
        if (!pxRigidDynamic) continue;

        const PxU32 shapeCount = pxRigidDynamic->getNbShapes();
        if (shapeCount == 0) continue;

        std::vector<PxShape*> shapes(shapeCount);
        pxRigidDynamic->getShapes(shapes.data(), shapeCount);

        float volume = 0.0f;
        for (PxShape* shape : shapes) {
            volume += Hell::Physics::ComputeShapeVolume(shape);
        }
        if (volume <= 0.0f) continue;

        const float density = marker.densityCustom > 0.0f ? static_cast<float>(marker.densityCustom) : 1.0f;
        const float mass = volume * density;
        PxRigidBodyExt::setMassAndUpdateInertia(*pxRigidDynamic, PxReal(mass));
    }
}

void Ragdoll::MarkForRemoval() {
    m_markedForRemoval = true;
}

bool Ragdoll::IsMarkedForRemoval() const {
    return m_markedForRemoval;
}

void Ragdoll::AddForce(const std::string& boneName, const glm::vec3& force, bool wakeIfDisabled) {
    const size_t count = std::min(m_markerBoneNames.size(), m_pxRigidDynamics.size());

    for (size_t i = 0; i < count; i++) {
        if (m_markerBoneNames[i] != boneName) continue;

        PxRigidDynamic* pxRigidDynamic = m_pxRigidDynamics[i];
        if (!pxRigidDynamic) return;

        if (!m_simulationEnabled) {
            if (!wakeIfDisabled) return;
            EnableSimulation();
        }

        pxRigidDynamic->addForce(PxVec3(force.x, force.y, force.z), PxForceMode::eVELOCITY_CHANGE, true);
        return;
    }
}

void Ragdoll::SetAngularVelocity(const std::string& boneName, const glm::vec3& angularVelocity, bool wakeIfDisabled) {
    const size_t count = std::min(m_markerBoneNames.size(), m_pxRigidDynamics.size());

    for (size_t i = 0; i < count; i++) {
        if (m_markerBoneNames[i] != boneName) continue;

        PxRigidDynamic* pxRigidDynamic = m_pxRigidDynamics[i];
        if (!pxRigidDynamic) return;

        if (!m_simulationEnabled) {
            if (!wakeIfDisabled) return;
            EnableSimulation();
        }

        pxRigidDynamic->setAngularVelocity(PxVec3(angularVelocity.x, angularVelocity.y, angularVelocity.z), true);
        return;
    }
}

void Ragdoll::AddForce(uint64_t physicsId, const glm::vec3& force, bool wakeIfDisabled) {
    for (PxRigidDynamic* pxRigidDynamic : m_pxRigidDynamics) {
        if (!pxRigidDynamic) continue;

        PhysicsUserData* physicsUserData = static_cast<PhysicsUserData*>(pxRigidDynamic->userData);
        if (!physicsUserData) continue;

        if (physicsUserData->physicsId == physicsId) {
            if (!m_simulationEnabled) {
                if (!wakeIfDisabled) return;
                EnableSimulation();
            }

            pxRigidDynamic->addForce(PxVec3(force.x, force.y, force.z), PxForceMode::eVELOCITY_CHANGE, true);

            return;
        }
    }
}

void Ragdoll::AddImpulse(uint64_t physicsId, const glm::vec3& impulse, bool wakeIfDisabled) {
    for (PxRigidDynamic* pxRigidDynamic : m_pxRigidDynamics) {
        if (!pxRigidDynamic) continue;

        PhysicsUserData* physicsUserData = static_cast<PhysicsUserData*>(pxRigidDynamic->userData);
        if (!physicsUserData) continue;

        if (physicsUserData->physicsId == physicsId) {
            if (!m_simulationEnabled) {
                if (!wakeIfDisabled) return;
                EnableSimulation();
            }

            pxRigidDynamic->addForce(PxVec3(impulse.x, impulse.y, impulse.z), PxForceMode::eIMPULSE, true);

            return;
        }
    }
}

void Ragdoll::AddImpulseAtPosition(uint64_t physicsId, const glm::vec3& impulse, const glm::vec3& position, bool wakeIfDisabled) {
    for (PxRigidDynamic* pxRigidDynamic : m_pxRigidDynamics) {
        if (!pxRigidDynamic) continue;

        PhysicsUserData* physicsUserData = static_cast<PhysicsUserData*>(pxRigidDynamic->userData);
        if (!physicsUserData) continue;

        if (physicsUserData->physicsId == physicsId) {
            if (!m_simulationEnabled) {
                if (!wakeIfDisabled) return;
                EnableSimulation();
            }

            PxVec3 pxImpulse(impulse.x, impulse.y, impulse.z);
            PxVec3 pxPosition(position.x, position.y, position.z);
            PxRigidBodyExt::addForceAtPos(*pxRigidDynamic, pxImpulse, pxPosition, PxForceMode::eIMPULSE, true);

            return;
        }
    }
}

void Ragdoll::AddAngularVelocityChangeAtPosition(uint64_t physicsId, const glm::vec3& velocityChange, const glm::vec3& position, bool wakeIfDisabled) {
    for (PxRigidDynamic* pxRigidDynamic : m_pxRigidDynamics) {
        if (!pxRigidDynamic) continue;

        PhysicsUserData* physicsUserData = static_cast<PhysicsUserData*>(pxRigidDynamic->userData);
        if (!physicsUserData) continue;

        if (physicsUserData->physicsId == physicsId) {
            if (!m_simulationEnabled) {
                if (!wakeIfDisabled) return;
                EnableSimulation();
            }

            PxVec3 pxVelocityChange(velocityChange.x, velocityChange.y, velocityChange.z);
            PxVec3 pxPosition(position.x, position.y, position.z);
            PxVec3 centerOfMass = pxRigidDynamic->getGlobalPose().transform(pxRigidDynamic->getCMassLocalPose().p);
            PxVec3 angularVelocityChange = (pxPosition - centerOfMass).cross(pxVelocityChange);
            pxRigidDynamic->addTorque(angularVelocityChange, PxForceMode::eVELOCITY_CHANGE, true);

            return;
        }
    }
}

void Ragdoll::DisableSimulation() {
    for (PxRigidDynamic* pxRigidDynamic : m_pxRigidDynamics) {
        if (!pxRigidDynamic) continue;
        pxRigidDynamic->setLinearVelocity(PxVec3(0.0f));
        pxRigidDynamic->setAngularVelocity(PxVec3(0.0f));
        pxRigidDynamic->setActorFlag(PxActorFlag::eDISABLE_SIMULATION, true);
    }
    m_simulationEnabled = false;
}

void Ragdoll::EnableSimulation() {
    for (PxRigidDynamic* pxRigidDynamic : m_pxRigidDynamics) {
        if (!pxRigidDynamic) continue;

        pxRigidDynamic->setActorFlag(PxActorFlag::eDISABLE_SIMULATION, false);
        pxRigidDynamic->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, false);
        pxRigidDynamic->wakeUp();
    }
    m_simulationEnabled = true;
}

void Ragdoll::SetSpawnPosition(const glm::vec3& position) {
    m_spawnTransform.position = position;
    SetToInitialPose();
    UpdateWorldSpaceAABBs(0.0f);
}

void Ragdoll::SetSpawnRotation(const glm::vec3& rotation) {
    m_spawnTransform.rotation = rotation;
    SetToInitialPose();
    UpdateWorldSpaceAABBs(0.0f);
}

void Ragdoll::SetToInitialPose() {
    RagdollData* ragdollData = Hell::ResourceManager::GetRagdollDataByName(m_ragdollName);
    if (!ragdollData) return;

    const PxTransform spawnTransform(Hell::Physics::GlmMat4ToPxMat44(m_spawnTransform.to_mat4()));
    PxTransform rootPose(Hell::Physics::GlmMat4ToPxMat44(m_spawnTransform.to_mat4()));

    const size_t count = std::min(m_pxRigidDynamics.size(), ragdollData->m_markers.size());
    for (size_t i = 0; i < count; ++i) {
        const RagdollMarker& marker = ragdollData->m_markers[i];
        PxTransform restTransform = PxTransformFromRest(marker.originMatrix, m_scale);
        m_pxRigidDynamics[i]->setGlobalPose(rootPose.transform(restTransform));
        m_pxRigidDynamics[i]->setLinearVelocity(PxVec3(0));
        m_pxRigidDynamics[i]->setAngularVelocity(PxVec3(0));
    }
}

void Ragdoll::CleanUp() {
    if (Hell::MeshBuffer* meshBuffer = Hell::ResourceManager::GetMeshBufferPtr("PhysicsDebugGeometry")) {
        for (uint32_t meshId : m_markerDebugMeshIds) {
            if (meshId != 0) {
                meshBuffer->RemoveMesh(meshId);
            }
        }
    }
    m_markerDebugMeshIds.clear();
    m_markerColors.clear();
    m_markerBoneNames.clear();

    for (PxD6Joint* pxD6Joint : m_pxD6Joints) {
        if (pxD6Joint) {
            pxD6Joint->release();
        }
    }
    m_pxD6Joints.clear();

    for (PxRigidDynamic*& pxRigidDynamic : m_pxRigidDynamics) {
        if (pxRigidDynamic) {
            if (pxRigidDynamic->userData) {
                delete static_cast<PhysicsUserData*>(pxRigidDynamic->userData);
                pxRigidDynamic->userData = nullptr;
            }

            if (pxRigidDynamic->getScene()) {
                pxRigidDynamic->getScene()->removeActor(*pxRigidDynamic);
            }

            pxRigidDynamic->release();
            pxRigidDynamic = nullptr;
        }
    }
    m_pxRigidDynamics.clear();
}

bool Ragdoll::IsInMotion() {
    const float linearThreshold = 0.01f;
    const float angularThreshold = 0.01f;
    const float linearThresholdSq = linearThreshold * linearThreshold;
    const float angularThresholdSq = angularThreshold * angularThreshold;

    for (PxRigidDynamic* pxRigidDynamic : m_pxRigidDynamics) {
        if (!pxRigidDynamic) continue;
        if (pxRigidDynamic->getRigidBodyFlags().isSet(PxRigidBodyFlag::eKINEMATIC)) continue;
        if (pxRigidDynamic->isSleeping()) continue;

        const PxVec3 v = pxRigidDynamic->getLinearVelocity();
        const PxVec3 w = pxRigidDynamic->getAngularVelocity();
        if (v.magnitudeSquared() > linearThresholdSq || w.magnitudeSquared() > angularThresholdSq) {
            return true;
        }
    }
    return false;
}

const std::string& Ragdoll::GetBoneNameByPhysicsId(uint64_t physicsId) const {
    static const std::string empty = "";

    const size_t count = std::min(m_pxRigidDynamics.size(), m_markerBoneNames.size());
    for (size_t i = 0; i < count; i++) {
        PxRigidDynamic* pxRigidDynamic = m_pxRigidDynamics[i];
        if (!pxRigidDynamic) continue;

        PhysicsUserData* physicsUserData = static_cast<PhysicsUserData*>(pxRigidDynamic->userData);
        if (physicsUserData && physicsUserData->physicsId == physicsId) {
            return m_markerBoneNames[i];
        }
    }

    return empty;
}

AABB Ragdoll::GetWorldSpaceAABB() {
    glm::vec3 min = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 max = glm::vec3(-std::numeric_limits<float>::max());

    for (PxRigidDynamic* pxRigidDynamic : m_pxRigidDynamics) {
        if (pxRigidDynamic) {
            const PxU32 nbShapes = pxRigidDynamic->getNbShapes();
            if (!nbShapes) continue;
            std::vector<PxShape*> shapes(nbShapes);
            pxRigidDynamic->getShapes(shapes.data(), nbShapes);

            const PxTransform pose = pxRigidDynamic->getGlobalPose();
            for (PxShape * s : shapes) {
                const PxBounds3 b = PxShapeExt::getWorldBounds(*s, *pxRigidDynamic, 1.0f);
                glm::vec3 bmin(b.minimum.x, b.minimum.y, b.minimum.z);
                glm::vec3 bmax(b.maximum.x, b.maximum.y, b.maximum.z);
                min = glm::min(min, bmin);
                max = glm::max(max, bmax);
            }
        }
    }

    return AABB(min, max);
}

void Ragdoll::GetWorldSpaceAABBs(std::vector<AABB>& aabbs) {
    aabbs.clear();

    for (PxRigidDynamic* pxRigidDynamic : m_pxRigidDynamics) {
        if (!pxRigidDynamic) {
            continue;
        }

        const PxU32 nbShapes = pxRigidDynamic->getNbShapes();
        if (!nbShapes) {
            continue;
        }

        glm::vec3 min = glm::vec3(std::numeric_limits<float>::max());
        glm::vec3 max = glm::vec3(-std::numeric_limits<float>::max());

        std::vector<PxShape*> shapes(nbShapes);
        pxRigidDynamic->getShapes(shapes.data(), nbShapes);

        for (PxShape* shape : shapes) {
            const PxBounds3 bounds = PxShapeExt::getWorldBounds(*shape, *pxRigidDynamic, 1.0f);
            const glm::vec3 boundsMin(bounds.minimum.x, bounds.minimum.y, bounds.minimum.z);
            const glm::vec3 boundsMax(bounds.maximum.x, bounds.maximum.y, bounds.maximum.z);

            min = glm::min(min, boundsMin);
            max = glm::max(max, boundsMax);
        }

        aabbs.push_back(AABB(min, max));
    }
}

void Ragdoll::UpdateWorldSpaceAABBs(float changeThreshold) {

    std::vector<AABB> worldspaceAABBsPreviousFrame = m_worldSpaceAABBs;

    GetWorldSpaceAABBs(m_worldSpaceAABBs);

    if (m_worldSpaceAABBs.size() != worldspaceAABBsPreviousFrame.size()) {
        m_dirty = true;
    }
    else {
        m_dirty = false;

        for (size_t i = 0; i < m_worldSpaceAABBs.size(); i++) {
            const AABB& aabb = m_worldSpaceAABBs[i];
            const AABB& aabbLastFrame = worldspaceAABBsPreviousFrame[i];

            const glm::vec3& boundsMin = aabb.GetBoundsMin();
            const glm::vec3& boundsMax = aabb.GetBoundsMax();
            const glm::vec3& boundsMinLastFrame = aabbLastFrame.GetBoundsMin();
            const glm::vec3& boundsMaxLastFrame = aabbLastFrame.GetBoundsMax();

            if (std::abs(boundsMin.x - boundsMinLastFrame.x) > changeThreshold ||
                std::abs(boundsMin.y - boundsMinLastFrame.y) > changeThreshold ||
                std::abs(boundsMin.z - boundsMinLastFrame.z) > changeThreshold ||
                std::abs(boundsMax.x - boundsMaxLastFrame.x) > changeThreshold ||
                std::abs(boundsMax.y - boundsMaxLastFrame.y) > changeThreshold ||
                std::abs(boundsMax.z - boundsMaxLastFrame.z) > changeThreshold) {
                m_dirty = true;
                return;
            }
        }
    }
}

glm::vec3 Ragdoll::GetMarkerColorByRigidIndex(uint32_t index) const {
    if (index >= m_markerColors.size()) {
        Logging::Error() << "Ragdoll::GetMarkerColorByRigidIndex() failed, index " << index << " out of range of size " << m_pxRigidDynamics.size();
        return glm::vec3(1.0f);
    }
    return m_markerColors[index];
}

uint32_t Ragdoll::GetMarkerDebugMeshIdByRigidIndex(uint32_t index) const {
    if (index >= m_markerDebugMeshIds.size()) {
        Logging::Error() << "Ragdoll::GetMarkerDebugMeshIdByRigidIndex() failed, index " << index << " out of range of size " << m_pxRigidDynamics.size();
        return 0;
    }
    return m_markerDebugMeshIds[index];
}

glm::mat4 Ragdoll::GetModelMatrixByRigidIndex(uint32_t index) const {
    if (index >= m_pxRigidDynamics.size()) {
        Logging::Error() << "Ragdoll::GetModelMatrixByRigidIndex() failed, index " << index << " out of range of size " << m_pxRigidDynamics.size();
        return glm::mat4(1.0f);
    }

    Hell::Transform scaleTransform;
    scaleTransform.scale = glm::vec3(m_scale);
    return Hell::Physics::PxMat44ToGlmMat4(m_pxRigidDynamics[index]->getGlobalPose()) * scaleTransform.to_mat4();
}

glm::mat4 Ragdoll::GetRigidWorldTransform(const std::string& boneName) const {
    const size_t count = std::min(m_pxRigidDynamics.size(), m_markerBoneNames.size());
    for (size_t i = 0; i < count; i++) {
        if (m_markerBoneNames[i] != boneName) continue;

        PxRigidDynamic* pxRigidDynamic = m_pxRigidDynamics[i];
        if (pxRigidDynamic) {
            return Hell::Physics::PxMat44ToGlmMat4(pxRigidDynamic->getGlobalPose());
        }
    }

    return glm::mat4(1.0f);
}

void Ragdoll::AddMarkerMeshData(RagdollMarker& marker, RagdollSolver& solver) {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    vertices.reserve(marker.convexMeshVertices.size());
    indices.reserve(marker.convexMeshIndices.size());

    const RdGeometryDescriptionComponent& desc = marker.geometryDescriptionComponent;

    // Apply local shape transfrm
    auto applyLocalShapeXform = [&](size_t begin) {
        glm::mat4 R(1.0f);
        R = glm::rotate(R, (float)desc.rotation.z(), glm::vec3(0, 0, 1));
        R = glm::rotate(R, (float)desc.rotation.y(), glm::vec3(0, 1, 0));
        R = glm::rotate(R, (float)desc.rotation.x(), glm::vec3(1, 0, 0));

        const float s = RagdollUtil::GetPhysicsSceneScale(solver);
        glm::vec3 T((float)desc.offset.x(),
                    (float)desc.offset.y(),
                    (float)desc.offset.z());
        T /= s;

        for (size_t i = begin; i < vertices.size(); ++i) {
            glm::vec3 p = vertices[i].position;
            p = glm::vec3(R * glm::vec4(p, 1.0f));
            vertices[i].position = p + T;
        }
    };

    if (desc.type == RdGeometryType::kConvexHull) {
        for (RdPoint& point : marker.convexMeshVertices) {
            Vertex& vertex = vertices.emplace_back();
            vertex.position.x = point.x();
            vertex.position.y = point.y();
            vertex.position.z = point.z();
        }
        for (RdUint& index : marker.convexMeshIndices) {
            indices.emplace_back(index);
        }
    }
    else if (desc.type == RdGeometryType::kBox) {
        const float hx = (float)desc.extents.x() * 0.5f;
        const float hy = (float)desc.extents.y() * 0.5f;
        const float hz = (float)desc.extents.z() * 0.5f;

        const size_t vbase = vertices.size();

        auto addFace = [&](glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3) {
            const uint32_t base = (uint32_t)vertices.size();
            Vertex v0; Vertex v1; Vertex v2; Vertex v3;
            const glm::vec3 invS = glm::vec3(1.0f / RagdollUtil::GetPhysicsSceneScale(solver));
            v0.position = p0 * invS; v1.position = p1 * invS; v2.position = p2 * invS; v3.position = p3 * invS;
            v0.uv = { 0,0 }; v1.uv = { 1,0 }; v2.uv = { 1,1 }; v3.uv = { 0,1 };
            vertices.push_back(v0); vertices.push_back(v1); vertices.push_back(v2); vertices.push_back(v3);
            indices.push_back(base + 0); indices.push_back(base + 1); indices.push_back(base + 2);
            indices.push_back(base + 0); indices.push_back(base + 2); indices.push_back(base + 3);
        };

        const glm::vec3 v000(-hx, -hy, -hz), v001(-hx, -hy, +hz), v010(-hx, +hy, -hz), v011(-hx, +hy, +hz);
        const glm::vec3 v100(+hx, -hy, -hz), v101(+hx, -hy, +hz), v110(+hx, +hy, -hz), v111(+hx, +hy, +hz);

        addFace(v100, v110, v111, v101); // +X
        addFace(v001, v011, v010, v000); // -X
        addFace(v010, v011, v111, v110); // +Y
        addFace(v100, v101, v001, v000); // -Y
        addFace(v101, v111, v011, v001); // +Z
        addFace(v000, v010, v110, v100); // -Z

        applyLocalShapeXform(vbase);
    }

    else if (desc.type == RdGeometryType::kCapsule) {
        const float sceneScale = RagdollUtil::GetPhysicsSceneScale(solver);
        const float r = (float)desc.radius / sceneScale;
        const float half = 0.5f * (float)desc.length / sceneScale;
        const unsigned int hemisphereRings = 12;
        const unsigned int cylinderRings = 1;
        const unsigned int segments = 24;

        const size_t vbase = vertices.size();

        const double PI = 3.14159265358979323846;
        const double TAU = 6.28318530717958647692;
        const double PI_HALF = 1.57079632679489661923;

        const double hemisphereRingAngleIncrement = PI_HALF / (double)hemisphereRings;

        auto append = [&](glm::dvec3 p, glm::dvec3 n) {
            glm::vec3 pos = glm::vec3((float)p.x, (float)(p.y * r), (float)(p.z * r));
            float u = (float)(std::atan2(p.z, p.y) / TAU);
            if (u < 0.0f) u += 1.0f;
            float v = (float)((p.x + (half + r)) / (2.0f * (half + r)));
            Vertex vtx;
            vtx.position = pos;
            vtx.normal = glm::normalize(glm::vec3((float)n.x, (float)n.y, (float)n.z));
            vtx.uv = glm::vec2(u, v);
            vertices.push_back(vtx);
        };

        auto capVertex = [&](double x, double normalX) {
            append({ x, 0.0, 0.0 }, { normalX, 0.0, 0.0 });
        };

        auto hemisphereVertexRings = [&](unsigned int count, double centerX, double startRingAngle, double ringAngleIncrement) {
            const double segInc = TAU / (double)segments;
            for (unsigned int i = 0; i != count; ++i) {
                const double a = startRingAngle + (double)i * ringAngleIncrement;
                const double s = std::sin(a);
                const double c = std::cos(a);
                for (unsigned int j = 0; j != segments; ++j) {
                    const double b = (double)j * segInc;
                    const double sb = std::sin(b), cb = std::cos(b);
                    append({ centerX + s * (double)r, c * sb, c * cb },
                           { s,                        c * sb, c * cb });
                }
            }
        };

        auto cylinderVertexRings = [&](const unsigned int count, const double startX, const glm::dvec2& inc) {
            const glm::dvec2 baseNormal = { inc.y, -inc.x };
            glm::dvec2 base = { 1.0, startX };
            const double segInc = TAU / (double)segments;
            for (unsigned int i = 0; i != count; ++i) {
                for (unsigned int j = 0; j != segments; ++j) {
                    const double b = (double)j * segInc;
                    const double sb = std::sin(b), cb = std::cos(b);
                    append({ base.y, base.x * sb, base.x * cb },
                           { baseNormal.y, baseNormal.x * sb, baseNormal.x * cb });
                }
                base.x += inc.x;
                base.y += inc.y;
            }
        };

        auto bottomFaceRing = [&]() {
            const unsigned int tip = (unsigned int)vbase;
            for (unsigned int j = 0; j != segments; ++j) {
                unsigned int topRight = (j != segments - 1) ? vbase + 1 + j + 1 : vbase + 1;
                unsigned int bottom = tip;
                unsigned int topLeft = vbase + 1 + j;
                indices.push_back(topRight);
                indices.push_back(bottom);
                indices.push_back(topLeft);
            }
        };

        auto faceRings = [&](unsigned int count, unsigned int offset) {
            const unsigned int vertexSegments = segments;
            for (unsigned int i = 0; i != count; ++i) {
                for (unsigned int j = 0; j != segments; ++j) {
                    const unsigned int bottomLeft = (unsigned int)vbase + offset + i * vertexSegments + j;
                    const unsigned int bottomRight = (j != segments - 1)
                        ? (unsigned int)vbase + offset + i * vertexSegments + j + 1
                        : (unsigned int)vbase + offset + i * vertexSegments;
                    const unsigned int topLeft = bottomLeft + vertexSegments;
                    const unsigned int topRight = bottomRight + vertexSegments;
                    indices.push_back(bottomRight);
                    indices.push_back(bottomLeft);
                    indices.push_back(topRight);
                    indices.push_back(topRight);
                    indices.push_back(bottomLeft);
                    indices.push_back(topLeft);
                }
            }
        };

        auto topFaceRing = [&]() {
            const unsigned int tip = (unsigned int)vertices.size() - 1;
            const unsigned int ringStart = tip - segments;
            for (unsigned int j = 0; j != segments; ++j) {
                unsigned int topRight = (j != segments - 1) ? ringStart + j + 1 : ringStart;
                unsigned int bottom = tip;
                unsigned int topLeft = ringStart + j;
                indices.push_back(topLeft);
                indices.push_back(bottom);
                indices.push_back(topRight);
            }
        };

        capVertex(-(double)(half + r), -1.0);
        hemisphereVertexRings(hemisphereRings - 1, -(double)half, -PI_HALF + hemisphereRingAngleIncrement, hemisphereRingAngleIncrement);
        cylinderVertexRings(cylinderRings + 1, -(double)half, glm::dvec2{ 0.0, 2.0 * (double)half / (double)cylinderRings });
        hemisphereVertexRings(hemisphereRings - 1, +(double)half, hemisphereRingAngleIncrement, hemisphereRingAngleIncrement);
        capVertex(+(double)(half + r), 1.0);

        bottomFaceRing();
        faceRings(hemisphereRings * 2 - 2 + cylinderRings, 1);
        topFaceRing();

        applyLocalShapeXform(vbase);
    }

    else if (desc.type == RdGeometryType::kSphere) {
        const float r = (float)desc.radius / RagdollUtil::GetPhysicsSceneScale(solver);
        const int lat = 16, lon = 24;

        const size_t vbase = vertices.size();

        for (int y = 0; y <= lat; ++y) {
            float v = (float)y / (float)lat;
            float a1 = v * 3.14159265359f;
            float sy = std::cos(a1);
            float sr = std::sin(a1);

            for (int x = 0; x <= lon; ++x) {
                float u = (float)x / (float)lon;
                float a2 = u * 6.28318530718f;
                float cx = std::cos(a2);
                float sx = std::sin(a2);

                Vertex vert;
                vert.position = { r * sr * cx, r * sy, r * sr * sx };
                vert.uv = { u, v };
                vertices.push_back(vert);
            }
        }

        auto idx = [&](int x, int y) { return (uint32_t)(vbase + y * (lon + 1) + x); };

        for (int y = 0; y < lat; ++y) {
            for (int x = 0; x < lon; ++x) {
                uint32_t a = idx(x, y);
                uint32_t b = idx(x + 1, y);
                uint32_t c = idx(x + 1, y + 1);
                uint32_t d = idx(x, y + 1);
                indices.push_back(a); indices.push_back(b); indices.push_back(c);
                indices.push_back(a); indices.push_back(c); indices.push_back(d);
            }
        }

        applyLocalShapeXform(vbase);
    }

    // Generate normals/tangents
    for (int i = 0; i < indices.size(); i += 3) {
        Vertex* vert0 = &vertices[indices[i]];
        Vertex* vert1 = &vertices[indices[i + 1]];
        Vertex* vert2 = &vertices[indices[i + 2]];

        glm::vec3 deltaPos1 = vert1->position - vert0->position;
        glm::vec3 deltaPos2 = vert2->position - vert0->position;
        glm::vec2 deltaUV1 = vert1->uv - vert0->uv;
        glm::vec2 deltaUV2 = vert2->uv - vert0->uv;

        float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);

        glm::vec3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
        vert0->tangent = tangent;
        vert1->tangent = tangent;
        vert2->tangent = tangent;

        glm::vec3 normal = glm::normalize(glm::cross(deltaPos1, deltaPos2));
        vert0->normal = normal;
        vert1->normal = normal;
        vert2->normal = normal;
    }

    uint32_t meshId = 0;
    if (Hell::MeshBuffer* meshBuffer = Hell::ResourceManager::GetMeshBufferPtr("PhysicsDebugGeometry")) {
        meshId = meshBuffer->AddMesh(vertices, indices, marker.name);
    }
    m_markerDebugMeshIds.push_back(meshId);
    //Logging::Debug() << "Added " << marker.shapeType << " vertex data: " << marker.name << " " << vertices.size() << " verts " << indices.size() << " indices";
}

