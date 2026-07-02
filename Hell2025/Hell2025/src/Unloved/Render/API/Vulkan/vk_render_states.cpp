#include "vk_render_states.h"

#include <unordered_map>

namespace {
    std::unordered_map<std::string, VulkanRenderer::VulkanRenderState> g_renderStates;

    void CreateVisibilityRenderState() {
        VulkanRenderer::VulkanRenderState& state = VulkanRenderer::CreateRenderState("Visibility");

        VulkanRenderer::RenderTargetInfo& visibility = state.AddColorTarget("GBufferRE.Visibility");
        visibility.format = VK_FORMAT_R32G32_UINT;
        visibility.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        visibility.clearValue.color.uint32[0] = 0;
        visibility.clearValue.color.uint32[1] = 0;

        VulkanRenderer::RenderTargetInfo& depth = state.SetDepthTarget("GBufferRE.Depth");
        depth.format = VK_FORMAT_D32_SFLOAT;
        depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth.clearValue.depthStencil.depth = 0.0f;

        state.rasterizer.depthTestEnabled = true;
        state.rasterizer.depthWriteEnabled = true;
        state.rasterizer.depthCompareOp = VK_COMPARE_OP_GREATER;
        state.rasterizer.cullFaceEnabled = false;
    }

    void CreateVisibilityAlphaDiscardRenderState() {
        VulkanRenderer::VulkanRenderState& state = VulkanRenderer::CreateRenderState("VisibilityAlphaDiscard");

        VulkanRenderer::RenderTargetInfo& visibility = state.AddColorTarget("GBufferRE.Visibility");
        visibility.format = VK_FORMAT_R32G32_UINT;
        visibility.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;

        VulkanRenderer::RenderTargetInfo& depth = state.SetDepthTarget("GBufferRE.Depth");
        depth.format = VK_FORMAT_D32_SFLOAT;
        depth.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;

        state.rasterizer.depthTestEnabled = true;
        state.rasterizer.depthWriteEnabled = true;
        state.rasterizer.depthCompareOp = VK_COMPARE_OP_GREATER;
        state.rasterizer.cullFaceEnabled = false;
    }
}

namespace VulkanRenderer {

    void CreateRenderStates() {
        g_renderStates.clear();

        CreateVisibilityRenderState();
        CreateVisibilityAlphaDiscardRenderState();
    }

    VulkanRenderState& CreateRenderState(const std::string& name) {
        VulkanRenderState& state = g_renderStates[name];
        state = VulkanRenderState();
        return state;
    }

    VulkanRenderState* GetRenderState(const std::string& name) {
        auto it = g_renderStates.find(name);
        if (it == g_renderStates.end()) return nullptr;
        return &it->second;
    }
}
