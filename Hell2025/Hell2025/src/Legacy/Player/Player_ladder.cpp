#include "Player.h"
#include "World/LegacyWorld.h"

void Player::UpdateLadderIds() {
    m_ladderIdOverlapIndexFeet = 0;
    m_ladderIdOverlapIndexEyes = 0;

    for (Ladder& ladder : LegacyWorld::GetLadders()) {
        float sphereRadius = 0.25f;

        if (ladder.GetOverlapHitBoxAABB().IntersectsSphere(GetFootPosition(), sphereRadius)) {
            m_ladderIdOverlapIndexFeet = ladder.GetObjectId();
        }
        if (ladder.GetOverlapHitBoxAABB().IntersectsSphere(GetCameraPosition(), sphereRadius)) {
            m_ladderIdOverlapIndexEyes = ladder.GetObjectId();
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