#pragma once
#include <Game/Types.h>
#include "Types/Renderer/AnimatedMeshNodes.h"
#include "Types/Renderer/MeshNodes.h"
#include "Types/Game/AnimatedGameObject.h"
#include "Types/Game/Bullet.h"
#include <vector>

struct DecalPaintingInfo {
    glm::vec3 rayOrigin;
    glm::vec3 rayDirection;
    int textureArrayIndex = 0;
};

namespace RenderDataManager {
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

    void SubmitGPULightHighRes(uint32_t lightIndex);
    //void SubmitSkinnedRenderItems(const std::vector<RenderItem>& renderItems);


    // House submissions
    void SubmitRenderItemProcedural(const RenderItem& renderItem);

    void SubmitDecalPaintingInfo(DecalPaintingInfo decalPaintingInfo);

    const RendererData& GetRendererData();
    const std::vector<glm::mat4>& GetOceanPatchTransforms();
    const std::vector<glm::mat4>& GetSkinningTransforms();
    const std::vector<GPULight>& GetGPULightsHighRes();
    const std::vector<DecalPaintingInfo>& GetDecalPaintingInfo();
    //const std::vector<HouseRenderItem>& GetHouseRenderItems();
    //const std::vector<HouseRenderItem>& GetHouseOutlineRenderItems();
    const std::vector<RenderItem>& GetInstanceData();
    const std::vector<RenderItem>& GetCombinedSkinnedRenderItems();

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

    const std::vector<RenderItem>& GetSkinnedRenderItemsAlphaDiscard();
    const std::vector<RenderItem>& GetSkinnedRenderItemsBlended();
    const std::vector<RenderItem>& GetSkinnedRenderItemsDefault();
    const std::vector<RenderItem>& GetSkinnedRenderItemsHair();

    const std::vector<BloodDecalInstanceData>& GetScreenSpaceBloodDecalInstanceData();
    const std::vector<ViewportData>& GetViewportData();
    const DrawCommandsSet& GetDrawInfoSet();
    const FlashLightShadowMapDrawInfo& GetFlashLightShadowMapDrawInfo();

    const std::vector<RenderItem>& GetNonDeformingSkinnedMeshRenderItems();
    const std::vector<RenderItem>& GetNonDeformingSkinnedMeshRenderItemsDepthPeeledTransparent();
}