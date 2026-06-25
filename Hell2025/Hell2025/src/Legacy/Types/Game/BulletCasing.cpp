#include "BulletCasing.h"
#include "Util.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/Audio.h"
#include "Hell/Physics/Physics.h"
#include "Game/RendereringConstants.h"
#include "Renderer/RenderDataManager.h"

BulletCasing::BulletCasing(BulletCasingCreateInfo createInfo) {
    m_materialIndex = createInfo.materialIndex;

    // Get model
    Model* model = Hell::ResourceManager::GetModelById(createInfo.modelId);
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
    transform.position = createInfo.position;
    transform.rotation = createInfo.rotation;

    PhysicsFilterData filterData;
    filterData.raycastGroup = RaycastGroup::RAYCAST_DISABLED;
    filterData.collisionGroup = CollisionGroup::BULLET_CASING;
    filterData.collidesWith = CollisionGroup::ENVIROMENT_OBSTACLE;

    glm::vec3 force = createInfo.force;
    glm::vec3 torque = glm::vec3(Util::RandomFloat(-10.0f, 10.0f), Util::RandomFloat(-10.0f, 10.0f), Util::RandomFloat(-10.0f, 10.0f));

    m_rigidDynamicId = Hell::Physics::CreateRigidDynamicFromBoxExtents(transform, mesh->extents, createInfo.mass, filterData, force, torque);
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
    renderItem.baseColorTextureIndex = material->m_basecolor;
    renderItem.rmaTextureIndex = material->m_rma;
    renderItem.normalMapTextureIndex = material->m_normal;
    renderItem.meshId = GetMeshId();
    renderItem.shadowBit = SHADOW_BIT_NONE;

    Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(renderItem.meshId);
    if (mesh) {
        renderItem.baseVertex = mesh->baseVertex;
        renderItem.baseIndex = mesh->baseIndex;
    }

    Util::UpdateRenderItemAABB(renderItem);
    RenderDataManager::SubmitRenderItem(renderItem);
}
