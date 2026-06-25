#include "RigidDynamic.h"
#include "Hell/Physics/Physics.h"

void RigidDynamic::Update(float deltaTime) {
    if (!m_pxRigidDynamic) return;

    const PxBounds3 bounds = m_pxRigidDynamic->getWorldBounds();
    const glm::vec3 aabbMin(bounds.minimum.x, bounds.minimum.y, bounds.minimum.z);
    const glm::vec3 aabbMax(bounds.maximum.x, bounds.maximum.y, bounds.maximum.z);
    m_aabb = AABB(aabbMin, aabbMax);

    const PxTransform currentGlobalPose = m_pxRigidDynamic->getGlobalPose();

    m_isDirty = m_lifeTime < 0.1f || !Hell::Physics::PxTransformNearlyEqual(m_previousGlobalPose, currentGlobalPose);
    m_worldTransform = Hell::Physics::PxMat44ToGlmMat4(currentGlobalPose);
    m_previousGlobalPose = currentGlobalPose;

    m_lifeTime += deltaTime;
}

void RigidDynamic::MarkForRemoval() {
    m_markedForRemoval = true;
}

void RigidDynamic::SetPxRigidDynamic(PxRigidDynamic* rigidDynamic) {
    m_pxRigidDynamic = rigidDynamic; 
}

//void RigidDynamic::SetPxShape(PxShape* shape) {
//    m_pxShape = shape; 
//}

void RigidDynamic::SetPxShapes(const std::vector<PxShape*>& pxShapes) {
    m_pxShapes = pxShapes;
}

void RigidDynamic::SetFilterData(PhysicsFilterData filterData) {
    PxFilterData pxFilterData;
    pxFilterData.word0 = (PxU32)filterData.raycastGroup;
    pxFilterData.word1 = (PxU32)filterData.collisionGroup;
    pxFilterData.word2 = (PxU32)filterData.collidesWith;

    for (PxShape* pxShape : m_pxShapes) {
        pxShape->setQueryFilterData(pxFilterData);       // ray casts
        pxShape->setSimulationFilterData(pxFilterData);  // collisions
    }
}

void RigidDynamic::AddForce(glm::vec3 force) {
    if (!m_pxRigidDynamic) return;

    PxVec3 pxForce = Hell::Physics::GlmVec3toPxVec3(force);
    m_pxRigidDynamic->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, false);
    m_pxRigidDynamic->addForce(pxForce);
}

void RigidDynamic::SetGlobalPose(const glm::mat4& globalPoseMatrix) {
    if (!m_pxRigidDynamic) return;

    PxMat44 pxMatrix = Hell::Physics::GlmMat4ToPxMat44(globalPoseMatrix);
    PxTransform pxTransform = PxTransform(pxMatrix);
    m_pxRigidDynamic->setGlobalPose(pxTransform);
}

void RigidDynamic::SetKinematicTarget(const glm::mat4& globalPoseMatrix) {
    if (!m_pxRigidDynamic) return;
    if (m_pxRigidDynamic->getScene() == nullptr) return;
    if (!m_pxRigidDynamic->getRigidBodyFlags().isSet(PxRigidBodyFlag::eKINEMATIC)) return;

    PxTransform targetPose(PxMat44(Hell::Physics::GlmMat4ToPxMat44(globalPoseMatrix)));
    if (!targetPose.isValid()) return;

    m_pxRigidDynamic->setKinematicTarget(targetPose);
}

void RigidDynamic::SetUserData(PhysicsUserData physicsUserData) {
    if (!m_pxRigidDynamic) return;

    m_pxRigidDynamic->userData = new PhysicsUserData(physicsUserData);
}

bool RigidDynamic::IsKinematic() const {
    if (!m_pxRigidDynamic) return false;

    return m_pxRigidDynamic->getRigidBodyFlags().isSet(physx::PxRigidBodyFlag::eKINEMATIC);
}

glm::mat4 RigidDynamic::GetWorldMatrix() const {
    if (!m_pxRigidDynamic) return glm::mat4(1.0f);

    return Hell::Physics::PxMat44ToGlmMat4(m_pxRigidDynamic->getGlobalPose());
}

float RigidDynamic::GetVolume() {
    float volume = 0.0f;
    for (PxShape* pxShape : m_pxShapes) {
        volume += Hell::Physics::ComputeShapeVolume(pxShape);
    }
    return volume;
}

void RigidDynamic::UpdateMassAndInertia(float density) {
    PxRigidBodyExt::updateMassAndInertia(*m_pxRigidDynamic, density);
}
