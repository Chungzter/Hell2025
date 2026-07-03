#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"

#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Render/API/Vulkan/Managers/vk_swapchain_manager.h"
#include "Hell/Render/API/Vulkan/Types/vk_pipeline.h"
#include "Hell/Render/API/Vulkan/Types/VK_render_state.h"
#include "Hell/Render/API/Vulkan/Types/vk_shader.h"
#include "Hell/Render/VertexAttributes.h"
#include "Unloved/Render/API/Vulkan/VK_push_constants.h"

namespace {

    void CreateLoadingScreenPipeline() {
        VulkanShader* shader = VulkanResourceManager::GetShader("FullscreenTriangle");
        if (!shader) return;

        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("LoadingScreen");
        pipeline.SetShader(shader);
        pipeline.AddColorAttachmentFormat(VulkanSwapchainManager::GetSwapchainImageFormat());
        pipeline.SetDepthTest(false, false);
        pipeline.SetCullMode(VK_CULL_MODE_NONE);
        pipeline.Build();
    }

    void CreatePresentPipeline() {
        VulkanShader* shader = VulkanResourceManager::GetShader("Present");
        if (!shader) return;

        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("Present");
        pipeline.SetShader(shader);
        pipeline.AddDescriptorSetLayout(VulkanResourceManager::GetDescriptorSetLayout("StaticDescriptorSet"));
        pipeline.AddColorAttachmentFormat(VulkanSwapchainManager::GetSwapchainImageFormat());
        pipeline.SetDepthTest(false, false);
        pipeline.SetCullMode(VK_CULL_MODE_NONE);
        pipeline.Build();
    }

    void CreateVisibilityPipeline() {
        VulkanShader* shader = VulkanResourceManager::GetShader("Visibility");
        if (!shader) return;

        VulkanRenderState* renderState = VulkanResourceManager::GetRenderState("Visibility");
        if (!renderState) return;

        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("Visibility");
        pipeline.SetShader(shader);
        pipeline.AddPushConstant(sizeof(PushConstantsVisibility), VK_SHADER_STAGE_VERTEX_BIT);
        if (!VulkanRenderer::ApplyRenderStateToPipeline(pipeline, *renderState)) return;
        pipeline.SetVertexDescription(Vertex::GetPositionUVLayout());
        pipeline.Build();
    }

    void CreateVisibilitySkinnedPipeline() {
        VulkanShader* shader = VulkanResourceManager::GetShader("VisibilitySkinned");
        if (!shader) return;

        VulkanRenderState* renderState = VulkanResourceManager::GetRenderState("Visibility");
        if (!renderState) return;

        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("VisibilitySkinned");
        pipeline.SetShader(shader);
        pipeline.AddPushConstant(sizeof(PushConstantsVisibility), VK_SHADER_STAGE_VERTEX_BIT);
        if (!VulkanRenderer::ApplyRenderStateToPipeline(pipeline, *renderState)) return;
        pipeline.Build();
    }

    void CreateVisibilityAlphaDiscardPipeline() {
        VulkanShader* shader = VulkanResourceManager::GetShader("VisibilityAlphaDiscard");
        if (!shader) return;

        VulkanRenderState* renderState = VulkanResourceManager::GetRenderState("VisibilityAlphaDiscard");
        if (!renderState) return;

        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("VisibilityAlphaDiscard");
        pipeline.SetShader(shader);
        pipeline.AddDescriptorSetLayout(VulkanResourceManager::GetDescriptorSetLayout("StaticDescriptorSet"));
        pipeline.AddPushConstant(sizeof(PushConstantsVisibility), VK_SHADER_STAGE_VERTEX_BIT);
        if (!VulkanRenderer::ApplyRenderStateToPipeline(pipeline, *renderState)) return;
        pipeline.SetVertexDescription(Vertex::GetPositionUVLayout());
        pipeline.Build();
    }

    void CreateVisibilitySkinnedAlphaDiscardPipeline() {
        VulkanShader* shader = VulkanResourceManager::GetShader("VisibilitySkinnedAlphaDiscard");
        if (!shader) return;

        VulkanRenderState* renderState = VulkanResourceManager::GetRenderState("VisibilityAlphaDiscard");
        if (!renderState) return;

        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("VisibilitySkinnedAlphaDiscard");
        pipeline.SetShader(shader);
        pipeline.AddDescriptorSetLayout(VulkanResourceManager::GetDescriptorSetLayout("StaticDescriptorSet"));
        pipeline.AddPushConstant(sizeof(PushConstantsVisibility), VK_SHADER_STAGE_VERTEX_BIT);
        if (!VulkanRenderer::ApplyRenderStateToPipeline(pipeline, *renderState)) return;
        pipeline.Build();
    }

    void CreateComputeSkinningPipeline() {
        VulkanShader* shader = VulkanResourceManager::GetShader("ComputeSkinning");
        if (!shader) return;

        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("ComputeSkinning");
        pipeline.SetShader(shader);
        pipeline.AddPushConstant(sizeof(PushConstantsSkinning), VK_SHADER_STAGE_COMPUTE_BIT);
        pipeline.Build();
    }

    void CreateComputeRedTestPipeline() {
        VulkanShader* shader = VulkanResourceManager::GetShader("ComputeRedTest");
        if (!shader) return;

        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("ComputeRedTest");
        pipeline.SetShader(shader);
        pipeline.AddDescriptorSetLayout(VulkanResourceManager::GetDescriptorSetLayout("StaticDescriptorSet"));
        pipeline.Build();
    }

    void CreateVisibilityDebugPipeline() {
        VulkanShader* shader = VulkanResourceManager::GetShader("VisibilityDebug");
        if (!shader) return;

        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("VisibilityDebug");
        pipeline.SetShader(shader);
        pipeline.AddDescriptorSetLayout(VulkanResourceManager::GetDescriptorSetLayout("StaticDescriptorSet"));
        pipeline.AddColorAttachmentFormat(VK_FORMAT_R16G16B16A16_SFLOAT);
        pipeline.SetDepthTest(false, false);
        pipeline.SetCullMode(VK_CULL_MODE_NONE);
        pipeline.Build();
    }

    void CreateMaterialResolvePipeline() {
        VulkanShader* shader = VulkanResourceManager::GetShader("MaterialResolve");
        if (!shader) return;

        VulkanRenderState* renderState = VulkanResourceManager::GetRenderState("MaterialResolve");
        if (!renderState) return;

        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("MaterialResolve");
        pipeline.SetShader(shader);
        pipeline.AddDescriptorSetLayout(VulkanResourceManager::GetDescriptorSetLayout("StaticDescriptorSet"));
        pipeline.AddPushConstant(sizeof(PushConstantsMaterialResolve), VK_SHADER_STAGE_FRAGMENT_BIT);
        if (!VulkanRenderer::ApplyRenderStateToPipeline(pipeline, *renderState)) return;
        pipeline.Build();
    }

    void CreateUIPipeline() {
        VulkanShader* shader = VulkanResourceManager::GetShader("UI");
        if (!shader) return;

        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("UI");
        pipeline.SetShader(shader);
        pipeline.AddDescriptorSetLayout(VulkanResourceManager::GetDescriptorSetLayout("StaticDescriptorSet"));
        pipeline.AddPushConstant(sizeof(PushConstantsUI), VK_SHADER_STAGE_VERTEX_BIT);
        pipeline.AddColorAttachmentFormat(VulkanSwapchainManager::GetSwapchainImageFormat());
        pipeline.SetDepthTest(false, false);
        pipeline.SetCullMode(VK_CULL_MODE_NONE);
        pipeline.SetColorBlending(true);
        pipeline.SetVertexDescription<Vertex2D>();
        pipeline.Build();
    }
}

namespace VulkanRenderer {

    void CreatePipelines() {
        CreateLoadingScreenPipeline();
        CreatePresentPipeline();
        CreateVisibilityPipeline();
        CreateVisibilitySkinnedPipeline();
        CreateVisibilityAlphaDiscardPipeline();
        CreateVisibilitySkinnedAlphaDiscardPipeline();
        CreateComputeSkinningPipeline();
        CreateComputeRedTestPipeline();
        CreateVisibilityDebugPipeline();
        CreateMaterialResolvePipeline();
        CreateUIPipeline();
    }
}
