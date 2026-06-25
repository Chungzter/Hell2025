#pragma once

#pragma warning(push, 0)
#include <physx/PxPhysicsAPI.h>
#pragma warning(pop)

#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <string>

namespace Hell::Physics {
    bool PxTransformNearlyEqual(const physx::PxTransform& a, const physx::PxTransform& b);
    std::string GetPxShapeTypeAsString(physx::PxShape* pxShape);
    float ComputeShapeVolume(physx::PxShape* pxShape);
    glm::vec3 PxVec3toGlmVec3(physx::PxVec3 vec);
    glm::vec3 PxVec3toGlmVec3(physx::PxExtendedVec3 vec);
    glm::quat PxQuatToGlmQuat(physx::PxQuat quat);
    glm::mat4 PxMat44ToGlmMat4(physx::PxMat44 pxMatrix);
    glm::vec3 GetHeightMapPositionAtXZ(float x, float z);
    physx::PxVec3 GlmVec3toPxVec3(const glm::vec3& vec);
    physx::PxQuat GlmQuatToPxQuat(const glm::quat& quat);
    physx::PxMat44 GlmMat4ToPxMat44(const glm::mat4& glmMatrix);
}
