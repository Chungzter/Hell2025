#pragma once

#include "Hell/Math/AABB.h"
#include "Hell/Physics/PhysicsTypes.h"

#include <cstdint>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace Hell::Physics {
    bool RigidDynamicIsKinematic(uint64_t rigidDynamicId);
    bool RigidDynamicIsDirty(uint64_t rigidDynamicId);
    glm::mat4 GetRigidDynamicWorldMatrix(uint64_t rigidDynamicId);
    void AddFoceToRigidDynamic(uint64_t rigidDynamicId, glm::vec3 force);
    void ActivateRigidDynamicPhysics(uint64_t rigidDynamicId);
    void DeactivateRigidDynamicPhysics(uint64_t rigidDynamicId);
    void SetRigidDynamicUserData(uint64_t rigidDynamicId, PhysicsUserData physicsUserData);
    void SetRigidDynamicGlobalPose(uint64_t rigidDynamicId, const glm::mat4& globalPoseMatrix);
    void SetRigidDynamicKinematicTarget(uint64_t rigidDynamicId, const glm::mat4& globalPoseMatrix);

    glm::mat4 GetRigidStaticGlobalPose(uint64_t rigidStaticId);
    void SetRigidStaticWorldTransform(uint64_t rigidStaticId, glm::mat4 worldMatrix);
    void SetRigidStaticUserData(uint64_t rigidStaticId, PhysicsUserData physicsUserData);

    AABB GetCharacterControllerAABB(uint64_t characterControllerId);
    glm::vec3 GetCharacterControllerPosition(uint64_t characterControllerId);
    void MoveCharacterController(uint64_t characterControllerId, glm::vec3 displacement);
}
