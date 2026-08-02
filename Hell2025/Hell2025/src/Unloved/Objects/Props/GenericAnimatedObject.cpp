#include "GenericAnimatedObject.h"

#include "Unloved/Bible/Bible.h"
#include "Unloved/Objects/Renderables/AnimatedGameObject.h"
#include "Unloved/World/World.h"

namespace Unloved {

GenericAnimatedObject::GenericAnimatedObject(uint64_t id, const GenericAnimatedObjectCreateInfo& createInfo, const SpawnOffset& spawnOffset) {
    m_objectId = id;
    m_createInfo = createInfo;
    m_createInfo.position += spawnOffset.translation;
    m_createInfo.rotation.y += spawnOffset.yRotation;
    CreateAnimatedGameObject();
}

void GenericAnimatedObject::CleanUp() {
    if (m_animatedGameObjectId == 0) return;
    World::RemoveObjectById(m_animatedGameObjectId);
    m_animatedGameObjectId = 0;
}

void GenericAnimatedObject::SetPosition(const glm::vec3& position) {
    m_createInfo.position = position;
    if (AnimatedGameObject* animatedGameObject = GetAnimatedGameObject()) animatedGameObject->SetPosition(position);
}

void GenericAnimatedObject::SetRotation(const glm::vec3& rotation) {
    m_createInfo.rotation = rotation;
    if (AnimatedGameObject* animatedGameObject = GetAnimatedGameObject()) {
        animatedGameObject->SetRotationX(rotation.x);
        animatedGameObject->SetRotationY(rotation.y);
        animatedGameObject->SetRotationZ(rotation.z);
    }
}

void GenericAnimatedObject::SetScale(float scale) {
    m_createInfo.scale = scale;
    if (AnimatedGameObject* animatedGameObject = GetAnimatedGameObject()) animatedGameObject->SetScale(scale);
}

void GenericAnimatedObject::SetType(GenericAnimatedObjectType type) {
    if (m_createInfo.type == type) return;
    m_createInfo.type = type;
    CleanUp();
    CreateAnimatedGameObject();
}

void GenericAnimatedObject::SetAnimationName(const std::string& animationName) {
    if (m_createInfo.animationName == animationName) return;
    m_createInfo.animationName = animationName;
    RestartAnimation();
}

void GenericAnimatedObject::SetAnimationSpeed(float animationSpeed) {
    if (m_createInfo.animationSpeed == animationSpeed) return;
    m_createInfo.animationSpeed = animationSpeed;
    RestartAnimation();
}

AnimatedGameObject* GenericAnimatedObject::GetAnimatedGameObject() {
    return World::GetAnimatedGameObjectByObjectId(m_animatedGameObjectId);
}

void GenericAnimatedObject::CreateAnimatedGameObject() {
    m_animatedGameObjectId = World::AddAnimatedGameObject();
    AnimatedGameObject* animatedGameObject = GetAnimatedGameObject();
    if (!animatedGameObject) {
        m_animatedGameObjectId = 0;
        return;
    }

    animatedGameObject->SetOwnerObjectId(m_objectId);
    Bible::ConfigureGenericAnimatedObject(m_createInfo.type, animatedGameObject);
    ApplyTransform();
    RestartAnimation();
}

void GenericAnimatedObject::ApplyTransform() {
    AnimatedGameObject* animatedGameObject = GetAnimatedGameObject();
    if (!animatedGameObject) return;

    animatedGameObject->SetPosition(m_createInfo.position);
    animatedGameObject->SetRotationX(m_createInfo.rotation.x);
    animatedGameObject->SetRotationY(m_createInfo.rotation.y);
    animatedGameObject->SetRotationZ(m_createInfo.rotation.z);
    animatedGameObject->SetScale(m_createInfo.scale);
}

void GenericAnimatedObject::RestartAnimation() {
    AnimatedGameObject* animatedGameObject = GetAnimatedGameObject();
    if (!animatedGameObject || !animatedGameObject->GetSkinnedModel()) return;

    if (m_createInfo.animationName.empty()) animatedGameObject->SetAnimationModeToBindPose();
    else animatedGameObject->PlayAndLoopAnimation("Main", m_createInfo.animationName, m_createInfo.animationSpeed);
}
}
