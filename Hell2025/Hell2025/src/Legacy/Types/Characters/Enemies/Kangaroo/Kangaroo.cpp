#include "Kangaroo.h"
#include "Hell/Physics/Physics.h"
#include "Pathfinding/AStarMap.h"
#include "World/LegacyWorld.h"
#include "Unloved/Session/Session.h"
#include "Renderer/Renderer.h"
#include "Hell/Logging.h"
#include "Unloved/ObjectId.h"
#include "Timer.hpp"
#include "Hell/Audio.h"
namespace Audio = Hell::Audio;

void Kangaroo::Init(KangarooCreateInfo createInfo) {
    m_createInfo = createInfo;
    m_objectId = Unloved::GetNextObjectId(ObjectType::KANGAROO);

    Respawn();
    
    if (m_animatedGameObjectId == 0) {
        m_animatedGameObjectId = LegacyWorld::CreateAnimatedGameObject();

        PhysicsFilterData filterData;
        filterData.raycastGroup = RaycastGroup::RAYCAST_ENABLED;
        filterData.collisionGroup = CollisionGroup::RAGDOLL_ENEMY;
        filterData.collidesWith = CollisionGroup(ENVIROMENT_OBSTACLE | CHARACTER_CONTROLLER | RAGDOLL_ENEMY);

        AnimatedGameObject* animatedGameObject = GetAnimatedGameObject();
        animatedGameObject->SetSkinnedModel("Kangaroo");
        animatedGameObject->SetRotationY(HELL_PI);
        animatedGameObject->SetAnimationModeToBindPose();
        animatedGameObject->SetName("Roo");

        m_RagdollId = Hell::Physics::SpawnRagdoll(m_position, m_rotation, "Kangaroo", m_objectId, filterData);

        animatedGameObject->SetRagdollId(m_RagdollId);
        animatedGameObject->SetAllMeshMaterials("Kangaroo");
        animatedGameObject->SetMeshMaterialByMeshName("LeftEye_Iris", "KangarooIris");
        animatedGameObject->SetMeshMaterialByMeshName("RightEye_Iris", "KangarooIris");

        animatedGameObject->SetBlendingModeByMeshName("LeftEye_Sclera", BlendingMode::DO_NOT_RENDER);
        animatedGameObject->SetBlendingModeByMeshName("RightEye_Sclera", BlendingMode::DO_NOT_RENDER);

        animatedGameObject->PlayAndLoopAnimation("MainLayer", "Kangaroo_Idle", 1.0f);

        int32_t woundMaskIndex = Renderer::GetNextFreeWoundMaskIndexAndMarkItTaken();

        animatedGameObject->SetMeshWoundMaskTextureIndex("Body", woundMaskIndex);
        animatedGameObject->SetMeshWoundMaterialByMeshName("Body", "KangarooBlood");
           
        //Logging::Debug() << "Assigned a Kangaroo a 'Body' mesh wound mask index of " << woundMaskIndex;

        CreateCharacterController(m_createInfo.position);
    }
}

void Kangaroo::Respawn() {
    m_position = m_createInfo.position;
    m_rotation = m_createInfo.rotation;
    m_forward = glm::vec3(-1.0f, 0.0f, 0.0f);
    m_alive = true;
    m_health = m_maxHealth;
    m_yVelocity = 0;

    m_agroState = KanagarooAgroState::CHILLING;
    m_movementState = KanagarooMovementState::IDLE;
    m_animationState = KanagarooAnimationState::IDLE;
    m_woundTextureNeedsClearing = true;

    AnimatedGameObject* animatedGameObject = GetAnimatedGameObject();
    if (animatedGameObject) {
        animatedGameObject->SetPosition(m_position);
        animatedGameObject->SetRotationX(m_rotation.x);
        animatedGameObject->SetRotationY(m_rotation.y);
        animatedGameObject->SetRotationZ(m_rotation.z);

        if (Ragdoll* ragdoll = GetRagdoll()) {
            ragdoll->SetToInitialPose();
            ragdoll->DisableSimulation();
        }
        animatedGameObject->SetAnimationModeToAnimated();
        animatedGameObject->PlayAndLoopAnimation("MainLayer", "Kangaroo_Idle", 1.0f);
    }

    CharacterController* characterController = GetCharacterController();
    if (characterController) {
        characterController->SetPosition(m_createInfo.position);
    }
}

AnimatedGameObject* Kangaroo::GetAnimatedGameObject(){
    return LegacyWorld::GetAnimatedGameObjectByObjectId(m_animatedGameObjectId);
}

Ragdoll* Kangaroo::GetRagdoll() {
    if (m_RagdollId == 0) {
        return nullptr;
    }
    return Hell::Physics::GetRagdollById(m_RagdollId);
}

void Kangaroo::Kill() {
    if (m_alive) {
        Audio::PlayAudio("Kangaroo_Death.wav", 1.0f);

        AnimatedGameObject* animatedGameObject = GetAnimatedGameObject();
        if (animatedGameObject) {
            animatedGameObject->SetAnimationModeToRagdoll();
        }
        m_health = 0;
        m_alive = false;
        m_agroState = KanagarooAgroState::KANGAROO_DEAD;
        m_animationState = KanagarooAnimationState::RAGDOLL;
        m_movementState = KanagarooMovementState::KANGAROO_DEAD;
        std::cout << "Killed kangaroo\n";
    }
}

void Kangaroo::GiveDamage(int damage) {
    m_health -= damage;
    Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(0);
    glm::vec3 playerPosition = player->GetCameraPosition();
    GoToTarget(playerPosition);
    PlayFleshAudio();
    return;
    m_agroState = KanagarooAgroState::ANGRY;
}

void Kangaroo::CleanUp() {
    AnimatedGameObject* animatedGameObject = GetAnimatedGameObject();
    if (animatedGameObject) {
        animatedGameObject->CleanUp();
    }
}

void Kangaroo::SetAgroState(KanagarooAgroState state) {
    m_agroState = state;
}

void Kangaroo::SetMovementState(KanagarooMovementState state) {
    m_movementState = state;
}

void Kangaroo::PlayAnimation(const std::string& animationName, float speed) {
    AnimatedGameObject* animatedGameObject = GetAnimatedGameObject();
    if (animatedGameObject) {
        animatedGameObject->PlayAnimation("MainLayer", animationName, speed);
    }
}

void Kangaroo::PlayAndLoopAnimation(const std::string& animationName, float speed) {
    AnimatedGameObject* animatedGameObject = GetAnimatedGameObject();
    if (animatedGameObject) {
        animatedGameObject->PlayAndLoopAnimation("MainLayer", animationName, speed);
    }
}

bool Kangaroo::AnimationIsComplete() {
    AnimatedGameObject* animatedGameObject = GetAnimatedGameObject();
    if (!animatedGameObject) return false;
    return animatedGameObject->IsAllAnimationsComplete();
}

CharacterController* Kangaroo::GetCharacterController() {
    return Hell::Physics::GetCharacterControllerById(m_characterControllerId);
}

glm::vec2 Kangaroo::GetGridPosition() {
    return AStarMap::GetCellCoordsFromWorldSpacePosition(m_position);
}

std::vector<glm::ivec2> Kangaroo::GetPath() {
    return m_aStar.GetPath();
}

void Kangaroo::MarkWoundTextureAsCleared() {
    m_woundTextureNeedsClearing = false;
}