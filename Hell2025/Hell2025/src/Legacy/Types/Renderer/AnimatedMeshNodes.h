#pragma once
#include <Game/Constants.h>
#include <Game/CreateInfo.h>
#include <Game/Enums.h>
#include <Game/Types.h>

#include <cstdint>
#include <string>
#include <vector>

#include "Types/Renderer/SkinnedModel.h"

struct AnimatedMeshNodeCreateInfo {
    std::string meshName;
    std::string materialName = UNDEFINED_STRING;
    BlendingMode blendingMode = BlendingMode::DEFAULT;
    bool castShadows = true;
};

struct AnimatedMeshNode {
    std::string meshName;
    int materialIndex = 0;
    int woundMaterialIndex = 0;
    int emissiveColorTexutreIndex = -1;
    bool renderAsGlass = false;
    uint32_t meshId = 0;
    float furLength = 0.0f;
    float furShellDistanceAttenuation = 0.0f;
    float furUVScale = 0.0f;
    bool deforming = true;
    int baseSkinningVertex = -1;
    BlendingMode blendingMode = BlendingMode::DEFAULT;
    RenderItem renderItem;
};

struct AnimatedMeshNodes {
    void Init(uint64_t parentId, const std::string& modelName, const std::vector<AnimatedMeshNodeCreateInfo>& createInfoSet);
    void UpdateRenderItems(const glm::mat4& modelMatrix, const std::vector<glm::mat4>& boneSkinningMatrices);

    void SetSkinnedModel(uint64_t parentId, std::string name); // temp

    void SetMeshWoundMaskTextureIndex(const std::string& meshName, int32_t woundMaskTextureIndex);
    void SetBlendingModeByMeshName(const std::string& meshName, BlendingMode blendingMode);
    void SetMeshMaterialByMeshName(const std::string& meshName, const std::string& materialName, BlendingMode blendingMode = BlendingMode::DEFAULT);
    void SetMeshMaterialByMeshIndex(int meshIndex, const std::string& materialName);
    void SetMeshToRenderAsGlassByMeshIndex(const std::string& materialName);
    void SetMeshFurLength(const std::string& meshName, float furLength);
    void SetMeshFurShellDistanceAttenuation(const std::string& meshName, float furShellDistanceAttenuation);
    void SetMeshFurUVScale(const std::string& meshName, float uvScale);
    void SetMeshEmissiveColorTextureByMeshName(const std::string& meshName, const std::string& textureName);
    void SetMeshWoundMaterialByMeshName(const std::string& meshName, const std::string& textureName);
    void SetAllMeshMaterials(const std::string& materialName);
    void SetAllMeshBlendingModes(BlendingMode blendingMode);
    void SetExclusiveViewportIndex(int index);
    void SetIgnoredViewportIndex(int index);
    void PrintMeshNames();
    void EnableRendering();
    void DisableRendering();

    bool RenderingEnabled() const { return m_renderingEnabled; }

    const uint32_t& GetIgnoredViewportIndex() const       { return m_ignoredViewportIndex; };
    const uint32_t& GetExclusiveViewportIndex() const     { return m_exclusiveViewportIndex; };
    const std::vector<AnimatedMeshNode>& GetNodes() const { return m_nodes; }

    uint64_t m_parentId = 0;
    uint32_t m_ignoredViewportIndex = -1;
    uint32_t m_exclusiveViewportIndex = -1;

    std::vector<int32_t> m_woundMaskTextureIndices;

    std::vector<RenderItem> m_deformingRenderItems;
    std::vector<RenderItem> m_nonDeformingRenderItems;
    std::vector<RenderItem> m_nonDeformingRenderItemsDepthPeeledTransparent;

    SkinnedModel* m_skinnedModel = nullptr;
    bool m_renderingEnabled = true;

private:
    std::vector<AnimatedMeshNode> m_nodes;
};
