#include "Player.h"
#include "Hell/Logging.h"

void Player::InitRagdoll() {
    HELL_LOG_FUNCTION

    AnimatedGameObject* characterModel = GetCharacterModelAnimatedGameObject();
    if (!characterModel) return;

    // Set ragdoll
    characterModel->SetRagdoll("UnisexGuy", 85.0f);

    // Get ragdoll
    Ragdoll* ragdoll = GetRagdoll();
    if (!ragdoll) return;

    // Set ragdoll PhysX filter and user data
    PhysicsFilterData filterData;
    filterData.raycastGroup = RaycastGroup::RAYCAST_ENABLED;
    filterData.collisionGroup = CollisionGroup::RAGDOLL_PLAYER;
    filterData.collidesWith = CollisionGroup(ENVIROMENT_OBSTACLE);

    ragdoll->SetFilterData(filterData);
    ragdoll->SetPhysicsData(characterModel->GetObjectId());
}