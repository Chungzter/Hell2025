#pragma once

#include "Unloved/Common/CreateInfo.h"

namespace Unloved {

struct AnimatedGameObject;

struct GenericAnimatedObject {
    GenericAnimatedObject() = default;
    GenericAnimatedObject(uint64_t id, const GenericAnimatedObjectCreateInfo& createInfo, const SpawnOffset& spawnOffset);
    GenericAnimatedObject(const GenericAnimatedObject&) = delete;
    GenericAnimatedObject& operator=(const GenericAnimatedObject&) = delete;
    GenericAnimatedObject(GenericAnimatedObject&&) noexcept = default;
    GenericAnimatedObject& operator=(GenericAnimatedObject&&) noexcept = default;
    ~GenericAnimatedObject() = default;

    void CleanUp();
    void SetPosition(const glm::vec3& position);
    void SetRotation(const glm::vec3& rotation);
    void SetScale(float scale);
    void SetType(GenericAnimatedObjectType type);
    void SetAnimationName(const std::string& animationName);
    void SetAnimationSpeed(float animationSpeed);

    uint64_t GetObjectId() const                                        { return m_objectId; }
    uint64_t GetAnimatedGameObjectId() const                            { return m_animatedGameObjectId; }
    const glm::vec3& GetPosition() const                                { return m_createInfo.position; }
    const glm::vec3& GetRotation() const                                { return m_createInfo.rotation; }
    float GetScale() const                                              { return m_createInfo.scale; }
    GenericAnimatedObjectType GetType() const                           { return m_createInfo.type; }
    const std::string& GetEditorName() const                            { return m_createInfo.editorName; }
    const GenericAnimatedObjectCreateInfo& GetCreateInfo() const        { return m_createInfo; }
    AnimatedGameObject* GetAnimatedGameObject();

private:
    void CreateAnimatedGameObject();
    void ApplyTransform();
    void RestartAnimation();

    GenericAnimatedObjectCreateInfo m_createInfo;
    uint64_t m_objectId = 0;
    uint64_t m_animatedGameObjectId = 0;
};
}
