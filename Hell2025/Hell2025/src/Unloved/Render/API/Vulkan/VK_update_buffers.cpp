#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"

#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/UI/UIBackEnd.h"

#include "Unloved/Render/RenderDataManager.h"

#include <vector>

namespace VulkanRenderer {

    void UpdateBuffers() {
        const VulkanFrameData& frameData = GetCurrentFrameData();

        const std::vector<RenderItem>& instanceData = Unloved::RenderDataManager::GetInstanceData();
        const std::vector<ViewportData>& viewportData = Unloved::RenderDataManager::GetViewportData();
        const RendererData& rendererData = Unloved::RenderDataManager::GetRendererData();
        const std::vector<glm::mat4>& skinningTransforms = Unloved::RenderDataManager::GetSkinningTransforms();

        UpdateBuffer(VulkanResourceManager::GetBuffer(frameData.buffers.instanceData), instanceData.data(), sizeof(RenderItem) * instanceData.size());
        UpdateBuffer(VulkanResourceManager::GetBuffer(frameData.buffers.viewportData), viewportData.data(), sizeof(ViewportData) * viewportData.size());
        UpdateBuffer(VulkanResourceManager::GetBuffer(frameData.buffers.rendererData), &rendererData, sizeof(RendererData));
        UpdateBuffer(VulkanResourceManager::GetBuffer(frameData.buffers.skinningTransforms), skinningTransforms.data(), sizeof(glm::mat4) * skinningTransforms.size());
    }

    void UpdateBuffersUI() {
        const VulkanFrameData& frameData = GetCurrentFrameData();

        const std::vector<RenderItemUI>& renderItems = UIBackEnd::GetRenderItems();

        UpdateBuffer(VulkanResourceManager::GetBuffer(frameData.buffers.uiRenderItems), renderItems.data(), sizeof(RenderItemUI) * renderItems.size());
    }
}
