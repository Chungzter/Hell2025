#pragma once

#include "Hell/Physics/Ragdoll/Ragdoll.h"

#include "Unloved/Common/CreateInfo.h"
#include "Unloved/Objects/Renderables/AnimatedGameObject.h"

#include <cstdint>
#include <string>
#include <vector>

enum class SharkMovementState {
    STOPPED,
    FOLLOWING_PATH,
    FOLLOWING_PATH_ANGRY,
    ARROW_KEYS,
    HUNT_PLAYER,
    UNDEFINED
};

enum class SharkHuntingState {
    CHARGE_PLAYER,
    BITING_PLAYER,
    UNDEFINED
};

enum class SharkMovementDirection {
    STRAIGHT,
    LEFT,
    RIGHT,
    UNDEFINED
};

namespace Unloved {

    #define SHARK_SPINE_SEGMENT_COUNT 11
    #define COLLISION_SPHERE_RADIUS 1
    #define COLLISION_TEST_STEP_COUNT 40
    #define SHARK_HEALTH_MAX 1000

    struct Shark {
        Shark() = default;
        Shark(uint64_t id, const SharkCreateInfo& createInfo, const SpawnOffset& spawnOffset);
        Shark(const Shark&) = delete;
        Shark& operator=(const Shark&) = delete;
        Shark(Shark&&) noexcept = default;
        Shark& operator=(Shark&&) noexcept = default;
        ~Shark() = default;

        void Init();
        void Update(float deltaTime);
        void SetPosition(const glm::vec3& position);
        void CleanUp();
        void DrawSpinePoints();
        void HuntPlayer(uint64_t playerId);
        void GiveDamage(uint64_t playerId, int damageAmount);
        void Kill();
        void Respawn();
        void SetPositionToBeginningOfPath();
        void PlayAnimation(const std::string& animationName, float speed);
        void PlayAndLoopAnimation(const std::string& animationName, float speed);
        void SetMovementState(SharkMovementState state);
        void StraightenSpine(float deltaTime, float straightSpeed);

        std::string GetDebugInfoAsString();
        void DrawDebug();

        Unloved::AnimatedGameObject* GetAnimatedGameObject();
        Ragdoll* GetRagdoll();

        glm::vec3 m_spinePositions[SHARK_SPINE_SEGMENT_COUNT];
        std::string m_spineBoneNames[SHARK_SPINE_SEGMENT_COUNT];
        float m_spineSegmentLengths[SHARK_SPINE_SEGMENT_COUNT - 1];

        const bool IsDead() const { return !m_alive; }
        const bool IsAlive() const { return m_alive; }
        const uint64_t& GetObjectId() const { return m_objectId; };
        const uint64_t& GetRagdollId() const { return m_RagdollId; };

        SharkHuntingState GetHuntingState() { return m_huntingState; }
        SharkMovementState GetMovementState() { return m_movementState; }

        const SharkCreateInfo& GetCreateInfo() const { return m_createInfo; }

    private:
        void CalculateTargetFromPath();
        void CalculateForwardVectorFromTarget(float deltaTime);
        void CalculateForwardVectorFromArrowKeys(float deltaTime);
        void CalculateTargetFromPlayer();
        void MoveShark(float deltaTime);

        void UpdateHuntingLogic(float deltaTime);

        // Util
        int GetAnimationFrameNumber();
        float GetDistanceMouthToTarget3D();
        float GetDistanceToTarget2D();
        float GetTurningRadius() const;
        bool TargetIsOnLeft(glm::vec3 targetPosition);
        bool IsBehindEvadePoint(glm::vec3 position);
        glm::vec3 GetMouthPosition3D();
        glm::vec3 GetForwardVector();
        glm::vec3 GetTargetPosition2D();
        glm::vec3 GetHeadPosition2D();
        glm::vec3 GetMouthPosition2D();
        glm::vec3 GetCollisionLineEnd();
        glm::vec3 GetCollisionSphereFrontPosition();
        glm::vec3 GetSpinePosition(int index);
        glm::vec3 GetEvadePoint3D();
        glm::vec3 GetEvadePoint2D();


        uint64_t m_objectId = 0;
        uint64_t m_RagdollId = 0;
        uint64_t g_animatedGameObjectObjectId = 0; 
        uint64_t m_huntedPlayerId = 0;
        int m_health = SHARK_HEALTH_MAX;
        int m_logicSubStepCount = 8;
        float m_swimSpeed = 8.0f;
        float m_rotationSpeed = 2.5f;
        glm::vec3 m_forward = glm::vec3(0);
        glm::vec3 m_right = glm::vec3(0);
        glm::vec3 m_lastKnownTargetPosition = glm::vec3(0);
        glm::vec3 m_left = glm::vec3(0); 
        bool m_hasBitPlayer = false;
        bool m_alive = false; 
        bool m_playerSafe = false;
        float m_yHeight = 0.0f;

        SharkHuntingState m_huntingState = SharkHuntingState::UNDEFINED;
        SharkMovementState m_movementState = SharkMovementState::FOLLOWING_PATH;

        int32_t m_nextPathPointIndex = 0;
        glm::vec3 m_targetPosition = glm::vec3(0);
        std::vector<glm::vec3> m_path;
        SharkCreateInfo m_createInfo;
    };
}
