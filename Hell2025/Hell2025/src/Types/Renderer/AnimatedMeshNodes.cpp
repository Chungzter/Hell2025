#include "AnimatedMeshNodes.h"
#include "AssetManagement/AssetManager.h"
#include "Util/Util.h"

#include <Hell/Logging.h>

void AnimatedMeshNodes::Init(uint64_t parentId, const std::string& modelName, const std::vector<AnimatedMeshNodeCreateInfo>& createInfoSet) {

}

void AnimatedMeshNodes::SetSkinnedModel(uint64_t parentId, std::string name) {
    m_parentId = parentId;

    SkinnedModel* ptr = AssetManager::GetSkinnedModelByName(name);
    if (ptr) {
        //std::cout << "SetSkinnedModel() " << name << " mesh count: " << m_skinnedModel->GetMeshCount() << "\n";

        m_skinnedModel = ptr;
        m_nodes.clear();
        m_woundMaskTextureIndices.resize(m_skinnedModel->GetMeshCount());

        int meshCount = m_skinnedModel->GetMeshCount();

        for (int i = 0; i < meshCount; i++) {
            SkinnedMesh* skinnedMesh = AssetManager::GetSkinnedMeshByIndex(m_skinnedModel->GetMeshIndices()[i]);
            AnimatedMeshNode& node = m_nodes.emplace_back();
            node.meshIndex = m_skinnedModel->GetMeshIndices()[i];
            node.meshName = skinnedMesh->name;
            node.deforming = skinnedMesh->requiresSkinning;
            node.baseSkinningVertex = skinnedMesh->baseVertexWeight;
            m_woundMaskTextureIndices[i] = -1;
        }
    }
    else {
        std::cout << "Could not SetSkinnedModel(name) with name: \"" << name << "\", it does not exist\n";
    }
}

void AnimatedMeshNodes::UpdateRenderItems(const glm::mat4& modelMatrix, const std::vector<glm::mat4>& boneSkinningMatrices) {
    m_deformingRenderItems.clear();
    m_nonDeformingRenderItems.clear();
    m_nonDeformingRenderItemsDepthPeeledTransparent.clear();

    if (!m_renderingEnabled) return;

    for (int i = 0; i < m_nodes.size(); i++) {
        if (m_nodes[i].blendingMode == BlendingMode::DO_NOT_RENDER) continue;

        RenderItem& renderItem = m_nodes[i].renderItem;
        SkinnedMesh* mesh = AssetManager::GetSkinnedMeshByIndex(m_nodes[i].meshIndex);

        Material* material = AssetManager::GetMaterialByIndex(m_nodes[i].materialIndex);
        renderItem.baseColorTextureIndex = material->m_basecolor;
        renderItem.rmaTextureIndex = material->m_rma;
        renderItem.normalMapTextureIndex = material->m_normal;
        renderItem.hairMapTextureIndex = material->m_hairMaps;
        renderItem.opacityTextureIndex = material->m_opacity;

        renderItem.prevModelMatrix = renderItem.modelMatrix; // TODO: write logic for on the first frame where this is identity
        renderItem.modelMatrix = modelMatrix;
        renderItem.inverseModelMatrix = glm::inverse(renderItem.modelMatrix);
        renderItem.meshIndex = m_skinnedModel->GetMeshIndices()[i];
        renderItem.ignoredViewportIndex = m_ignoredViewportIndex;
        renderItem.exclusiveViewportIndex = m_exclusiveViewportIndex;
        renderItem.baseVertexWeight = mesh->baseVertexWeight;
        //renderItem.furLength = m_nodes[i].furLength;
        //renderItem.furUVScale = m_nodes[i].furUVScale;
        //renderItem.furShellDistanceAttenuation = m_nodes[i].furShellDistanceAttenuation;
        renderItem.woundMaskTexutreIndex = m_woundMaskTextureIndices[i];
        renderItem.blockScreenSpaceBloodDecals = (int)true;
        renderItem.baseVertex = mesh->baseVertexGlobal;
        renderItem.baseIndex = mesh->baseIndex;

        Util::PackUint64(m_parentId, renderItem.objectIdLowerBit, renderItem.objectIdUpperBit);

        // Additional textures (hair)
		if (m_nodes[i].blendingMode == BlendingMode::HAIR) {
			renderItem.additionalTextureIndex0 = material->m_hairFlowMap;
            renderItem.additionalTextureIndex1 = material->m_hairIdMap;
            renderItem.additionalTextureIndex2 = material->m_hairRootMap;
            renderItem.additionalTextureIndex3 = material->m_hairBlendMap;
		}
		// Additional textures (wound mask)
        else if (m_woundMaskTextureIndices[i] != -1) {
            Material* wouldMaterial = AssetManager::GetMaterialByIndex(m_nodes[i].woundMaterialIndex);
            renderItem.additionalTextureIndex0 = wouldMaterial->m_basecolor;
            renderItem.additionalTextureIndex1 = wouldMaterial->m_normal;
            renderItem.additionalTextureIndex2 = wouldMaterial->m_rma;
        }

        // Put it where it belongs
        if (mesh->requiresSkinning) {
            renderItem.prevModelMatrix = renderItem.modelMatrix; // Hack because you are compute skinning and can't rely on shit here. FIGURE THIS OUT
            m_deformingRenderItems.push_back(renderItem);
        }
        else {
            // Update the model matrix to include the animated bone transform
            int boneIndex = mesh->nonDeformingBoneIndex;

            if (boneIndex >= 0 && boneIndex < boneSkinningMatrices.size()) {
                renderItem.prevModelMatrix = renderItem.modelMatrix * boneSkinningMatrices[boneIndex]; // Hack because you are compute skinning and can't rely on shit here. FIGURE THIS OUT
                renderItem.modelMatrix = modelMatrix * boneSkinningMatrices[boneIndex];
                renderItem.inverseModelMatrix = glm::inverse(renderItem.modelMatrix);

                if (mesh->name == "P90_Magazine") {
                    m_nonDeformingRenderItemsDepthPeeledTransparent.push_back(renderItem);
                }
                else {
                    m_nonDeformingRenderItems.push_back(renderItem);
                }
            }
            else {
                Logging::Error() << "AnimatedMeshNodes::UpdateRenderItems(..) wants to access boneSkinningMatrices[" << boneIndex << "] but size is " << boneSkinningMatrices.size() << "\n";
            }
        }
    }
}

void AnimatedMeshNodes::SetMeshWoundMaskTextureIndex(const std::string& meshName, int32_t woundMaskTextureIndex) {
    std::vector<uint32_t>& meshIndices = m_skinnedModel->GetMeshIndices();

    for (int i = 0; i < meshIndices.size(); i++) {
        uint32_t meshIndex = meshIndices[i];
        SkinnedMesh* skinnedMesh = AssetManager::GetSkinnedMeshByIndex(meshIndex);
        if (skinnedMesh && skinnedMesh->name == meshName) {
            m_woundMaskTextureIndices[i] = woundMaskTextureIndex;
            return;
        }
    }
}

void AnimatedMeshNodes::SetBlendingModeByMeshName(const std::string& meshName, BlendingMode blendingMode) {
    for (AnimatedMeshNode& node : m_nodes) {
        if (node.meshName == meshName) {
            node.blendingMode = blendingMode;
        }
    }
}

void AnimatedMeshNodes::SetMeshMaterialByMeshName(const std::string& meshName, const std::string& materialName, BlendingMode blendingMode) {
    int materialIndex = AssetManager::GetMaterialIndexByName(materialName);

    if (materialIndex == -1) {
        Logging::Error() << "AnimatedMeshNodes::SetMeshMaterialByMeshName(..) failed because '" << materialName << "' was not found\n";
        return;
    }
    
    for (AnimatedMeshNode& node : m_nodes) {
        if (node.meshName == meshName) {
            node.materialIndex = materialIndex;
            node.blendingMode = blendingMode;
        }
    }
}

void AnimatedMeshNodes::SetMeshFurLength(const std::string& meshName, float furLength) {
    for (AnimatedMeshNode& node : m_nodes) {
        if (node.meshName == meshName) {
            node.furLength = furLength;
        }
    }
}

void AnimatedMeshNodes::SetMeshFurUVScale(const std::string& meshName, float uvScale) {
    for (AnimatedMeshNode& node : m_nodes) {
        if (node.meshName == meshName) {
            node.furUVScale = uvScale;
        }
    }
}

void AnimatedMeshNodes::SetMeshFurShellDistanceAttenuation(const std::string& meshName, float furShellDistanceAttenuation) {
    for (AnimatedMeshNode& node : m_nodes) {
        if (node.meshName == meshName) {
            node.furShellDistanceAttenuation = furShellDistanceAttenuation;
        }
    }
}


void AnimatedMeshNodes::SetMeshMaterialByMeshIndex(int meshIndex, const std::string& materialName) {
    if (meshIndex >= 0 && meshIndex < m_nodes.size()) {
        m_nodes[meshIndex].materialIndex = AssetManager::GetMaterialIndexByName(materialName);
    }
}


void AnimatedMeshNodes::SetMeshToRenderAsGlassByMeshIndex(const std::string& meshName) {
    for (AnimatedMeshNode& node : m_nodes) {
        if (node.meshName == meshName) {
            node.renderAsGlass = true;
        }
    }
}

void AnimatedMeshNodes::SetMeshEmissiveColorTextureByMeshName(const std::string& meshName, const std::string& textureName) {
    for (AnimatedMeshNode& node : m_nodes) {
        if (node.meshName == meshName) {
            node.emissiveColorTexutreIndex = AssetManager::GetTextureIndexByName(textureName);
        }
    }
}

void AnimatedMeshNodes::SetMeshWoundMaterialByMeshName(const std::string& meshName, const std::string& textureName) {
    for (AnimatedMeshNode& node : m_nodes) {
        if (node.meshName == meshName) {
            node.woundMaterialIndex = AssetManager::GetMaterialIndexByName(textureName);
        }
    }
}

void AnimatedMeshNodes::SetAllMeshMaterials(const std::string& materialName) {
    for (AnimatedMeshNode& node : m_nodes) {
        node.materialIndex = AssetManager::GetMaterialIndexByName(materialName);
    }
}

void AnimatedMeshNodes::SetAllMeshBlendingModes(BlendingMode blendingMode) {
    for (AnimatedMeshNode& node : m_nodes) {
        node.blendingMode = blendingMode;
    }
}

void AnimatedMeshNodes::SetExclusiveViewportIndex(int index) {
    m_exclusiveViewportIndex = index;
}

void AnimatedMeshNodes::SetIgnoredViewportIndex(int index) {
    m_ignoredViewportIndex = index;
}

void AnimatedMeshNodes::PrintMeshNames() {
    std::cout << m_skinnedModel->GetName() << "\n";
    for (int i = 0; i < m_nodes.size(); i++) {
        std::cout << "-" << i << " " << m_nodes[i].meshName << "\n";
    }
}

void AnimatedMeshNodes::EnableRendering() {
    m_renderingEnabled = true;
}

void AnimatedMeshNodes::DisableRendering() {
    m_renderingEnabled = false;
}