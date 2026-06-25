#include "Physics.h"
#include "Editor/Editor.h"
#include "Hell/Time.h"

#include <unordered_map>
#include <vector>

namespace Hell::Physics {
    namespace {
        void StepPhysics(float deltaTime);
        void UpdateHeightFields();
        void UpdateActiveRigidDynamicAABBList();
        void UpdateAllRigidDynamics(float deltaTime);
    }

    void BeginFrame() {
        RemoveAnyHeightFieldMarkedForRemoval();
        RemoveAnyRagdollsMarkedForRemoval();
        RemoveAnyD6JointMarkedForRemoval();
        RemoveAnyRigidDynamicMarkedForRemoval();
        RemoveAnyRigidStaticMarkedForRemoval();
        RemoveAnyCharacterControllerMarkedForRemoval();
    }

    void StepSimulation() {
        while (Hell::Time::ConsumeFixedStep()) {
            if (Editor::IsClosed()) {
                StepPhysics(Hell::Time::FixedDeltaTime());
            }
        }
    }

    void ForceZeroStepUpdate() {
        PxScene* pxScene = GetPxScene();
        pxScene->simulate(0.0f, nullptr);
        //pxScene->flushQueryUpdates();
        pxScene->fetchResults(true);
    }

    void SyncRuntimeState() {
        UpdateAllRigidDynamics(Hell::Time::DeltaTime());
        UpdateActiveRigidDynamicAABBList();
        UpdateHeightFields();
    }

    namespace {
    void StepPhysics(float deltaTime) {
        ClearCollisionReports();

        PxScene* pxScene = GetPxScene();
        pxScene->simulate(deltaTime);
        pxScene->fetchResults(true);
        //pxScene->sceneQueriesUpdate();
        //pxScene->fetchSceneQueries(true);
    }

    void UpdateHeightFields() {
        std::vector<HeightField>& heightFields = GetHeightFields();

        for (HeightField& heightfield : heightFields) {
            const AABB& heightFieldAABB = heightfield.GetAABB();

            bool intersectionFound = false;
            float threshold = 0.25f;

            // Enable heightfield physics if other active PhysX object AABBs intersect heightfield AABB
            if (Editor::IsClosed()) {

                // Character controllers
                const std::unordered_map<uint64_t, CharacterController>& characterControllers = GetCharacterControllers();
                for (auto it = characterControllers.begin(); it != characterControllers.end(); ) {
                    const CharacterController& characterController = it->second;

                    const AABB characterControllerAABB = characterController.GetAABB();
                    if (heightFieldAABB.IntersectsAABB(characterControllerAABB, threshold)) {
                        intersectionFound = true;
                        break;
                    }
                    it++;
                }

                // Active rigid dynamics
                if (!intersectionFound) {
                    const std::vector<AABB>& activeRigidAABBS = GetActiveRididDynamicAABBs();
                    for (const AABB& aabb : activeRigidAABBS) {

                        //DebugDraw::DrawAABB(aabb, GREEN);

                        if (heightFieldAABB.IntersectsAABB(aabb, threshold)) {
                            intersectionFound = true;
                            break;
                        }
                    }
                }
            }
            // Activate physics so ray casts work in the sector editor
            if (Editor::IsOpen() && Editor::GetEditorMode() == EditorMode::MAP_OBJECT_EDITOR) {
                heightfield.ActivatePhsyics();
            }
            // Regular check
            else if (intersectionFound) {
                heightfield.ActivatePhsyics();
            }
            else {
                heightfield.DisablePhsyics();
            }
        }
    }

    void UpdateActiveRigidDynamicAABBList() {
        std::vector<AABB>& activeRigidDynamicAABBs = GetActiveRididDynamicAABBs();
        std::unordered_map<uint64_t, RigidDynamic>& rigidDynamics = GetRigidDynamics();

        activeRigidDynamicAABBs.clear();
        for (auto it = rigidDynamics.begin(); it != rigidDynamics.end(); ) {
            RigidDynamic& rigidDynamic = it->second;
            PxRigidDynamic* pxRigidDynamic = rigidDynamic.GetPxRigidDynamic();
            if (pxRigidDynamic && !pxRigidDynamic->getRigidBodyFlags().isSet(PxRigidBodyFlag::eKINEMATIC)) {
                activeRigidDynamicAABBs.push_back(rigidDynamic.GetAABB());
            }
            it++;
        }
    }

    void UpdateAllRigidDynamics(float deltaTime) {
        std::unordered_map<uint64_t, RigidDynamic>& rigidDynamics = GetRigidDynamics();

        for (auto it = rigidDynamics.begin(); it != rigidDynamics.end(); ++it) {
            RigidDynamic& rigidDynamic = it->second;
            rigidDynamic.Update(deltaTime);

            PxRigidDynamic* pxRigidDynamic = rigidDynamic.GetPxRigidDynamic();
            if (!pxRigidDynamic) {
                continue;
            }

            //const bool awake = pxRigidDynamic->isSleeping() == false;
            //const glm::vec4 color = awake ? RED : GREEN;
            //const PxBounds3 bounds = pxRigidDynamic->getWorldBounds();
            //const glm::vec3 aabbMin = Hell::Physics::PxVec3toGlmVec3(bounds.minimum);
            //const glm::vec3 aabbMax = Hell::Physics::PxVec3toGlmVec3(bounds.maximum);
            //AABB aabb(aabbMin, aabbMax);
            //DebugDraw::DrawAABB(aabb, color);
            //
            //if (Hell::Physics::RigidDynamicIsKinematic(it->first)) {
            //    DebugDraw::DrawPoint(aabb.GetCenter(), YELLOW);
            //}
        }
    }
}
}
