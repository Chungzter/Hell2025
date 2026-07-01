#include "Player.h"

#include "Unloved/Objects/Traversal/Ladder.h"
#include "Unloved/World/World.h"

#include <vector>

namespace Unloved {

void Player::UpdateLadderIds() {
    m_ladderIdOverlapIndexFeet = 0;
    m_ladderIdOverlapIndexEyes = 0;

    const std::vector<uint64_t> ladderIds = Unloved::World::GetLadders().ids();

    for (uint64_t ladderId : ladderIds) {
        Ladder* ladder = Unloved::World::GetLadderByObjectId(ladderId);
        if (!ladder) {
            continue;
        }

        const float sphereRadius = 0.25f;
        const AABB& overlapHitBox = ladder->GetOverlapHitBoxAABB();

        if (overlapHitBox.IntersectsSphere(GetFootPosition(), sphereRadius)) {
            m_ladderIdOverlapIndexFeet = ladder->GetObjectId();
        }
        if (overlapHitBox.IntersectsSphere(GetCameraPosition(), sphereRadius)) {
            m_ladderIdOverlapIndexEyes = ladder->GetObjectId();
        }
    }
}

bool Player::IsOverlappingLadder() {
    return (m_ladderIdOverlapIndexFeet != 0 && m_ladderIdOverlapIndexEyes != 0);
}

void Player::UpdateLadderMovement(float deltaTime) {
    float ladderClimpingSpeed = 3.5f;

    if (!PressingWalkForward() &&
        !PressingWalkBackward() &&
        !PressingWalkLeft() &&
        !PressingWalkRight()) {
        return;
    }

    if (m_ladderIdOverlapIndexEyes != 0 && IsMoving() && !IsCrouching()) {
        glm::vec3 ladderMovementDisplacement = glm::vec3(0.0f, 1.0f, 0.0f) * ladderClimpingSpeed * deltaTime;
        Hell::Physics::MoveCharacterController(m_characterControllerId, ladderMovementDisplacement);
    }
}

} // namespace Unloved
