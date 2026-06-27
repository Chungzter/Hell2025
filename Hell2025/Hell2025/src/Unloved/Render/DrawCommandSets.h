#pragma once

#include "Hell/Render/DrawCommandTypes.h"

#include "Unloved/Render/RendererConstants.h"
#include "Unloved/Render/RendererTypes.h"

#include <vector>

struct DrawCommandsSet {
    std::vector<RenderItem> glass[4];

    std::vector<DrawIndexedIndirectCommand> alphaDiscard[4];
    std::vector<DrawIndexedIndirectCommand> blended[4];
    std::vector<DrawIndexedIndirectCommand> hair[4];
    std::vector<DrawIndexedIndirectCommand> standard[4];
    std::vector<DrawIndexedIndirectCommand> procedural[4];
    std::vector<DrawIndexedIndirectCommand> mirrorRenderItems[4];
    std::vector<DrawIndexedIndirectCommand> plastic[4];
    std::vector<DrawIndexedIndirectCommand> emissive[4];

    std::vector<DrawIndexedIndirectCommand> skinnedAlphaDiscard[4];
    std::vector<DrawIndexedIndirectCommand> skinnedBlended[4];
    std::vector<DrawIndexedIndirectCommand> skinnedHair[4];
    std::vector<DrawIndexedIndirectCommand> skinnedStandard[4];

    std::vector<DrawIndexedIndirectCommand> skinnedNonDeformingAlphaDiscard[4];
    std::vector<DrawIndexedIndirectCommand> skinnedNonDeformingBlended[4];
    std::vector<DrawIndexedIndirectCommand> skinnedNonDeformingHair[4];
    std::vector<DrawIndexedIndirectCommand> skinnedNonDeformingStandard[4];

    std::vector<DrawIndexedIndirectCommand> shadowMapHiRes[SHADOWMAP_HI_RES_COUNT][6];
    std::vector<DrawIndexedIndirectCommand> moonLightCascades[4][SHADOW_CASCADE_COUNT];
};

struct FlashLightShadowMapDrawInfo {
    std::vector<DrawIndexedIndirectCommand> flashlightShadowMapGeometry[4];
    std::vector<uint32_t> heightMapChunkIndices[4];
};