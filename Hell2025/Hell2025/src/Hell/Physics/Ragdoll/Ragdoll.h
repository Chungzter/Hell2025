#pragma once
#include "RagdollData.h"
#include "RagdollTypes.h"

#include "Hell/Math/AABB.h"
#include "Hell/Physics/PhysicsTypes.h"
#include "Hell/Transform.h"

#pragma warning(push, 0)
#include <physx/PxRigidDynamic.h>
#include <physx/extensions/PxD6Joint.h>
#pragma warning(pop)

#include <cstdint>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <string>
#include <vector>

struct Ragdoll {
    void Init(const glm::vec3& spawnPosition, const glm::vec3& spawnEulerRotation, const std::string& ragdollName, uint64_t ragdollId, uint64_t parentObjectId, PhysicsFilterData filterData);
    void CleanUp();
    void Update();
    void DisableSimulation();
    void EnableSimulation();
    void SetToInitialPose();
    void MarkForRemoval();
    void AddForce(uint64_t physicsId, const glm::vec3& force);
    const std::string& GetBoneNameByPhysicsId(uint64_t physicsId) const;

    bool IsInMotion();
    bool IsMarkedForRemoval() const;
    AABB GetWorldSpaceAABB();
    void GetWorldSpaceAABBs(std::vector<AABB>& aabbs);
    void UpdateWorldSpaceAABBs(float changeThreshold);
    const std::vector<AABB>& GetWorldSpaceAABBs() const          { return m_worldSpaceAABBs; }
    glm::mat4 GetRigidWorldTransform(const std::string& boneName) const;
    uint64_t GetRagdollId()                     { return m_ragdollId; }
    const std::string& GetRagdollName() const   { return m_ragdollName; }
    uint32_t GetMarkerDebugMeshIdByRigidIndex(uint32_t index) const;
    glm::vec3 GetMarkerColorByRigidIndex(uint32_t index) const;
    glm::mat4 GetModelMatrixByRigidIndex(uint32_t index) const;

    std::vector<std::string> m_markerBoneNames;
    std::vector<physx::PxRigidDynamic*> m_pxRigidDynamics;

    bool IsDirty() const { return m_dirty; }

private:
    void AddMarkerMeshData(RagdollMarker& marker, RagdollSolver& solver);

    std::vector<physx::PxD6Joint*> m_pxD6Joints;
    std::vector<uint32_t> m_markerDebugMeshIds;
    std::vector<glm::vec3> m_markerColors;
    std::vector<AABB> m_worldSpaceAABBs;
    std::string m_ragdollName;
    Hell::Transform m_spawnTransform;
    uint64_t m_ragdollId;
    float m_scale = 1.0f;
    bool m_simulationEnabled = false;
    bool m_renderingEnabled = true;
    bool m_markedForRemoval = false;
    bool m_dirty = false;
};
