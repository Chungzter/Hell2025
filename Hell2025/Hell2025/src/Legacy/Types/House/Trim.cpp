#include "Trim.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Renderer/RenderDataManager.h"
#include "Util.h"
#include <Game/UniqueID.h>

#include <iostream> // TODO clean up logging

void Trim::Init(Transform transform, const std::string& modelName, const std::string& materialName) {
    m_transform = transform;
    m_objectId = UniqueID::GetNextObjectId(ObjectType::TRIM);

    Model* model = Hell::ResourceManager::GetModelByName(modelName);
    m_materialIndex = Hell::ResourceManager::GetMaterialIndexByName(materialName);
    Material* material = Hell::ResourceManager::GetMaterialByIndex(m_materialIndex);

    if (!model || !material) {
        std::cout << "Trim::Init() failed: model name '" << modelName << "' not found\n";
        return;
    }

    m_renderItem.modelMatrix = transform.to_mat4();
    m_renderItem.inverseModelMatrix = glm::inverse(m_renderItem.modelMatrix);
    m_renderItem.meshId = model->GetMeshIndices()[0];
    m_renderItem.baseColorTextureIndex = material->m_basecolor;
    m_renderItem.rmaTextureIndex = material->m_rma;
    m_renderItem.normalMapTextureIndex = material->m_normal;
    Util::UpdateRenderItemAABB(m_renderItem);
    Util::PackUint64(m_objectId, m_renderItem.objectIdLowerBit, m_renderItem.objectIdUpperBit);
}

void Trim::SubmitRenderItem() {
    RenderDataManager::SubmitRenderItem(m_renderItem);
}
