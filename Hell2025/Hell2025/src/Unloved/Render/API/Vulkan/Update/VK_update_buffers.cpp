#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"

#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/UI/UIBackEnd.h"

#include "Unloved/Render/RenderDataManager.h"

#include <algorithm>
#include <array>
#include <vector>

namespace VulkanRenderer {

    void UpdateBuffers() {
        const VulkanFrameData& frameData = GetCurrentFrameData();

        const std::vector<RenderItem>& instanceData = Unloved::RenderDataManager::GetInstanceData();
        const std::vector<ViewportData>& viewportData = Unloved::RenderDataManager::GetViewportData();
        const RendererData& rendererData = Unloved::RenderDataManager::GetRendererData();
        const std::vector<GPULight>& gpuLights = Unloved::RenderDataManager::GetGPULights();
        const std::vector<Material>& materials = Hell::ResourceManager::GetMaterials();
        const std::vector<glm::mat4>& skinningTransforms = Unloved::RenderDataManager::GetSkinningTransforms();
        std::array<GPULight, 8> gpuLightBufferData{};
        const size_t gpuLightCount = std::min(gpuLights.size(), gpuLightBufferData.size());
        std::copy_n(gpuLights.data(), gpuLightCount, gpuLightBufferData.data());

        UpdateBuffer(VulkanResourceManager::GetBuffer(frameData.buffers.instanceData), instanceData.data(), sizeof(RenderItem) * instanceData.size());
        UpdateBuffer(VulkanResourceManager::GetBuffer(frameData.buffers.viewportData), viewportData.data(), sizeof(ViewportData) * viewportData.size());
        UpdateBuffer(VulkanResourceManager::GetBuffer(frameData.buffers.rendererData), &rendererData, sizeof(RendererData));
        UpdateBuffer(VulkanResourceManager::GetBuffer(frameData.buffers.gpuLights), gpuLightBufferData.data(), sizeof(GPULight) * gpuLightBufferData.size());
        if (EnsureBufferSize(frameData.buffers.materials, sizeof(Material) * materials.size())) {
            UpdateBuffer(VulkanResourceManager::GetBuffer(frameData.buffers.materials), materials.data(), sizeof(Material) * materials.size());
        }
        UpdateBuffer(VulkanResourceManager::GetBuffer(frameData.buffers.skinningTransforms), skinningTransforms.data(), sizeof(glm::mat4) * skinningTransforms.size());
    }

    void UpdateBuffersUI() {
        const VulkanFrameData& frameData = GetCurrentFrameData();

        const std::vector<RenderItemUI>& renderItems = UIBackEnd::GetRenderItems();

        UpdateBuffer(VulkanResourceManager::GetBuffer(frameData.buffers.uiRenderItems), renderItems.data(), sizeof(RenderItemUI) * renderItems.size());
    }
}
