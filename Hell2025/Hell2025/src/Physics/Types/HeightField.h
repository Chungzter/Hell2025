#pragma once
#include <Game/Types.h>
#include "Math/AABB.h"
#include <vector>
#include <glm/vec3.hpp>

#pragma warning(push, 0)
#include <physx/PxShape.h>
#include <physx/PxRigidStatic.h>
#include <physx/geometry/PxHeightField.h>
#pragma warning(pop)

using namespace physx;

struct HeightField {
    void Create(vecXZ& worldSpaceOffset, const float* heightValues);
    void ActivatePhsyics();
    void DisablePhsyics();
    void MarkForRemoval();

    const bool HasActivePhysics() const     { return m_activePhysics; }
    const bool IsMarkedForRemoval() const   { return m_markedForRemoval; }
    vecXZ GetWorldSpaceOffset()             { return m_worldSpaceOffset; }
    PxHeightField* GetPxHeightField()       { return m_pxHeightField; }
    PxRigidStatic* GetPxRigidStatic()       { return m_pxRigidStatic; }
    PxShape* GetPxShape()                   { return m_pxShape; }
    const AABB& GetAABB()                   { return m_aabb; }

private:
    vecXZ m_worldSpaceOffset;
    PxHeightField* m_pxHeightField = nullptr;
    PxRigidStatic* m_pxRigidStatic = nullptr;
    PxShape* m_pxShape = nullptr;
    bool m_activePhysics = false;
    bool m_markedForRemoval = false;
    AABB m_aabb;
};