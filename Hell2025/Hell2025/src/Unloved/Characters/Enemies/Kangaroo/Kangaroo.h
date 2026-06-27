#pragma once
#include "Unloved/Common/Types.h"
#include "Unloved/Common/CreateInfo.h"
#include "Unloved/Common/Constants.h"

#include "Hell/Physics/Types/CharacterController.h"
#include "Hell/Physics/Ragdoll/Ragdoll.h"

#include "Unloved/Objects/Renderables/AnimatedGameObject.h"
#include "Unloved/Systems/Pathfinding/AStar.h"

namespace Unloved {

    enum struct KanagarooAgroState {
        CHILLING,
        ANGRY,
        KANGAROO_DEAD
    };

    enum struct KanagarooAnimationState {
        IDLE,
        HOP_TO_IDLE,
        IDLE_TO_HOP,
        HOP,
        BITE,
        RAGDOLL
    };

    enum struct KanagarooMovementState {
        IDLE,
        HOP,
        KANGAROO_DEAD
    };

    struct Kangaroo {
        void Init(KangarooCreateInfo createInfo);
        void Update(float deltaTime);
        void Kill();
        void GiveDamage(int damage);
        void CleanUp();
        void Respawn();

        void SetAgroState(KanagarooAgroState state);
        void SetMovementState(KanagarooMovementState state);

        void GoToTarget(glm::vec3 targetPosition);

        Unloved::AnimatedGameObject* GetAnimatedGameObject();
        Ragdoll* GetRagdoll();

        const std::string GetDebugInfoString();
        const std::string GetAnimationStateAsString();
        std::vector<glm::ivec2> GetPath();
        glm::vec2 GetGridPosition();

        CharacterController* GetCharacterController();

        void MarkWoundTextureAsCleared();

        int GetHealth()                             { return m_health; }
        uint64_t GetObjectId()                      { return m_objectId; }
        uint64_t GetCharacterControllerId ()        { return m_characterControllerId; }
        uint64_t GetRagdollId()                   { return m_RagdollId; }
        glm::vec3 GetPosition()                     { return m_position; }
        bool WoundTextureNeedsClearing()            { return m_woundTextureNeedsClearing; }

    private:
        void UpdateAnimatedGameObjectPositionRotation();
        void UpdateAudio();
        void UpdateMovementLogic(float deltaTime);
        void UpdateAnimationStateMachine();
    
        void FindPathToTarget();

        void PlayAnimation(const std::string& animationName, float speed);
        void PlayAndLoopAnimation(const std::string& animationName, float speed);
        bool AnimationIsComplete();
    
        void DebugDraw();


        // Audio wrappers
        void PlayFleshAudio();
        void PlayBiteSound();

        // Util
        bool HasValidPath();
        void CreateCharacterController(glm::vec3 position);
    

        float m_timeSinceBiteBegan = 0.0f;
        float m_timeSinceIdleBegan = 0.0f;

        AStar m_aStar;
        glm::vec3 m_targetPosition = glm::vec3(0.0f);
        bool m_alive = true;
        int m_maxHealth = KANGAROO_MAX_HEALTH;
        int m_health = 500;
        //int32_t m_woundMaskIndex = -1;

        glm::vec3 m_position;
        glm::vec3 m_rotation;

        glm::vec3 m_forward = glm::vec3(-1.0f, 0.0f, 0.0f);

        bool m_awaitingHopStepAudio = false;
        bool m_grounded = false;
        float m_yVelocity = 0;
        bool m_woundTextureNeedsClearing = false;

        uint64_t m_ambientLoopAudioHandle = 0;
        uint64_t m_objectId = 0;
        uint64_t m_characterControllerId = 0;
        uint64_t m_animatedGameObjectId = 0;
        uint64_t m_RagdollId = 0;

        KangarooCreateInfo m_createInfo;
        KanagarooAgroState m_agroState = KanagarooAgroState::CHILLING;
        KanagarooMovementState m_movementState = KanagarooMovementState::IDLE;
        KanagarooAnimationState m_animationState = KanagarooAnimationState::IDLE;
    };
}