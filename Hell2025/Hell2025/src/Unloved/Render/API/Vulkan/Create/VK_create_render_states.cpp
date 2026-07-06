#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"
#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"

namespace {

    void CreateVisibilityRenderState() {
        VulkanRenderState& state = VulkanResourceManager::CreateRenderState("Visibility");

        VulkanRenderTargetInfo& visibility = state.AddColorTarget("GBufferRE.Visibility");
        visibility.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        visibility.clearValue.color.uint32[0] = 0;
        visibility.clearValue.color.uint32[1] = 0;

        VulkanRenderTargetInfo& depth = state.SetDepthTarget("GBufferRE.Depth");
        depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth.clearValue.depthStencil.depth = 0.0f;
        depth.clearValue.depthStencil.stencil = 0;

        state.rasterizer.depthTestEnabled = true;
        state.rasterizer.depthWriteEnabled = true;
        state.rasterizer.depthCompareOp = VK_COMPARE_OP_GREATER;
        state.rasterizer.cullFaceEnabled = true;
        state.rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        state.rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

        state.rasterizer.stencilTestEnabled = true;
        state.rasterizer.stencilCompareOp = VK_COMPARE_OP_ALWAYS;
        state.rasterizer.stencilPassOp = VK_STENCIL_OP_REPLACE;
        state.rasterizer.stencilReadMask = 0xff;
        state.rasterizer.stencilWriteMask = 0xff;
    }

    void CreateVisibilityAlphaDiscardRenderState() {
        VulkanRenderState& state = VulkanResourceManager::CreateRenderState("VisibilityAlphaDiscard");

        VulkanRenderTargetInfo& visibility = state.AddColorTarget("GBufferRE.Visibility");
        visibility.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;

        VulkanRenderTargetInfo& depth = state.SetDepthTarget("GBufferRE.Depth");
        depth.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;

        state.rasterizer.depthTestEnabled = true;
        state.rasterizer.depthWriteEnabled = true;
        state.rasterizer.depthCompareOp = VK_COMPARE_OP_GREATER;
        state.rasterizer.cullFaceEnabled = false;
        state.rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

        state.rasterizer.stencilTestEnabled = true;
        state.rasterizer.stencilCompareOp = VK_COMPARE_OP_ALWAYS;
        state.rasterizer.stencilPassOp = VK_STENCIL_OP_REPLACE;
        state.rasterizer.stencilReadMask = 0xff;
        state.rasterizer.stencilWriteMask = 0xff;
    }

    void CreateMaterialResolveRenderState() {
        VulkanRenderState& state = VulkanResourceManager::CreateRenderState("MaterialResolve");

        VulkanRenderTargetInfo& baseColor = state.AddColorTarget("BaseColorMetallic");
        baseColor.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        baseColor.clearValue.color.float32[3] = 1.0f;

        VulkanRenderTargetInfo& normal = state.AddColorTarget("NormalXYRoughnessMisc");
        normal.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        normal.clearValue.color.float32[3] = 1.0f;

        VulkanRenderTargetInfo& velocity = state.AddColorTarget("VelocityXYOcclusionSubSurface");
        velocity.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        velocity.clearValue.color.float32[3] = 1.0f;

        VulkanRenderTargetInfo& depth = state.SetDepthTarget("GBufferRE.Depth");
        depth.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;

        state.rasterizer.depthTestEnabled = false;
        state.rasterizer.depthWriteEnabled = false;
        state.rasterizer.cullFaceEnabled = false;

        state.rasterizer.stencilTestEnabled = true;
        state.rasterizer.stencilCompareOp = VK_COMPARE_OP_EQUAL;
        state.rasterizer.stencilFailOp = VK_STENCIL_OP_KEEP;
        state.rasterizer.stencilDepthFailOp = VK_STENCIL_OP_KEEP;
        state.rasterizer.stencilPassOp = VK_STENCIL_OP_KEEP;
        state.rasterizer.stencilReadMask = 0xff;
        state.rasterizer.stencilWriteMask = 0x00;
    }
}

namespace VulkanRenderer {

    void CreateRenderStates() {
        CreateVisibilityRenderState();
        CreateVisibilityAlphaDiscardRenderState();
        CreateMaterialResolveRenderState();
    }
}
