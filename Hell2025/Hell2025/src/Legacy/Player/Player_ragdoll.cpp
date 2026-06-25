#include "Player.h"
#include "Hell/Logging.h"

void Player::InitRagdoll() {
    AnimatedGameObject* characterModel = GetCharacterModelAnimatedGameObject();
    if (!characterModel) return;

    const glm::vec3 spawnPosition = GetFootPosition();
    const glm::vec3 spawnRotation = glm::vec3(0.0f, m_camera.GetEulerRotation().y + HELL_PI, 0.0f);

    PhysicsFilterData filterData;
    filterData.raycastGroup = RaycastGroup::RAYCAST_ENABLED;
    filterData.collisionGroup = CollisionGroup::RAGDOLL_PLAYER;
    filterData.collidesWith = CollisionGroup::ENVIROMENT_OBSTACLE;

    const uint64_t RagdollId = Hell::Physics::SpawnRagdoll(spawnPosition, spawnRotation, "UnisexGuy", m_playerId, filterData);

    characterModel->SetRagdollId(RagdollId);
}
