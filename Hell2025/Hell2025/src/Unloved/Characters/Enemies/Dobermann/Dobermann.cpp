#include "Dobermann.h"
#include "Hell/Math/Rotation.h"
#include "Hell/Logging.h"
#include "Hell/Physics/Physics.h"
#include "Unloved/Debug/DebugDraw.h"
#include "Unloved/Systems/NavMesh/NavMesh.h"
#include "Unloved/Render/Renderer.h"
#include "World/LegacyWorld.h"
#include "Unloved/ObjectId.h"
#include "Unloved/World/World.h"
#include "Hell/Audio.h"
namespace Audio = Hell::Audio;

#include <glm/gtc/quaternion.hpp>

// GET ME OUT OF HERE
#include "Unloved/Session/Session.h"
#include "Hell/Input.h"
namespace Input = Hell::Input;
// GET ME OUT OF HERE

namespace {
    glm::vec3 YawOnlyRotation(const glm::vec3& rotation) {
        return glm::vec3(0.0f, rotation.y, 0.0f);
    }

    glm::vec3 ForwardFromRotation(const glm::vec3& rotation) {
        return glm::normalize(glm::quat(rotation) * glm::vec3(0.0f, 0.0f, 1.0f));
    }
}

namespace Unloved {

    Dobermann::Dobermann(uint64_t id, DobermannCreateInfo createInfo, SpawnOffset spawnOffset) {
        Init(id, createInfo, spawnOffset);
    }

    void Dobermann::Init(uint64_t id, DobermannCreateInfo createInfo, SpawnOffset spawnOffset) {
        m_createInfo = createInfo;
        m_createInfo.position += spawnOffset.translation;
        m_createInfo.rotation.y += spawnOffset.yRotation;
        m_createInfo.rotation = YawOnlyRotation(m_createInfo.rotation);
        m_initalForward = ForwardFromRotation(m_createInfo.rotation);
        m_forward = m_initalForward;

        PhysicsFilterData filterData;
        filterData.raycastGroup = RaycastGroup::RAYCAST_ENABLED;
        filterData.collisionGroup = CollisionGroup::RAGDOLL_ENEMY;
        filterData.collidesWith = CollisionGroup(ENVIROMENT_OBSTACLE | CHARACTER_CONTROLLER | RAGDOLL_ENEMY);

        m_objectId = id;
        m_RagdollId = Hell::Physics::SpawnRagdoll(m_createInfo.position, m_createInfo.rotation, "dobermann_new", m_objectId, filterData);

        g_animatedGameObjectObjectId = LegacyWorld::CreateAnimatedGameObject();

        Unloved::AnimatedGameObject* animatedGameObject = GetAnimatedGameObject();
        animatedGameObject->SetOwnerObjectId(m_objectId);
        animatedGameObject->SetSkinnedModel("Dobermann");
        animatedGameObject->SetName("Dobermann " + std::to_string(m_objectId));
        animatedGameObject->SetMeshMaterialByMeshName("Body", "DobermannMouthBlood");
        animatedGameObject->SetMeshMaterialByMeshName("Jaw", "DobermannMouthBlood");
        animatedGameObject->SetMeshMaterialByMeshName("Tongue", "DobermannMouthBlood");
        animatedGameObject->SetMeshMaterialByMeshName("Iris", "DobermannIris");
        animatedGameObject->SetRagdollId(m_RagdollId);

        int32_t woundMaskIndex = Renderer::GetNextFreeWoundMaskIndexAndMarkItTaken();
        animatedGameObject->SetMeshWoundMaskArrayIndex("Body", woundMaskIndex);
        animatedGameObject->SetMeshWoundMaterialByMeshName("Body", "DobermannFullBlood");
        Logging::Debug() << "Assigned a Dobermann a 'Body' mesh wound mask index of " << woundMaskIndex;

        ResetToInitialState();

        CreateCharacterController(GetPosition());

        m_health = 1.0f;
    }

    void Dobermann::CleanUp() {
        Unloved::AnimatedGameObject* animatedGameObject = GetAnimatedGameObject();
        if (animatedGameObject) {
            animatedGameObject->CleanUp();
            Unloved::World::GetAnimatedGameObjects().erase(g_animatedGameObjectObjectId);
        }
        g_animatedGameObjectObjectId = 0;
        Hell::Physics::MarkCharacterControllerForRemoval(m_characterControllerId);
        m_characterControllerId = 0;
    }

    void Dobermann::TakeDamage(uint32_t damage) {
        const bool wasAlive = IsAlive();
        m_health -= damage;

        if (wasAlive && IsDead()) {
            Audio::PlayAudio("Dobermann_Death.wav", 1.0f);
            if (Unloved::AnimatedGameObject* animatedGameObject = GetAnimatedGameObject()) {
                animatedGameObject->SetAnimationModeToRagdoll();
            }
        }
    }

    void Dobermann::SetPosition(const glm::vec3& position) {
        m_createInfo.position = position;

        Unloved::AnimatedGameObject* animatedGameObject = GetAnimatedGameObject();
        animatedGameObject->SetPosition(position);

        if (Ragdoll* ragdoll = Hell::Physics::GetRagdollById(m_RagdollId)) {
            ragdoll->SetSpawnPosition(position);
        }

        if (CharacterController* characterController = GetCharacterController()) {
            characterController->SetPosition(position);
        }
    }

    void Dobermann::SetRotation(const glm::vec3& rotation) {
        m_createInfo.rotation = YawOnlyRotation(rotation);
        m_initalForward = ForwardFromRotation(m_createInfo.rotation);
        m_forward = m_initalForward;

        if (Ragdoll* ragdoll = Hell::Physics::GetRagdollById(m_RagdollId)) {
            ragdoll->SetSpawnRotation(m_createInfo.rotation);
        }

        UpdateAnimatedGameObjectRotation();
    }

    void Dobermann::ResetToInitialState() {
        m_target = glm::vec3(0.0f);
        m_state = DobermannState::LAY;
        m_forward = m_initalForward;
        m_health = m_initalHealth;

        Unloved::AnimatedGameObject* animatedGameObject = GetAnimatedGameObject();
        animatedGameObject->SetAnimationModeToBindPose();
        animatedGameObject->SetPosition(m_createInfo.position);
        animatedGameObject->PlayAndLoopAnimation("MainLayer", "Dobermann_Lay", 1.0f);

        if (CharacterController* characterController = GetCharacterController()) {
            characterController->SetPosition(m_createInfo.position);
        }

        UpdateAnimatedGameObjectRotation();
    }


    void Dobermann::UpdateAnimatedGameObjectRotation() {
        Unloved::AnimatedGameObject* animatedGameObject = GetAnimatedGameObject();
        float rotY = Hell::Math::YawBetweenPoints(GetPosition(), GetPosition() + GetForward()) + (HELL_PI * 0.5f);
        animatedGameObject->SetRotationY(rotY);
    }

    void Dobermann::DebugDraw() {
        // Forward
        glm::vec3 p1 = GetPosition();
        glm::vec3 p2 = GetPosition() + GetForward() * 0.25f;
        DebugDraw::DrawPoint(p1, GREEN);
        DebugDraw::DrawPoint(p2, GREEN);
        DebugDraw::DrawLine(p1, p2, GREEN);

        // Path
        Unloved::NavMeshManager::DrawPath(m_path, WHITE);
    }

    void Dobermann::UpdateMovement(float deltaTime) {
        Unloved::AnimatedGameObject* animatedGameObject = GetAnimatedGameObject();
        Ragdoll* ragdoll = Hell::Physics::GetRagdollById(m_RagdollId);

        //DebugDraw();

        if (!ragdoll) return;
        if (!animatedGameObject) return;

        if (Input::KeyPressed(HELL_KEY_Y)) {
            ResetToInitialState();
        }

        if (Input::KeyPressed(HELL_KEY_T)) {
            if (Unloved::Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(0)) {

                m_target = player->GetInteractHitPosition();

                if (m_state == DobermannState::LAY) {
                    m_state = DobermannState::GET_UP_FROM_LAY;
                    animatedGameObject->PlayAnimation("MainLayer", "Dobermann_Lay_to_Walk", 1.0f);
                }
            }
        }

        if (animatedGameObject->IsAllAnimationsComplete()) {

            if (m_state == DobermannState::GET_UP_FROM_LAY) {
                m_state = DobermannState::WALK_TO_TARGET;
                animatedGameObject->PlayAndLoopAnimation("MainLayer", "Dobermann_Walk", 1.0f);
            }

            if (m_state == DobermannState::SIT_FROM_LAY) {
                m_state = DobermannState::LAY;
                animatedGameObject->PlayAnimation("MainLayer", "Dobermann_Lay", 1.0f);
            }
        }

        // WALK
        if (m_state == DobermannState::WALK_TO_TARGET) {
            float speed = 1.0f;

            m_path = Unloved::NavMeshManager::FindPath(GetPosition(), m_target);

        
            if (m_path.size() >= 2) {

                // Compute and calculate a new forward vector based on the next path point
                const glm::vec3 normalizedPosition = GetPosition() * glm::vec3(1.0f, 0.0f, 1.0f);
                const glm::vec3 normalizedNextPathPosition = m_path[1] * glm::vec3(1.0f, 0.0f, 1.0f);
                const glm::vec3 normalizedTarget = m_target * glm::vec3(1.0f, 0.0f, 1.0f);
                glm::vec3 targetForward = glm::normalize(normalizedNextPathPosition - normalizedPosition);
                float turnSpeed = 5.5f;
                float alpha = glm::clamp(turnSpeed * deltaTime, 0.0f, 1.0f);
                m_forward = glm::normalize(m_forward * (1.0f - alpha) + targetForward * alpha);

                glm::vec3 displacement = m_forward * speed * deltaTime;

                Hell::Physics::MoveCharacterController(m_characterControllerId, displacement);

                if (CharacterController* characterController = GetCharacterController()) {
                    animatedGameObject->SetPosition(characterController->GetFootPosition());
                }

                // Did you reach the target
                float distanceToTarget = glm::distance(normalizedPosition, normalizedTarget);
                if (distanceToTarget < 0.2f) {
                    m_state = DobermannState::SIT_FROM_LAY;
                    animatedGameObject->PlayAnimation("MainLayer", "Dobermann_Stretch_to_Lay", 1.0f);
                }
            }

        }

        //
        //
        //
        //if (Input::KeyPressed(HELL_KEY_Y)) {
        //    ragdoll->SetToInitialPose();
        //    animatedGameObject->SetAnimationModeToAnimated();
        //    animatedGameObject->PlayAndLoopAnimation("MainLayer", "Dobermann_Lay", 1.0f);
        //    m_health = 1.0f;
        //}
        //
        //static bool gettingUp = false;
        //
        //if (Input::KeyPressed(HELL_KEY_LEFT)) {
        //    animatedGameObject->PlayAndLoopAnimation("MainLayer", "Dobermann_Lay", 1.0f);
        //    gettingUp = false;
        //}
        //if (Input::KeyPressed(HELL_KEY_RIGHT)) {
        //    animatedGameObject->PlayAndLoopAnimation("MainLayer", "Dobermann_Walk", 1.0f);
        //    gettingUp = false;
        //}
        //if (Input::KeyPressed(HELL_KEY_UP)) {
        //    animatedGameObject->PlayAnimation("MainLayer", "Dobermann_Lay_to_Walk", 1.0f);
        //    gettingUp = true;
        //}
        //if (Input::KeyPressed(HELL_KEY_DOWN)) {
        //    gettingUp = false;
        //    animatedGameObject->PlayAnimation("MainLayer", "Dobermann_Stretch_to_Lay", 1.0f);
        //}
        //
        //if (animatedGameObject->IsAllAnimationsComplete() && gettingUp) {
        //    animatedGameObject->PlayAndLoopAnimation("MainLayer", "Dobermann_Walk", 1.0f);
        //}

        //DebugDraw::DrawPoint(GetPosition(), PINK);

    }

    void Dobermann::Update(float deltaTime) {
        (void)deltaTime;

        Unloved::AnimatedGameObject* animatedGameObject = GetAnimatedGameObject();
        Ragdoll* ragdoll = Hell::Physics::GetRagdollById(m_RagdollId);

        if (!ragdoll) return;
        if (!animatedGameObject) return;

        UpdateAnimatedGameObjectRotation();

        if (Renderer::GetCurrentRendererSettings().debugDrawRagdolls) {
            animatedGameObject->DisableRendering();
        }
        else {
            animatedGameObject->EnableRendering();
        }
    }

    Unloved::AnimatedGameObject* Dobermann::GetAnimatedGameObject() {
        return Unloved::World::GetAnimatedGameObjectByObjectId(g_animatedGameObjectObjectId);
    }

    glm::vec3 Dobermann::GetPosition() {
        Unloved::AnimatedGameObject* animatedGameObject = GetAnimatedGameObject();
        if (!animatedGameObject) return glm::vec3(0.0f);

        return animatedGameObject->GetModelMatrix()[3];

    }

    void Dobermann::CreateCharacterController(const glm::vec3& position) {
        float capsuleHeight = 0.2f;
        float capsuleRadius = 0.15;

        PhysicsFilterData physicsFilterData;
        physicsFilterData.raycastGroup = RaycastGroup::RAYCAST_ENABLED;
        physicsFilterData.collisionGroup = CollisionGroup::CHARACTER_CONTROLLER;
        //physicsFilterData.collidesWith = CollisionGroup(ENVIROMENT_OBSTACLE | CHARACTER_CONTROLLER);
        physicsFilterData.collidesWith = CollisionGroup(ENVIROMENT_OBSTACLE);

        m_characterControllerId = Hell::Physics::CreateCharacterController(m_objectId, position, capsuleHeight, capsuleRadius, physicsFilterData);
    }

    CharacterController* Dobermann::GetCharacterController() {
        return Hell::Physics::GetCharacterControllerById(m_characterControllerId);
    }
}
