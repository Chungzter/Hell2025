#pragma once
#include "Unloved/Common/Types.h"
#include "Unloved/Common/CreateInfo.h"
#include "Unloved/Objects/Renderables/AnimatedGameObject.h"
#include "Hell/Physics/Types/CharacterController.h"

namespace Unloved {

    enum struct DobermannState {
        LAY,
        GET_UP_FROM_LAY,
        WALK_TO_TARGET,
        SIT_FROM_LAY
    };

    struct Dobermann {
        Dobermann() = default;
        Dobermann(uint64_t id, DobermannCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());

	    void Init(uint64_t id, DobermannCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
	    void UpdateMovement(float deltaTime);
	    void Update(float deltaTime);
        void CleanUp();

        void ResetToInitialState();
        void SetPosition(const glm::vec3& position);
        void SetRotation(const glm::vec3& rotation);

	    void TakeDamage(uint32_t damage);

        void DebugDraw();

        Unloved::AnimatedGameObject* GetAnimatedGameObject(); 
        CharacterController* GetCharacterController();
        glm::vec3 GetPosition();

        const glm::vec3& GetForward() const             { return m_forward; }
        const glm::vec3& GetRotation() const            { return m_createInfo.rotation; }
        uint64_t GetObjectId() const                    { return m_objectId; }
	    uint64_t GetRagdollId()                       { return m_RagdollId; }
        bool IsAlive() const                            { return m_health > 0.0f; }
        bool IsDead() const                             { return !IsAlive(); }
        const DobermannState GetDobermannState() const  { return m_state; }
        const DobermannCreateInfo& GetCreateInfo() const { return m_createInfo; }
        const std::string& GetEditorName() const        { return m_createInfo.editorName; }

    private:
        void CreateCharacterController(const glm::vec3& position);
        void UpdateAnimatedGameObjectRotation();

        DobermannCreateInfo m_createInfo;
	    uint64_t g_animatedGameObjectObjectId = 0;
	    uint64_t m_objectId = 0;
	    uint64_t m_RagdollId = 0;
        uint64_t m_characterControllerId = 0;
	    float m_health = 0.0f;
        float m_initalHealth = 1.0f;
        DobermannState m_state = DobermannState::LAY;
        glm::vec3 m_target = glm::vec3(0.0f);
        std::vector<glm::vec3> m_path;

        glm::vec3 m_initalForward = glm::vec3(0.0f, 0.0f, 1.0f);
        glm::vec3 m_forward = glm::vec3(0.0f);
    };
}
