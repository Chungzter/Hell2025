#include "SpawnPoint.h"

#include "Hell/Math/AABB.h"
#include "Hell/Math/Transform.h"
#include "Hell/Physics/Physics.h"

#include "Unloved/Common/Constants.h"
#include "Unloved/Debug/DebugDraw.h"
#include "Unloved/ObjectId.h"

namespace Unloved {

SpawnPoint::SpawnPoint(uint64_t id, const SpawnPointCreateInfo& createInfo, const SpawnOffset& spawnOffset) {
    m_objectId = id;
    m_createInfo = createInfo;
    m_createInfo.position += spawnOffset.translation;
    m_createInfo.camEuler.y += spawnOffset.yRotation;
}

void SpawnPoint::CleanUp() {
    // Nothing as of yet
}

void SpawnPoint::DrawDebugCube() {
    glm::vec3 aabbMin = glm::vec3(-0.5f);
    glm::vec3 aabbMax = glm::vec3(0.5f);

    Hell::Transform transform;
    transform.position = GetPosition();

    AABB aabb = AABB(aabbMin, aabbMax);

    DebugDraw::DrawAABB(aabb, OUTLINE_COLOR, transform.to_mat4());
}
}
