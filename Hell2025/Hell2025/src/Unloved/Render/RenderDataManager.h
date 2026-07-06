#pragma once
#include "Unloved/Common/Types.h"
#include "Unloved/Objects/Renderables/AnimatedMeshNodes.h"
#include "Unloved/Objects/Renderables/MeshNodes.h"
#include "Unloved/Objects/Renderables/AnimatedGameObject.h"
#include "Unloved/Render/DrawCommandSets.h"
#include "Unloved/Render/RendererTypes.h"
#include "Unloved/Systems/Bullets/Bullet.h"

#include <vector>

struct DecalPaintingInfo {
    glm::vec3 rayOrigin;
    glm::vec3 rayDirection;
    int textureArrayIndex = 0;
};

namespace Unloved::RenderDataManager {
    using namespace Unloved;

    void BeginFrame();
    void Update();
    void UpdateDrawCommandsUI();

    const std::vector<DrawIndexedIndirectCommand>& GetDrawCommandsUI();

    inline std::vector<glm::mat4> skinningTransforms;

    int EncodeBaseInstance(int playerIndex, int instanceOffset);
    void DecodeBaseInstance(int baseInstance, int& playerIndex, int& instanceOffset);

    // Submissions
    void SubmitAnimatedMeshNodes(const AnimatedMeshNodes& animatedMeshNodes);
    void SubmitMeshNodes(const MeshNodes& meshNodes);

    void SubmitRenderItem(const RenderItem& renderItem);
    void SubmitRenderItems(const std::vector<RenderItem>& renderItems);

    // House submissions
    void SubmitRenderItemProcedural(const RenderItem& renderItem);

    void SubmitDecalPaintingInfo(DecalPaintingInfo decalPaintingInfo);

    const std::vector<SkinningJob>& GetSkinningJobs();
    const std::vector<RayQuerySkinnedGroup>& GetRayQuerySkinnedGroups();

    const RendererData& GetRendererData();
    const std::vector<glm::mat4>& GetOceanPatchTransforms();
    const std::vector<glm::mat4>& GetSkinningTransforms();
    const std::vector<GPULight>& GetGPULights();
    const std::vector<DecalPaintingInfo>& GetDecalPaintingInfo();
    const std::vector<RenderItem>& GetInstanceData();
    const std::vector<RenderItem>& GetCombinedSkinnedRenderItems();
    uint32_t GetRequiredSkinnedVertexCount();

    const std::vector<RenderItem>& GetRenderItems();
    const std::vector<RenderItem>& GetRenderItemsAlphaDiscard();
    const std::vector<RenderItem>& GetRenderItemsBlended();
    const std::vector<RenderItem>& GetRenderItemsGlass();
    const std::vector<RenderItem>& GetRenderItemsHair();
    const std::vector<RenderItem>& GetRenderItemsMirror();
    const std::vector<RenderItem>& GetRenderItemsOutline();
    const std::vector<RenderItem>& GetRenderItemsPlastic();
    const std::vector<RenderItem>& GetRenderItemsProcedural();
    const std::vector<RenderItem>& GetRenderItemsStainedGlass();
    const std::vector<RenderItem>& GetRenderItemsToiletWater();
    const std::vector<RenderItem>& GetRenderItemsPointLightShadows();

    const std::vector<RenderItem>& GetSkinnedRenderItemsAlphaDiscard();
    const std::vector<RenderItem>& GetSkinnedRenderItemsBlended();
    const std::vector<RenderItem>& GetSkinnedRenderItemsDefault();
    const std::vector<RenderItem>& GetSkinnedRenderItemsHair();

    const std::vector<BloodDecalInstanceData>& GetBloodScreenSpaceDecalInstanceData();
    const std::vector<ViewportData>& GetViewportData();
    const DrawCommandsSet& GetDrawInfoSet();
    const FlashLightShadowMapDrawInfo& GetFlashLightShadowMapDrawInfo();

    const std::vector<RenderItem>& GetNonDeformingSkinnedMeshRenderItems();
    const std::vector<RenderItem>& GetNonDeformingSkinnedMeshRenderItemsAlphaDiscard();
    const std::vector<RenderItem>& GetNonDeformingSkinnedMeshRenderItemsDepthPeeledTransparent();
}
