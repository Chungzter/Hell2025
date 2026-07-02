#include "VK_renderer.h"

#include "Hell/Backend/BackEnd.h"
#include "Hell/Logging.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/ResourceManagement/Types/Texture.h"
#include "Hell/Render/API/Vulkan/Managers/vk_command_manager.h"
#include "Hell/Render/API/Vulkan/Managers/vk_deletion_queue.h"
#include "Hell/Render/API/Vulkan/Managers/vk_device_manager.h"
#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Render/API/Vulkan/Managers/vk_swapchain_manager.h"
#include "Hell/Render/API/Vulkan/Managers/vk_sync_manager.h"
#include "Hell/Render/API/Vulkan/Types/vk_texture.h"
#include "Hell/Render/API/Vulkan/vk_tools.h"
#include "Hell/Render/API/Vulkan/vk_types.h"
#include "Hell/UI/UIBackEnd.h"
#include "Unloved/Common/Constants.h"
#include "Unloved/Config/Config.h"
#include "Unloved/Render/API/Vulkan/VK_descriptor_indices.h"
#include "Unloved/Render/API/Vulkan/VK_draw.h"
#include "Unloved/Render/API/Vulkan/VK_push_constants.h"
#include "Unloved/Render/API/Vulkan/vk_render_states.h"
#include "Unloved/Render/RenderDataManager.h"

#include <array>
#include <vector>

namespace VulkanRenderer {
    uint32_t g_frameIndex = 0;
    std::array<VulkanFrameData, FRAME_OVERLAP> g_frameData;
    std::vector<VkImageLayout> g_swapchainImageLayouts;
    VkExtent2D g_presentImageExtent = {};
    VkFormat g_presentImageFormat = VK_FORMAT_UNDEFINED;
    VkImageLayout g_presentImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkExtent2D g_gameGBufferExtent = {};
    VkExtent2D g_gameFinalImageExtent = {};
    VkFormat g_gameFinalImageFormat = VK_FORMAT_UNDEFINED;
    VkImageLayout g_gBufferLightingLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageLayout g_gBufferVisibilityLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageLayout g_gBufferDepthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageLayout g_finalImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    bool g_staticSamplersUploaded = false;

    void LoadShaders();
    void CreateSamplers();
    void CreateStaticDescriptorSet();
    void CreatePresentDescriptorSet();
    void CreateVisibilityDebugDescriptorSet();
    void CreateFrameData();
    void CreatePipelines();
    void EnsurePresentImage(VkExtent2D extent);
    void EnsureGameImages();
    void UpdatePresentDescriptorSet();
    void UpdateVisibilityDebugDescriptorSet();
    void UpdateVisibilityBuffers();
    void RenderLoadingScreenPass(VkCommandBuffer commandBuffer, VkImageView imageView, VkExtent2D extent);
    void RenderVisibilityPass(VkCommandBuffer commandBuffer, VkExtent2D extent);
    void RenderVisibilityAlphaDiscardPass(VkCommandBuffer commandBuffer, VkExtent2D extent);
    void RenderVisibilityDebugPass(VkCommandBuffer commandBuffer, VkImageView lightingImageView, VkExtent2D extent);
    void RenderPresentPass(VkCommandBuffer commandBuffer, VkImageView imageView, VkExtent2D extent);

    void Init() {
        LoadShaders();
        CreateSamplers();
        CreateStaticDescriptorSet();
        CreatePresentDescriptorSet();
        CreateVisibilityDebugDescriptorSet();
        CreateFrameData();
        CreateRenderStates();
        CreatePipelines();
        UpdateBindlessTextureDescriptors();
    }

    void InitMain() {
        UpdateBindlessTextureDescriptors();
    }

    void CleanUp() {
        if (VulkanDeviceManager::GetDevice() != VK_NULL_HANDLE) {
            vkDeviceWaitIdle(VulkanDeviceManager::GetDevice());
        }

        VulkanDeletionQueue::FlushAll();
        VulkanResourceManager::Cleanup();
    }

    void LoadShaders() {
        VulkanResourceManager::CreateShader("FullscreenTriangle", { "VK_fullscreen_triangle.vert", "VK_solid_color.frag" });
        VulkanResourceManager::CreateShader("Present", { "VK_fullscreen_triangle.vert", "VK_present.frag" });
        VulkanResourceManager::CreateShader("Visibility", { "VK_visibility.vert", "VK_visibility.frag" });
        VulkanResourceManager::CreateShader("VisibilityAlphaDiscard", { "VK_visibility.vert", "VK_visibility_alpha_discard.frag" });
        VulkanResourceManager::CreateShader("VisibilityDebug", { "VK_fullscreen_triangle.vert", "VK_visibility_debug.frag" });
        VulkanResourceManager::CreateShader("UI", { "VK_ui.vert", "VK_ui.frag" });
    }

    void CreateSamplers() {
        const VkPhysicalDeviceProperties& properties = VulkanDeviceManager::GetProperties();
        const float maxAnisotropy = properties.limits.maxSamplerAnisotropy;

        VulkanResourceManager::CreateSampler("Linear", VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, maxAnisotropy);
        VulkanResourceManager::CreateSampler("Nearest", VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    }

    void CreateStaticDescriptorSet() {
        g_staticSamplersUploaded = false;

        std::vector<VkDescriptorSetLayoutBinding> bindings = {
            { DESC_IDX_SAMPLERS, VK_DESCRIPTOR_TYPE_SAMPLER, 16, VK_SHADER_STAGE_ALL },
            { DESC_IDX_TEXTURES, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 10000, VK_SHADER_STAGE_ALL },
            { DESC_IDX_UBOS, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 128, VK_SHADER_STAGE_ALL },
            { DESC_IDX_SSBOS, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1024, VK_SHADER_STAGE_ALL },
            { DESC_IDX_STORAGE_IMAGES_RGBA32F, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 100, VK_SHADER_STAGE_ALL },
            { DESC_IDX_STORAGE_IMAGES_RGBA16F, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 100, VK_SHADER_STAGE_ALL },
            { DESC_IDX_STORAGE_IMAGES_RGBA8, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 100, VK_SHADER_STAGE_ALL }
        };

        std::vector<VkDescriptorBindingFlags> flags(bindings.size(), 0);
        flags[DESC_IDX_TEXTURES] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;

        VkDescriptorSetLayoutBindingFlagsCreateInfo flagsInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO };
        flagsInfo.bindingCount = static_cast<uint32_t>(flags.size());
        flagsInfo.pBindingFlags = flags.data();

        VkDescriptorSetLayoutCreateInfo layoutInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        layoutInfo.pNext = &flagsInfo;
        layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        VulkanResourceManager::CreateDescriptorSet("StaticDescriptorSet", layoutInfo, DescriptorSetLifetime::STATIC);
    }

    void CreatePresentDescriptorSet() {
        VkDescriptorSetLayoutBinding binding{};
        binding.binding = 0;
        binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        binding.descriptorCount = 1;
        binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &binding;

        VulkanResourceManager::CreateDescriptorSet("PresentDescriptorSet", layoutInfo, DescriptorSetLifetime::STATIC);
    }

    void CreateVisibilityDebugDescriptorSet() {
        VkDescriptorSetLayoutBinding binding{};
        binding.binding = 0;
        binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        binding.descriptorCount = 1;
        binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &binding;

        VulkanResourceManager::CreateDescriptorSet("VisibilityDebugDescriptorSet", layoutInfo, DescriptorSetLifetime::STATIC);
    }

    void CreateFrameData() {
        VkBufferUsageFlags usageStorage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        VkBufferUsageFlags usageIndirect = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        VmaAllocationCreateFlags vmaFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

        for (VulkanFrameData& frameData : g_frameData) {
            frameData.buffers.instanceData = VulkanResourceManager::CreateBuffer(sizeof(RenderItem) * MAX_INSTANCE_DATA_COUNT, usageStorage, VMA_MEMORY_USAGE_AUTO, vmaFlags);
            frameData.buffers.viewportData = VulkanResourceManager::CreateBuffer(sizeof(ViewportData) * MAX_VIEWPORT_COUNT, usageStorage, VMA_MEMORY_USAGE_AUTO, vmaFlags);
            frameData.buffers.drawCommands = VulkanResourceManager::CreateBuffer(sizeof(DrawIndexedIndirectCommand) * MAX_INDIRECT_DRAW_COMMAND_COUNT, usageIndirect, VMA_MEMORY_USAGE_AUTO, vmaFlags);
            frameData.buffers.uiRenderItems = VulkanResourceManager::CreateBuffer(sizeof(RenderItemUI) * VULKAN_MAX_UI_RENDER_ITEMS, usageStorage, VMA_MEMORY_USAGE_AUTO, vmaFlags);
            frameData.buffers.uiDrawCommands = VulkanResourceManager::CreateBuffer(sizeof(DrawIndexedIndirectCommand) * VULKAN_MAX_UI_RENDER_ITEMS, usageIndirect, VMA_MEMORY_USAGE_AUTO, vmaFlags);
        }
    }

    void CreatePipelines() {
        VulkanShader* shader = VulkanResourceManager::GetShader("FullscreenTriangle");
        if (!shader) return;

        VulkanPipeline& pipeline = VulkanResourceManager::CreatePipeline("LoadingScreen");
        pipeline.SetShader(shader);
        pipeline.AddColorAttachmentFormat(VulkanSwapchainManager::GetSwapchainImageFormat());
        pipeline.SetDepthTest(false, false);
        pipeline.SetCullMode(VK_CULL_MODE_NONE);
        pipeline.Build();

        VulkanShader* presentShader = VulkanResourceManager::GetShader("Present");
        if (!presentShader) return;

        VulkanPipeline& presentPipeline = VulkanResourceManager::CreatePipeline("Present");
        presentPipeline.SetShader(presentShader);
        presentPipeline.AddDescriptorSetLayout(VulkanResourceManager::GetDescriptorSetLayout("PresentDescriptorSet"));
        presentPipeline.AddColorAttachmentFormat(VulkanSwapchainManager::GetSwapchainImageFormat());
        presentPipeline.SetDepthTest(false, false);
        presentPipeline.SetCullMode(VK_CULL_MODE_NONE);
        presentPipeline.Build();

        VulkanShader* visibilityShader = VulkanResourceManager::GetShader("Visibility");
        if (visibilityShader) {
            VulkanRenderState* renderState = GetRenderState("Visibility");
            if (!renderState) return;

            VulkanPipeline& visibilityPipeline = VulkanResourceManager::CreatePipeline("Visibility");
            visibilityPipeline.SetShader(visibilityShader);
            visibilityPipeline.AddPushConstant(sizeof(PushConstantsVisibility), VK_SHADER_STAGE_VERTEX_BIT);
            if (!ApplyRenderStateToPipeline(visibilityPipeline, *renderState)) return;
            visibilityPipeline.SetVertexDescription<Vertex>();
            visibilityPipeline.Build();
        }

        VulkanShader* visibilityAlphaDiscardShader = VulkanResourceManager::GetShader("VisibilityAlphaDiscard");
        if (visibilityAlphaDiscardShader) {
            VulkanRenderState* renderState = GetRenderState("VisibilityAlphaDiscard");
            if (!renderState) return;

            VulkanPipeline& visibilityAlphaDiscardPipeline = VulkanResourceManager::CreatePipeline("VisibilityAlphaDiscard");
            visibilityAlphaDiscardPipeline.SetShader(visibilityAlphaDiscardShader);
            visibilityAlphaDiscardPipeline.AddDescriptorSetLayout(VulkanResourceManager::GetDescriptorSetLayout("StaticDescriptorSet"));
            visibilityAlphaDiscardPipeline.AddPushConstant(sizeof(PushConstantsVisibility), VK_SHADER_STAGE_VERTEX_BIT);
            if (!ApplyRenderStateToPipeline(visibilityAlphaDiscardPipeline, *renderState)) return;
            visibilityAlphaDiscardPipeline.SetVertexDescription<Vertex>();
            visibilityAlphaDiscardPipeline.Build();
        }

        VulkanShader* visibilityDebugShader = VulkanResourceManager::GetShader("VisibilityDebug");
        if (visibilityDebugShader) {
            VulkanPipeline& visibilityDebugPipeline = VulkanResourceManager::CreatePipeline("VisibilityDebug");
            visibilityDebugPipeline.SetShader(visibilityDebugShader);
            visibilityDebugPipeline.AddDescriptorSetLayout(VulkanResourceManager::GetDescriptorSetLayout("VisibilityDebugDescriptorSet"));
            visibilityDebugPipeline.AddColorAttachmentFormat(VK_FORMAT_R16G16B16A16_SFLOAT);
            visibilityDebugPipeline.SetDepthTest(false, false);
            visibilityDebugPipeline.SetCullMode(VK_CULL_MODE_NONE);
            visibilityDebugPipeline.Build();
        }

        VulkanShader* uiShader = VulkanResourceManager::GetShader("UI");
        if (!uiShader) return;

        VulkanPipeline& uiPipeline = VulkanResourceManager::CreatePipeline("UI");
        uiPipeline.SetShader(uiShader);
        uiPipeline.AddDescriptorSetLayout(VulkanResourceManager::GetDescriptorSetLayout("StaticDescriptorSet"));
        uiPipeline.AddPushConstant(sizeof(PushConstantsUI), VK_SHADER_STAGE_VERTEX_BIT);
        uiPipeline.AddColorAttachmentFormat(VulkanSwapchainManager::GetSwapchainImageFormat());
        uiPipeline.SetDepthTest(false, false);
        uiPipeline.SetCullMode(VK_CULL_MODE_NONE);
        uiPipeline.SetColorBlending(true);
        uiPipeline.SetVertexDescription<Vertex2D>();
        uiPipeline.Build();
    }

    VulkanFrameData& GetCurrentFrameData() {
        return g_frameData[g_frameIndex % FRAME_OVERLAP];
    }

    VulkanFrameData& GetFrameDataByIndex(uint32_t frameIndex) {
        return g_frameData[frameIndex % FRAME_OVERLAP];
    }

    uint32_t GetCurrentFrameIndex() {
        return g_frameIndex % FRAME_OVERLAP;
    }

    void EnsurePresentImage(VkExtent2D extent) {
        VkFormat format = VulkanSwapchainManager::GetSwapchainImageFormat();
        bool needsCreate = !VulkanResourceManager::AllocatedImageExists("Present");
        needsCreate = needsCreate || g_presentImageExtent.width != extent.width || g_presentImageExtent.height != extent.height || g_presentImageFormat != format;

        if (!needsCreate) return;

        if (VulkanResourceManager::AllocatedImageExists("Present")) {
            vkDeviceWaitIdle(VulkanDeviceManager::GetDevice());
        }

        VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        VulkanResourceManager::CreateAllocatedImage("Present", extent.width, extent.height, format, usage);

        g_presentImageExtent = extent;
        g_presentImageFormat = format;
        g_presentImageLayout = VK_IMAGE_LAYOUT_GENERAL;

        UpdatePresentDescriptorSet();
    }

    void EnsureGameImages() {
        const Resolutions& resolutions = Config::GetResolutions();
        VkExtent2D gBufferExtent{ static_cast<uint32_t>(resolutions.gBuffer.x), static_cast<uint32_t>(resolutions.gBuffer.y) };
        VkExtent2D finalImageExtent{ static_cast<uint32_t>(resolutions.finalImage.x), static_cast<uint32_t>(resolutions.finalImage.y) };
        VkFormat finalImageFormat = VulkanSwapchainManager::GetSwapchainImageFormat();

        bool needsCreate = !VulkanResourceManager::AllocatedImageExists("GBufferRE.Lighting");
        needsCreate = needsCreate || !VulkanResourceManager::AllocatedImageExists("GBufferRE.Visibility");
        needsCreate = needsCreate || !VulkanResourceManager::AllocatedImageExists("GBufferRE.Depth");
        needsCreate = needsCreate || !VulkanResourceManager::AllocatedImageExists("FinalImage.Color");
        needsCreate = needsCreate || g_gameGBufferExtent.width != gBufferExtent.width || g_gameGBufferExtent.height != gBufferExtent.height;
        needsCreate = needsCreate || g_gameFinalImageExtent.width != finalImageExtent.width || g_gameFinalImageExtent.height != finalImageExtent.height;
        needsCreate = needsCreate || g_gameFinalImageFormat != finalImageFormat;

        if (!needsCreate) return;

        if (VulkanResourceManager::AllocatedImageExists("GBufferRE.Lighting")) {
            vkDeviceWaitIdle(VulkanDeviceManager::GetDevice());
        }

        VulkanResourceManager::CreateAllocatedImage("GBufferRE.Lighting", gBufferExtent.width, gBufferExtent.height, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
        VulkanResourceManager::CreateAllocatedImage("GBufferRE.Visibility", gBufferExtent.width, gBufferExtent.height, VK_FORMAT_R32G32_UINT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
        VulkanResourceManager::CreateAllocatedImage("GBufferRE.Depth", gBufferExtent.width, gBufferExtent.height, VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
        VulkanResourceManager::CreateAllocatedImage("FinalImage.Color", finalImageExtent.width, finalImageExtent.height, finalImageFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

        g_gameGBufferExtent = gBufferExtent;
        g_gameFinalImageExtent = finalImageExtent;
        g_gameFinalImageFormat = finalImageFormat;
        g_gBufferLightingLayout = VK_IMAGE_LAYOUT_GENERAL;
        g_gBufferVisibilityLayout = VK_IMAGE_LAYOUT_GENERAL;
        g_gBufferDepthLayout = VK_IMAGE_LAYOUT_GENERAL;
        g_finalImageLayout = VK_IMAGE_LAYOUT_GENERAL;

        UpdateVisibilityDebugDescriptorSet();
    }

    void UpdatePresentDescriptorSet() {
        AllocatedImage* presentImage = VulkanResourceManager::GetAllocatedImage("Present");
        VulkanDescriptorSet* descriptorSet = VulkanResourceManager::GetDescriptorSet("PresentDescriptorSet");
        VulkanSampler* sampler = VulkanResourceManager::GetSampler("Linear");
        if (!presentImage || !descriptorSet || !sampler) return;

        descriptorSet->WriteImage(0, presentImage->GetImageView(), sampler->GetSampler(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        descriptorSet->Update();
    }

    void UpdateVisibilityDebugDescriptorSet() {
        AllocatedImage* visibilityImage = VulkanResourceManager::GetAllocatedImage("GBufferRE.Visibility");
        VulkanDescriptorSet* descriptorSet = VulkanResourceManager::GetDescriptorSet("VisibilityDebugDescriptorSet");
        VulkanSampler* sampler = VulkanResourceManager::GetSampler("Nearest");
        if (!visibilityImage || !descriptorSet || !sampler) return;

        descriptorSet->WriteImage(0, visibilityImage->GetImageView(), sampler->GetSampler(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        descriptorSet->Update();
    }

    void UpdateBindlessTextureDescriptors() {
        VulkanDescriptorSet* staticDescriptorSet = VulkanResourceManager::GetDescriptorSet("StaticDescriptorSet");
        VulkanSampler* linearSampler = VulkanResourceManager::GetSampler("Linear");
        VulkanSampler* nearestSampler = VulkanResourceManager::GetSampler("Nearest");

        if (!staticDescriptorSet || !linearSampler || !nearestSampler) return;

        if (!g_staticSamplersUploaded) {
            staticDescriptorSet->WriteImage(DESC_IDX_SAMPLERS, VK_NULL_HANDLE, linearSampler->GetSampler(), VK_IMAGE_LAYOUT_UNDEFINED, VK_DESCRIPTOR_TYPE_SAMPLER, 0);
            staticDescriptorSet->WriteImage(DESC_IDX_SAMPLERS, VK_NULL_HANDLE, nearestSampler->GetSampler(), VK_IMAGE_LAYOUT_UNDEFINED, VK_DESCRIPTOR_TYPE_SAMPLER, 1);
            g_staticSamplersUploaded = true;
        }

        for (auto& [name, texture] : Hell::ResourceManager::GetTextures()) {
            if (texture.GetUploadState() != UploadState::UPLOADED) continue;
            if (texture.GetBindlessIndex() < 0) continue;
            if (texture.GetVulkanId() == 0) continue;

            VulkanTexture* vulkanTexture = VulkanResourceManager::GetTexturePtr(texture.GetVulkanId());
            if (!vulkanTexture || vulkanTexture->GetImageView() == VK_NULL_HANDLE) continue;

            staticDescriptorSet->WriteImage(DESC_IDX_TEXTURES, vulkanTexture->GetImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, static_cast<uint32_t>(texture.GetBindlessIndex()));
        }

        staticDescriptorSet->Update();
    }

    void UpdateUIBuffers() {
        const std::vector<RenderItemUI>& renderItems = UIBackEnd::GetRenderItems();
        const std::vector<DrawIndexedIndirectCommand>& drawCommands = Unloved::RenderDataManager::GetDrawCommandsUI();

        if (renderItems.empty() || drawCommands.empty()) return;

        VulkanFrameData& frameData = GetCurrentFrameData();
        VulkanBuffer* renderItemBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.uiRenderItems);
        VulkanBuffer* drawCommandBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.uiDrawCommands);
        if (!renderItemBuffer || !drawCommandBuffer) return;

        const VkDeviceSize renderItemSize = sizeof(RenderItemUI) * renderItems.size();
        const VkDeviceSize drawCommandSize = sizeof(DrawIndexedIndirectCommand) * drawCommands.size();

        if (renderItemSize > renderItemBuffer->GetSize() || drawCommandSize > drawCommandBuffer->GetSize()) {
            Logging::Error() << "VulkanRenderer::UpdateUIBuffers() UI data exceeded the current Vulkan buffer capacity\n";
            return;
        }

        renderItemBuffer->UpdateData(renderItems.data(), renderItemSize);
        drawCommandBuffer->UpdateData(drawCommands.data(), drawCommandSize);
    }

    void UpdateVisibilityBuffers() {
        const std::vector<RenderItem>& instanceData = Unloved::RenderDataManager::GetInstanceData();
        const std::vector<ViewportData>& viewportData = Unloved::RenderDataManager::GetViewportData();

        if (instanceData.empty() || viewportData.empty()) return;

        VulkanFrameData& frameData = GetCurrentFrameData();
        VulkanBuffer* instanceDataBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.instanceData);
        VulkanBuffer* viewportDataBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.viewportData);
        if (!instanceDataBuffer || !viewportDataBuffer) return;

        const VkDeviceSize instanceDataSize = sizeof(RenderItem) * instanceData.size();
        const VkDeviceSize viewportDataSize = sizeof(ViewportData) * viewportData.size();

        if (instanceDataSize > instanceDataBuffer->GetSize() || viewportDataSize > viewportDataBuffer->GetSize()) {
            Logging::Error() << "VulkanRenderer::UpdateVisibilityBuffers() visibility data exceeded the current Vulkan buffer capacity\n";
            return;
        }

        instanceDataBuffer->UpdateData(instanceData.data(), instanceDataSize);
        viewportDataBuffer->UpdateData(viewportData.data(), viewportDataSize);
    }

    void RenderLoadingScreen() {
        VkDevice device = VulkanDeviceManager::GetDevice();
        VkSwapchainKHR swapchain = VulkanSwapchainManager::GetSwapchain();
        std::vector<VkImage>& swapchainImages = VulkanSwapchainManager::GetSwapchainImages();
        std::vector<VkImageView>& swapchainImageViews = VulkanSwapchainManager::GetSwapchainImageViews();

        if (g_swapchainImageLayouts.size() != swapchainImages.size()) {
            g_swapchainImageLayouts.assign(swapchainImages.size(), VK_IMAGE_LAYOUT_UNDEFINED);
        }

        uint32_t frameIndex = g_frameIndex % FRAME_OVERLAP;

        VulkanSyncManager::WaitForRenderFence(frameIndex);
        VulkanDeletionQueue::Flush(frameIndex);
        VulkanDeletionQueue::SetFrameIndex(frameIndex);

        uint32_t imageIndex = 0;
        VkResult acquireResult = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, VulkanSyncManager::GetPresentSemaphore(frameIndex), VK_NULL_HANDLE, &imageIndex);

        if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
            VulkanSwapchainManager::RecreateSwapchain();
            g_swapchainImageLayouts.clear();
            return;
        }
        if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR) {
            Logging::Error() << "VulkanRenderer::RenderLoadingScreen() failed to acquire swapchain image\n";
            return;
        }

        VkExtent2D extent = VulkanSwapchainManager::GetSwapchainExtent();

        EnsurePresentImage(extent);
        AllocatedImage* presentImage = VulkanResourceManager::GetAllocatedImage("Present");
        if (!presentImage) return;

        VulkanSyncManager::ResetRenderFence(frameIndex);

        VkCommandPool commandPool = VulkanCommandManager::GetGraphicsCommandPool(frameIndex);
        VkCommandBuffer commandBuffer = VulkanCommandManager::GetGraphicsCommandBuffer(frameIndex);

        vkResetCommandPool(device, commandPool, 0);

        VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        VkImage image = swapchainImages[imageIndex];
        vktools::setImageLayout(commandBuffer, image, VK_IMAGE_ASPECT_COLOR_BIT, g_swapchainImageLayouts[imageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        g_swapchainImageLayouts[imageIndex] = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        vktools::setImageLayout(commandBuffer, presentImage->GetImage(), VK_IMAGE_ASPECT_COLOR_BIT, g_presentImageLayout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        g_presentImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        RenderLoadingScreenPass(commandBuffer, presentImage->GetImageView(), extent);
        UpdateUIBuffers();
        RenderUIPass(commandBuffer, presentImage->GetImageView(), extent);

        vktools::setImageLayout(commandBuffer, presentImage->GetImage(), VK_IMAGE_ASPECT_COLOR_BIT, g_presentImageLayout, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        g_presentImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        RenderPresentPass(commandBuffer, swapchainImageViews[imageIndex], extent);

        vktools::setImageLayout(commandBuffer, image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
        g_swapchainImageLayouts[imageIndex] = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        vkEndCommandBuffer(commandBuffer);

        VkSemaphore waitSemaphore = VulkanSyncManager::GetPresentSemaphore(frameIndex);
        VkSemaphore signalSemaphore = VulkanSyncManager::GetRenderFinishedSemaphore(frameIndex, imageIndex);
        VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &waitSemaphore;
        submitInfo.pWaitDstStageMask = &waitStage;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &signalSemaphore;

        vkQueueSubmit(VulkanDeviceManager::GetGraphicsQueue(), 1, &submitInfo, VulkanSyncManager::GetRenderFence(frameIndex));

        VkPresentInfoKHR presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &signalSemaphore;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapchain;
        presentInfo.pImageIndices = &imageIndex;

        VkResult presentResult = vkQueuePresentKHR(VulkanDeviceManager::GetPresentQueue(), &presentInfo);
        if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR) {
            VulkanSwapchainManager::RecreateSwapchain();
            g_swapchainImageLayouts.clear();
        }
        else if (presentResult != VK_SUCCESS) {
            Logging::Error() << "VulkanRenderer::RenderLoadingScreen() failed to present swapchain image\n";
        }

        g_frameIndex = (g_frameIndex + 1) % FRAME_OVERLAP;
    }

    void RenderGame() {
        VkDevice device = VulkanDeviceManager::GetDevice();
        VkSwapchainKHR swapchain = VulkanSwapchainManager::GetSwapchain();
        std::vector<VkImage>& swapchainImages = VulkanSwapchainManager::GetSwapchainImages();
        std::vector<VkImageView>& swapchainImageViews = VulkanSwapchainManager::GetSwapchainImageViews();

        if (g_swapchainImageLayouts.size() != swapchainImages.size()) {
            g_swapchainImageLayouts.assign(swapchainImages.size(), VK_IMAGE_LAYOUT_UNDEFINED);
        }

        uint32_t frameIndex = g_frameIndex % FRAME_OVERLAP;

        VulkanSyncManager::WaitForRenderFence(frameIndex);
        VulkanDeletionQueue::Flush(frameIndex);
        VulkanDeletionQueue::SetFrameIndex(frameIndex);

        uint32_t imageIndex = 0;
        VkResult acquireResult = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, VulkanSyncManager::GetPresentSemaphore(frameIndex), VK_NULL_HANDLE, &imageIndex);

        if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
            VulkanSwapchainManager::RecreateSwapchain();
            g_swapchainImageLayouts.clear();
            return;
        }
        if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR) {
            Logging::Error() << "VulkanRenderer::RenderGame() failed to acquire swapchain image\n";
            return;
        }

        VkExtent2D extent = VulkanSwapchainManager::GetSwapchainExtent();

        EnsurePresentImage(extent);
        EnsureGameImages();

        AllocatedImage* presentImage = VulkanResourceManager::GetAllocatedImage("Present");
        AllocatedImage* lightingImage = VulkanResourceManager::GetAllocatedImage("GBufferRE.Lighting");
        AllocatedImage* visibilityImage = VulkanResourceManager::GetAllocatedImage("GBufferRE.Visibility");
        AllocatedImage* depthImage = VulkanResourceManager::GetAllocatedImage("GBufferRE.Depth");
        AllocatedImage* finalImage = VulkanResourceManager::GetAllocatedImage("FinalImage.Color");
        if (!presentImage || !lightingImage || !visibilityImage || !depthImage || !finalImage) return;

        UpdateVisibilityBuffers();

        VulkanSyncManager::ResetRenderFence(frameIndex);

        VkCommandPool commandPool = VulkanCommandManager::GetGraphicsCommandPool(frameIndex);
        VkCommandBuffer commandBuffer = VulkanCommandManager::GetGraphicsCommandBuffer(frameIndex);

        vkResetCommandPool(device, commandPool, 0);

        VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(commandBuffer, &beginInfo);
        BeginDrawCommandWrite();

        VkImage image = swapchainImages[imageIndex];
        vktools::setImageLayout(commandBuffer, image, VK_IMAGE_ASPECT_COLOR_BIT, g_swapchainImageLayouts[imageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        g_swapchainImageLayouts[imageIndex] = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        vktools::setImageLayout(commandBuffer, visibilityImage->GetImage(), VK_IMAGE_ASPECT_COLOR_BIT, g_gBufferVisibilityLayout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        g_gBufferVisibilityLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        vktools::setImageLayout(commandBuffer, depthImage->GetImage(), VK_IMAGE_ASPECT_DEPTH_BIT, g_gBufferDepthLayout, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
        g_gBufferDepthLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;

        RenderVisibilityPass(commandBuffer, g_gameGBufferExtent);
        RenderVisibilityAlphaDiscardPass(commandBuffer, g_gameGBufferExtent);

        vktools::setImageLayout(commandBuffer, visibilityImage->GetImage(), VK_IMAGE_ASPECT_COLOR_BIT, g_gBufferVisibilityLayout, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        g_gBufferVisibilityLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        vktools::setImageLayout(commandBuffer, lightingImage->GetImage(), VK_IMAGE_ASPECT_COLOR_BIT, g_gBufferLightingLayout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        g_gBufferLightingLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        RenderVisibilityDebugPass(commandBuffer, lightingImage->GetImageView(), g_gameGBufferExtent);

        vktools::setImageLayout(commandBuffer, lightingImage->GetImage(), VK_IMAGE_ASPECT_COLOR_BIT, g_gBufferLightingLayout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        g_gBufferLightingLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

        vktools::setImageLayout(commandBuffer, finalImage->GetImage(), VK_IMAGE_ASPECT_COLOR_BIT, g_finalImageLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        g_finalImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

        VkImageBlit downscaleBlit{};
        downscaleBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        downscaleBlit.srcSubresource.layerCount = 1;
        downscaleBlit.srcOffsets[1].x = static_cast<int32_t>(g_gameGBufferExtent.width);
        downscaleBlit.srcOffsets[1].y = static_cast<int32_t>(g_gameGBufferExtent.height);
        downscaleBlit.srcOffsets[1].z = 1;
        downscaleBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        downscaleBlit.dstSubresource.layerCount = 1;
        downscaleBlit.dstOffsets[1].x = static_cast<int32_t>(g_gameFinalImageExtent.width);
        downscaleBlit.dstOffsets[1].y = static_cast<int32_t>(g_gameFinalImageExtent.height);
        downscaleBlit.dstOffsets[1].z = 1;
        vkCmdBlitImage(commandBuffer, lightingImage->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, finalImage->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &downscaleBlit, VK_FILTER_LINEAR);

        vktools::setImageLayout(commandBuffer, finalImage->GetImage(), VK_IMAGE_ASPECT_COLOR_BIT, g_finalImageLayout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        g_finalImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

        vktools::setImageLayout(commandBuffer, presentImage->GetImage(), VK_IMAGE_ASPECT_COLOR_BIT, g_presentImageLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        g_presentImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

        VkImageBlit upscaleBlit{};
        upscaleBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        upscaleBlit.srcSubresource.layerCount = 1;
        upscaleBlit.srcOffsets[1].x = static_cast<int32_t>(g_gameFinalImageExtent.width);
        upscaleBlit.srcOffsets[1].y = static_cast<int32_t>(g_gameFinalImageExtent.height);
        upscaleBlit.srcOffsets[1].z = 1;
        upscaleBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        upscaleBlit.dstSubresource.layerCount = 1;
        upscaleBlit.dstOffsets[1].x = static_cast<int32_t>(extent.width);
        upscaleBlit.dstOffsets[1].y = static_cast<int32_t>(extent.height);
        upscaleBlit.dstOffsets[1].z = 1;
        vkCmdBlitImage(commandBuffer, finalImage->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, presentImage->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &upscaleBlit, VK_FILTER_NEAREST);

        vktools::setImageLayout(commandBuffer, presentImage->GetImage(), VK_IMAGE_ASPECT_COLOR_BIT, g_presentImageLayout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        g_presentImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        UpdateUIBuffers();
        RenderUIPass(commandBuffer, presentImage->GetImageView(), extent);

        vktools::setImageLayout(commandBuffer, presentImage->GetImage(), VK_IMAGE_ASPECT_COLOR_BIT, g_presentImageLayout, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        g_presentImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        RenderPresentPass(commandBuffer, swapchainImageViews[imageIndex], extent);

        vktools::setImageLayout(commandBuffer, image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
        g_swapchainImageLayouts[imageIndex] = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        vkEndCommandBuffer(commandBuffer);

        VkSemaphore waitSemaphore = VulkanSyncManager::GetPresentSemaphore(frameIndex);
        VkSemaphore signalSemaphore = VulkanSyncManager::GetRenderFinishedSemaphore(frameIndex, imageIndex);
        VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &waitSemaphore;
        submitInfo.pWaitDstStageMask = &waitStage;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &signalSemaphore;

        vkQueueSubmit(VulkanDeviceManager::GetGraphicsQueue(), 1, &submitInfo, VulkanSyncManager::GetRenderFence(frameIndex));

        VkPresentInfoKHR presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &signalSemaphore;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapchain;
        presentInfo.pImageIndices = &imageIndex;

        VkResult presentResult = vkQueuePresentKHR(VulkanDeviceManager::GetPresentQueue(), &presentInfo);
        if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR) {
            VulkanSwapchainManager::RecreateSwapchain();
            g_swapchainImageLayouts.clear();
        }
        else if (presentResult != VK_SUCCESS) {
            Logging::Error() << "VulkanRenderer::RenderGame() failed to present swapchain image\n";
        }

        g_frameIndex = (g_frameIndex + 1) % FRAME_OVERLAP;
    }
}
