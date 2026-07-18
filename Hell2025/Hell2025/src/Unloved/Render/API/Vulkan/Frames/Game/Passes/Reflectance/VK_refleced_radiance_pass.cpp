#include "Unloved/Render/API/Vulkan/VK_renderer.h"
#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"

#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Render/API/Vulkan/Managers/vk_device_manager.h"
#include "Hell/Render/API/Vulkan/Types/vk_acceleration_structure.h"
#include "Hell/Render/API/Vulkan/Types/vk_allocated_image.h"
#include "Hell/Render/API/Vulkan/Types/vk_buffer.h"
#include "Hell/Render/API/Vulkan/Types/vk_descriptor_set.h"
#include "Hell/Render/API/Vulkan/Types/vk_pipeline.h"
#include "Hell/Render/VertexAttributes.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/ResourceManagement/Types/MeshBuffer.h"

#include "Unloved/Render/API/Vulkan/VK_descriptor_indices.h"
#include "Unloved/Render/API/Vulkan/VK_push_constants.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Render/RendererConstants.h"
#include "Unloved/Viewport/ViewportManager.h"

#include <glm/matrix.hpp>
#include <algorithm>
#include <array>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace Unloved;

namespace VulkanRenderer {

    void ReflectedRadiancePass(VkCommandBuffer commandBuffer) {
        ProfilerVulkanZoneFunction();

        AllocatedImage* reflectedRadianceImage = VulkanResourceManager::GetAllocatedImage("ReflectedRadiance");
        AllocatedImage* baseColorImage = VulkanResourceManager::GetAllocatedImage("BaseColorMetallic");
        AllocatedImage* normalImage = VulkanResourceManager::GetAllocatedImage("NormalXYRoughnessMisc");
        AllocatedImage* indirectDiffuseImage = VulkanResourceManager::GetAllocatedImage("IndirectDiffuse");
        AllocatedImage* indirectDiffuseSurfaceImage = VulkanResourceManager::GetAllocatedImage("IndirectDiffuseSurface");
        VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("ReflectedRadiance");
        VulkanRenderState* renderState = VulkanResourceManager::GetRenderState("ReflectedRadiance");
        VulkanDescriptorSet* staticDescriptorSet = VulkanResourceManager::GetDescriptorSet("StaticDescriptorSet");
        VulkanDescriptorSetResource* rayQueryDescriptorSetResource = VulkanResourceManager::GetDescriptorSetResource("RayQueryDescriptorSet");
        const VulkanFrameData& frameData = GetCurrentFrameData();
        VulkanDescriptorSet* rayQueryDescriptorSet = rayQueryDescriptorSetResource ? &rayQueryDescriptorSetResource->GetSet(GetCurrentFrameIndex()) : nullptr;
        PushConstantsFrameResources frameResources = CreatePushConstantsFrameResources();
        VulkanBuffer* rayQueryBLASInstanceDataBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.rayQueryBLASInstanceData);
        VulkanBuffer* rayQueryMeshInstanceDataBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.rayQueryMeshInstanceData);

        if (!pipeline) return;
        if (!renderState) return;
        if (!staticDescriptorSet) return;
        if (!rayQueryDescriptorSet) return;
        if (!reflectedRadianceImage) return;
        if (!baseColorImage) return;
        if (!normalImage) return;
        if (!indirectDiffuseImage) return;
        if (!indirectDiffuseSurfaceImage) return;
        if (frameResources.viewportDataDeviceAddress == 0) return;
        if (frameResources.rendererDataDeviceAddress == 0) return;
        if (frameResources.materialsDeviceAddress == 0) return;
        if (frameResources.lightsDeviceAddress == 0) return;
        if (!rayQueryBLASInstanceDataBuffer) return;
        if (!rayQueryMeshInstanceDataBuffer) return;

        VkExtent2D extent = reflectedRadianceImage->GetExtent2D();

        baseColorImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);
        normalImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);
        indirectDiffuseImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);
        indirectDiffuseSurfaceImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);

        if (!BeginRenderState(commandBuffer, *renderState, extent)) return;

        VkViewport viewport{};
        viewport.width = static_cast<float>(extent.width);
        viewport.height = static_cast<float>(extent.height);
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.extent = extent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetHandle());
        VkDescriptorSet descriptorSets[] = { staticDescriptorSet->GetHandle(), rayQueryDescriptorSet->GetHandle() };
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetLayout(), 0, 2, descriptorSets, 0, nullptr);

        PushConstantsReflectedRadiance pushConstants{};
        pushConstants.frame = frameResources;
        pushConstants.rayQueryBLASInstanceDataDeviceAddress = rayQueryBLASInstanceDataBuffer->GetDeviceAddress();
        pushConstants.rayQueryMeshInstanceDataDeviceAddress = rayQueryMeshInstanceDataBuffer->GetDeviceAddress();
        pushConstants.rayQueryEnabled = 1;
        vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstantsReflectedRadiance), &pushConstants);

        vkCmdDraw(commandBuffer, 3, 1, 0, 0);

        EndRenderState(commandBuffer);

        // Now generate mips
        reflectedRadianceImage->GenerateMipmaps(commandBuffer);
        reflectedRadianceImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);
    }
}