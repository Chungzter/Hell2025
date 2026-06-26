#include "Dobermann.h"
#include "Hell/Logging.h"
#include "Debug/DebugDraw.h"
#include "Unloved/SubSystems/NavMesh/NavMesh.h"
#include "Renderer/Renderer.h"
#include "World/LegacyWorld.h"
#include "Unloved/ObjectId.h"
#include "Hell/Audio.h"
namespace Audio = Hell::Audio;


// GET ME OUT OF HERE
#include "Unloved/Session/Session.h"
#include "Hell/Input.h"
namespace Input = Hell::Input;
// GET ME OUT OF HERE

void Dobermann::Init(DobermannCreateInfo createInfo) {
    m_createInfo = createInfo;

    PhysicsFilterData filterData;
    filterData.raycastGroup = RaycastGroup::RAYCAST_ENABLED;
    filterData.collisionGroup = CollisionGroup::RAGDOLL_ENEMY;
    filterData.collidesWith = CollisionGroup(ENVIROMENT_OBSTACLE | CHARACTER_CONTROLLER | RAGDOLL_ENEMY);

    m_objectId = Unloved::GetNextObjectId(ObjectType::DOBERMANN);
    m_RagdollId = Hell::Physics::SpawnRagdoll(createInfo.position, createInfo.eulerDirection, "dobermann_new", m_objectId, filterData);

    g_animatedGameObjectObjectId = LegacyWorld::CreateAnimatedGameObject();

    AnimatedGameObject* animatedGameObject = GetAnimatedGameObject();
    animatedGameObject->SetSkinnedModel("Dobermann");
    animatedGameObject->SetName("Dobermann " + std::to_string(m_objectId));
    animatedGameObject->SetMeshMaterialByMeshName("Body", "DobermannMouthBlood");
    animatedGameObject->SetMeshMaterialByMeshName("Jaw", "DobermannMouthBlood");
    animatedGameObject->SetMeshMaterialByMeshName("Tongue", "DobermannMouthBlood");
    animatedGameObject->SetMeshMaterialByMeshName("Iris", "DobermannIris");
    animatedGameObject->SetRagdollId(m_RagdollId);

    int32_t woundMaskIndex = Renderer::GetNextFreeWoundMaskIndexAndMarkItTaken();
    animatedGameObject->SetMeshWoundMaskTextureIndex("Body", woundMaskIndex);
    animatedGameObject->SetMeshWoundMaterialByMeshName("Body", "DobermannFullBlood");
    Logging::Debug() << "Assigned a Dobermann a 'Body' mesh wound mask index of " << woundMaskIndex;

    ResetToInitialState();

    CreateCharacterController(GetPosition());

    m_health = 1.0f;
}

void Dobermann::TakeDamage(uint32_t damage) {
    Ragdoll* ragdoll = Hell::Physics::GetRagdollById(m_RagdollId);
    AnimatedGameObject* animatedGameObject = GetAnimatedGameObject();
    animatedGameObject->SetAnimationModeToRagdoll();

    // Would this kill it?
    if (m_health > 0.0f && m_health - damage <= 0.0f) {
        Audio::PlayAudio("Dobermann_Death.wav", 1.0f);
    }

    // Apply damage
    m_health -= damage;
}

void Dobermann::SetPosition(const glm::vec3& position) {
    AnimatedGameObject* animatedGameObject = GetAnimatedGameObject();
    animatedGameObject->SetPosition(position);
}

void Dobermann::ResetToInitialState() {
    m_target = glm::vec3(0.0f);
    m_state = DobermannState::LAY;
    m_forward = m_initalForward;
    m_health = m_initalHealth;

    AnimatedGameObject* animatedGameObject = GetAnimatedGameObject();
    animatedGameObject->SetAnimationModeToBindPose();
    animatedGameObject->SetPosition(m_createInfo.position);
    animatedGameObject->PlayAndLoopAnimation("MainLayer", "Dobermann_Lay", 1.0f);

    if (CharacterController* characterController = GetCharacterController()) {
        characterController->SetPosition(m_createInfo.position);
    }

    UpdateAnimatedGameObjectRotation();
}


void Dobermann::UpdateAnimatedGameObjectRotation() {
    AnimatedGameObject* animatedGameObject = GetAnimatedGameObject();
    float rotY = Util::YRotationBetweenTwoPoints(GetPosition(), GetPosition() + GetForward()) + (HELL_PI * 0.5f);
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

void Dobermann::Update(float deltaTime) {
    AnimatedGameObject* animatedGameObject = GetAnimatedGameObject();
    Ragdoll* ragdoll = Hell::Physics::GetRagdollById(m_RagdollId);

    //DebugDraw();

    if (!ragdoll) return;
    if (!animatedGameObject) return;

    if (Input::KeyPressed(HELL_KEY_Y)) {
        ResetToInitialState();
    }

    if (Input::KeyPressed(HELL_KEY_T)) {
        if (Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(0)) {

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
                SetPosition(characterController->GetFootPosition());
            }

            // Did you reach the target
            float distanceToTarget = glm::distance(normalizedPosition, normalizedTarget);
            if (distanceToTarget < 0.2f) {
                m_state = DobermannState::SIT_FROM_LAY;
                animatedGameObject->PlayAnimation("MainLayer", "Dobermann_Stretch_to_Lay", 1.0f);
            }
        }

    }

    UpdateAnimatedGameObjectRotation();

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

    if (Renderer::GetCurrentRendererSettings().debugDrawRagdolls) {
        animatedGameObject->DisableRendering();
    }
    else {
        animatedGameObject->EnableRendering();
    }
    //DebugDraw::DrawPoint(GetPosition(), PINK);

}

AnimatedGameObject* Dobermann::GetAnimatedGameObject() {
    return LegacyWorld::GetAnimatedGameObjectByObjectId(g_animatedGameObjectObjectId);
}

glm::vec3 Dobermann::GetPosition() {
    AnimatedGameObject* animatedGameObject = GetAnimatedGameObject();
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