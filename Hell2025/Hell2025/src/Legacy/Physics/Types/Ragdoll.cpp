#include "Ragdoll.h"
#include "Physics/Physics.h"
#include "Util.h"
#include <iostream>

#include "Renderer/Renderer.h"
void Ragdoll::SetFilterData(PhysicsFilterData physicsFilterData) {
    for (int i = 0; i < m_correspondingBoneNames.size(); i++) {
        RigidDynamic* rigidDynamic = Physics::GetRigidDynamicById(m_rigidDynamicIds[i]);

        if (rigidDynamic) {
            PxRigidDynamic* pxRigidDynamic = rigidDynamic->GetPxRigidDynamic();

            if (pxRigidDynamic) {
                rigidDynamic->SetFilterData(physicsFilterData);
            }
        }
    }
}

void Ragdoll::SetPhysicsData(uint64_t objectId) {
    for (int i = 0; i < m_rigidDynamicIds.size(); i++) {
        RigidDynamic* rigidDynamic = Physics::GetRigidDynamicById(m_rigidDynamicIds[i]);
        if (rigidDynamic) {
            PxRigidDynamic* pxRigidDynamic = rigidDynamic->GetPxRigidDynamic();
            if (pxRigidDynamic) {
                PhysicsUserData physicsUserData;
                physicsUserData.objectId = objectId;
                physicsUserData.physicsId = m_rigidDynamicIds[i];
                physicsUserData.physicsType = PhysicsType::RIGID_DYNAMIC;
                pxRigidDynamic->userData = new PhysicsUserData(physicsUserData);
            }
        }
    }
}

void Ragdoll::ActivatePhysics() {
    for (int i = 0; i < m_correspondingBoneNames.size(); i++) {
        RigidDynamic* rigidDynamic = Physics::GetRigidDynamicById(m_rigidDynamicIds[i]);
        if (rigidDynamic) {
            PxRigidDynamic* pxRigidDynamic = rigidDynamic->GetPxRigidDynamic();
            if (pxRigidDynamic) {
                pxRigidDynamic->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, false);
                pxRigidDynamic->wakeUp();
            }
        }
    }
}

void Ragdoll::PrintCorrespondingBoneNames() {
    std::cout << "Ragdoll '" << m_name << "' corresponding bone names:\n";
    for (int i = 0; i < m_correspondingBoneNames.size(); i++) {
        const std::string& correspondingBoneName = m_correspondingBoneNames[i];
        std::cout << "-" << i << " " << correspondingBoneName << "\n";
    }
}

void Ragdoll::SetRigidGlobalPosesFromAnimatedGameObject(AnimatedGameObject* animatedGameObject) {
    if (!animatedGameObject) {
        std::cout << "Ragdoll::SetRigidGlobalPosesFromAnimatedGameObject() failed because AnimatedGameObject was nullptr\n";
        return;
    }

    for (int i = 0; i < m_correspondingBoneNames.size(); i++) {
        const std::string& correspondingBoneName = m_correspondingBoneNames[i];

        for (const auto& entry : animatedGameObject->GetSkinnedModel()->m_boneMapping) {
            const std::string& boneName = entry.first;
            unsigned int boneIndex = entry.second;

            if (correspondingBoneName == boneName) {
                RigidDynamic* rididDynamic = Physics::GetRigidDynamicById(m_rigidDynamicIds[i]);

                if (rididDynamic) {
                    PxRigidDynamic* pxRigidDynamic = rididDynamic->GetPxRigidDynamic();

                    if (pxRigidDynamic) {
                        glm::mat4 objectMatrixWorld = animatedGameObject->GetModelMatrix();
                        glm::mat4 boneMatrixLocal = animatedGameObject->GetAnimatedTransformByBoneName(boneName);
                        glm::mat4 boneMatrixWorld = objectMatrixWorld * boneMatrixLocal;

                        PxTransform pxTransform = PxTransform(Physics::GlmMat4ToPxMat44(boneMatrixWorld));
                        pxRigidDynamic->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
                        pxRigidDynamic->setKinematicTarget(pxTransform);

                        //PxShape* shape;
                        //pxRigidDynamic->getShapes(&shape, 1);
                        //
                        //// 2. Perform a raw intersection test (bypasses ALL filtering and scene trees)
                        //PxRaycastHit hitInfo;
                        //
                        //PxVec3 origin = PxVec3(43.3178, 32.641, 36.5277) ;
                        //PxVec3 dir = PxVec3(0.961002, -0.00999986, 0.27636);
                        //float maxDist = 10;
                        //
                        //PxGeometryHolder holder = shape->getGeometry();
                        //
                        //// This is the correct way to get the base reference
                        //const PxGeometry& geometry = holder.any();
                        //
                        //bool physicalHit = PxGeometryQuery::raycast(
                        //    origin, dir,
                        //    geometry,
                        //    pxRigidDynamic->getGlobalPose() * shape->getLocalPose(),
                        //    maxDist,
                        //    PxHitFlag::eDEFAULT,
                        //    1, &hitInfo
                        //);
                        //
                        //
                        //if (Input::KeyPressed(HELL_KEY_H)) {
                        //    if (physicalHit) {
                        //        std::cout << "hit for " << correspondingBoneName << "\n";
                        //    }
                        //    else {
                        //        std::cout << "no hit for " << correspondingBoneName << "\n";
                        //    }
                        //}

                        break;
                    }
                    else {
                        std::cout << "failed to get pxRigidDynamic for id " << m_rigidDynamicIds[i] << "\n";
                    }
                }
            }
        }
    }
}

glm::mat4 Ragdoll::GetRigidWorlTransform(const std::string& correspondingBoneName) {
    for (int i = 0; i < m_correspondingBoneNames.size(); i++) {

        if (m_correspondingBoneNames[i] == correspondingBoneName) {
            RigidDynamic* rigidDynamic = Physics::GetRigidDynamicById(m_rigidDynamicIds[i]);

            if (rigidDynamic) {
                PxRigidDynamic* pxRigidDynamic = rigidDynamic->GetPxRigidDynamic();

                if (pxRigidDynamic) {
                    return Physics::PxMat44ToGlmMat4(pxRigidDynamic->getGlobalPose());
                }
            }
        }
    }

    static glm::mat4 identity(1.0f);
    return identity;
}

void Ragdoll::MarkForRemoval() {
    m_markedForRemoval = true;
}