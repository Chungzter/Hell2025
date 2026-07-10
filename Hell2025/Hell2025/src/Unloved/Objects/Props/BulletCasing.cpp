#include "BulletCasing.h"
#include "Unloved/Render/RendererUtil.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/Audio.h"
#include "Hell/Common/Random.h"
#include "Hell/Physics/Physics.h"
#include "Unloved/Render/RendererConstants.h"
#include "Unloved/Render/RenderDataManager.h"

namespace Unloved {

BulletCasing::BulletCasing(uint64_t id, BulletCasingCreateInfo createInfo) {
    m_createInfo = createInfo;
    m_objectId = id;
    m_materialIndex = m_createInfo.materialIndex;

    // Get model
    Model* model = Hell::ResourceManager::GetModelById(m_createInfo.modelId);
    if (!model) {
        std::cout << "BulletCasing(BulletCasingCreateInfo createInfo) failed from invalid model\n";
        return;
    }
    if (model->GetMeshCount() < 1) {
        std::cout << "BulletCasing(BulletCasingCreateInfo createInfo) failed from mesh count 0\n";
    }

    // Get mesh
    m_meshId = model->GetMeshIndices()[0];
    Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(m_meshId);
    if (!mesh) {
        std::cout << "BulletCasing(BulletCasingCreateInfo createInfo) failed from invalid mesh\n";
    }

    Transform transform;
    transform.position = m_createInfo.position;
    transform.rotation = m_createInfo.rotation;

    PhysicsFilterData filterData;
    filterData.raycastGroup = RaycastGroup::RAYCAST_DISABLED;
    filterData.collisionGroup = CollisionGroup::BULLET_CASING;
    filterData.collidesWith = CollisionGroup::ENVIROMENT_OBSTACLE;

    glm::vec3 force = m_createInfo.force;
    glm::vec3 torque = glm::vec3(Hell::Random::Float(-10.0f, 10.0f), Hell::Random::Float(-10.0f, 10.0f), Hell::Random::Float(-10.0f, 10.0f));

    m_rigidDynamicId = Hell::Physics::CreateRigidDynamicFromBoxExtents(transform, mesh->extents, m_createInfo.mass, filterData, force, torque);
}

void BulletCasing::CleanUp() {
    Hell::Physics::MarkRigidDynamicForRemoval(m_rigidDynamicId);
}

const glm::mat4& BulletCasing::GetModelMatrix() {
    return m_modelMatrix;
}

void BulletCasing::Update(float deltaTime) {
    m_lifeTime += deltaTime;

    float maxLifeTime = 5.0f;
    if (m_lifeTime > maxLifeTime) {
        Hell::Physics::MarkRigidDynamicForRemoval(m_rigidDynamicId);
    }

    if (Hell::Physics::RigidDynamicExists(m_rigidDynamicId)) {
        m_modelMatrix = Hell::Physics::GetRigidDynamicWorldMatrix(m_rigidDynamicId);
    }
}

void BulletCasing::SubmitRenderItem() {
    Material* material = Hell::ResourceManager::GetMaterialByIndex(GetMaterialIndex());

    RenderItem renderItem;
    renderItem.modelMatrix = GetModelMatrix();
    renderItem.inverseModelMatrix = inverse(renderItem.modelMatrix);
    renderItem.materialIndex = GetMaterialIndex();
    renderItem.meshId = GetMeshId();
    renderItem.shadowFlags = SHADOW_FLAG_NONE;
    renderItem.vulkanFlags = 0;

    Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(renderItem.meshId);
    if (mesh) {
        renderItem.baseVertex = mesh->baseVertex;
        renderItem.baseIndex = mesh->baseIndex;
    }

    RendererUtil::UpdateRenderItemAABB(renderItem);
    RenderDataManager::SubmitRenderItem(renderItem);
}
}
