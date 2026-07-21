#include "FeatureTest.h"

#include "Hell/Input.h"
#include "Hell/Physics.h"

#include "Unloved/ObjectId.h"
#include "Unloved/Editor/Editor.h"
#include "Unloved/Objects/Lighting/Light.h"
#include "Unloved/Objects/Renderables/AnimatedGameObject.h"
#include "Unloved/World/World.h"

namespace Unloved::FeatureTest {

    static uint64_t ratKingAnimatedObjectId = 0;
    static uint64_t trapKingAnimatedObjectId = 0;
    static uint64_t manikinRagdollId0 = 0;
    static uint64_t manikinRagdollId1 = 0;

    void UpdateManikinTest();
    void UpdateRagdollTest();
    void UpdateRatKing();
    void UpdateTrapKing();

    void Update() {
        UpdateManikinTest();
        UpdateRagdollTest();
        UpdateRatKing();
        // UpdateTrapKing();
    }

    void CleanUp() {
        World::RemoveObjectById(ratKingAnimatedObjectId);
        ratKingAnimatedObjectId = 0;

        World::RemoveObjectById(trapKingAnimatedObjectId);
        trapKingAnimatedObjectId = 0;

        Hell::Physics::MarkRagdollForRemoval(manikinRagdollId0);
        manikinRagdollId0 = 0;

        Hell::Physics::MarkRagdollForRemoval(manikinRagdollId1);
        manikinRagdollId1 = 0;
    }

    void UpdateManikinTest() {
        if (manikinRagdollId0 == 0) {
            PhysicsFilterData filterData;
            filterData.raycastGroup = RaycastGroup::RAYCAST_ENABLED;
            filterData.collisionGroup = CollisionGroup::RAGDOLL_ENEMY;
            filterData.collidesWith = CollisionGroup(ENVIROMENT_OBSTACLE | CHARACTER_CONTROLLER | RAGDOLL_ENEMY);

            manikinRagdollId0 = Hell::Physics::SpawnRagdoll(glm::vec3(36, 31, 36), glm::vec3(0.0f,  0.2f, 0.0f), "manikin", GetNextObjectId(ObjectType::RAGDOLL_STANDALONE), filterData);
            manikinRagdollId1 = Hell::Physics::SpawnRagdoll(glm::vec3(37, 31, 36), glm::vec3(0.0f, -0.4f, 0.0f), "manikin", GetNextObjectId(ObjectType::RAGDOLL_STANDALONE), filterData);
        }
    }

    void UpdateRagdollTest() {
        auto& ragdolls = Hell::Physics::GetRagdolls();
        for (auto it = ragdolls.begin(); it != ragdolls.end(); ) {
            Ragdoll& ragdoll = it->second;

            // Disable ragdoll simulation  and reset to initial pose
            if (Hell::Input::KeyPressed(HELL_KEY_Y)) {
                ragdoll.SetToInitialPose();
                ragdoll.DisableSimulation();

                for (Light& light : Unloved::World::GetLights()) {
                    AABB aabb = ragdoll.GetWorldSpaceAABB();
                    if (aabb.IntersectsSphere(light.GetPosition(), light.GetRadius())) {
                        light.ForceDirty();
                    }
                }
            }

            // Enable ragdoll simulation and reset to initial pose
            if (Hell::Input::KeyPressed(HELL_KEY_O)) {
                ragdoll.EnableSimulation();

                for (Light& light : Unloved::World::GetLights()) {
                    AABB aabb = ragdoll.GetWorldSpaceAABB();
                    if (aabb.IntersectsSphere(light.GetPosition(), light.GetRadius())) {
                        light.ForceDirty();
                    }
                }
            }
            ++it;
        }
    }

    void UpdateRatKing() {
        // Create if non-existent
        if (ratKingAnimatedObjectId == 0) {
            World::RemoveObjectById(ratKingAnimatedObjectId);

            ratKingAnimatedObjectId = World::AddAnimatedGameObject();
            AnimatedGameObject* object = World::GetAnimatedGameObjectByObjectId(ratKingAnimatedObjectId);
            if (!object) return;

            object->SetSkinnedModel("RatKing", "RatKing");
            object->SetPosition(glm::vec3(37.0f, 31.0f, 36.23f));
            object->PlayAndLoopAnimation("Main", "RatKid_GlockIdle3", 1.0f);
        }


        // Update
        AnimatedGameObject* object = World::GetAnimatedGameObjectByObjectId(ratKingAnimatedObjectId);
        if (!object) return;

        static bool ogPos = true;

        if (Unloved::Editor::IsClosed() && Hell::Input::MiddleMousePressed()) {
            ogPos = !ogPos;

            if (ogPos) {
                object->SetPosition(glm::vec3(37.0f, 31.0f, 36.73f));
            }
            else {
                object->SetPosition(glm::vec3(35.6f, 31.0f, 36.83f));
            }

            for (Light& light : Unloved::World::GetLights()) {
                light.ForceDirty();
            }
        }
    }

    void UpdateTrapKing() {
        // Create if non-existent
        if (trapKingAnimatedObjectId == 0) {
            World::RemoveObjectById(trapKingAnimatedObjectId);

            trapKingAnimatedObjectId = World::AddAnimatedGameObject();
            AnimatedGameObject* object = World::GetAnimatedGameObjectByObjectId(trapKingAnimatedObjectId);
            if (!object) return;

            object->SetSkinnedModel("TrapKing", "TrapKing");
            object->SetPosition(glm::vec3(36.0f, 31.0f, 36.23f));
            object->SetAnimationModeToBindPose();
        }
    }
}
