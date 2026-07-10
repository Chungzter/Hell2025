#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"

#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Render/API/Vulkan/Types/vk_buffer.h"

namespace {
    uint64_t GetDeviceAddressOrZero(const VulkanBuffer* buffer) {
        return buffer ? buffer->GetDeviceAddress() : 0;
    }
}

namespace VulkanRenderer {
    PushConstantsFrameResources CreatePushConstantsFrameResources() {
        const VulkanFrameData& frameData = GetCurrentFrameData();

        PushConstantsFrameResources frame{};
        frame.renderItemsDeviceAddress = GetDeviceAddressOrZero(VulkanResourceManager::GetBuffer(frameData.buffers.instanceData));
        frame.viewportDataDeviceAddress = GetDeviceAddressOrZero(VulkanResourceManager::GetBuffer(frameData.buffers.viewportData));
        frame.rendererDataDeviceAddress = GetDeviceAddressOrZero(VulkanResourceManager::GetBuffer(frameData.buffers.rendererData));
        frame.materialsDeviceAddress = GetDeviceAddressOrZero(VulkanResourceManager::GetBuffer(frameData.buffers.materials));
        frame.lightsDeviceAddress = GetDeviceAddressOrZero(VulkanResourceManager::GetBuffer(frameData.buffers.lights));
        return frame;
    }
}
