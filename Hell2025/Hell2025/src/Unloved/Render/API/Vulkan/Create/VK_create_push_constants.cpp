#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"

#include "Hell/Logging.h"
#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Render/API/Vulkan/Types/vk_buffer.h"

namespace {
    uint64_t GetDeviceAddressOrZero(uint64_t bufferId) {
        const VulkanBuffer* buffer = VulkanResourceManager::GetBuffer(bufferId);
        return buffer ? buffer->GetDeviceAddress() : 0;
    }

    bool ValidateDeviceAddress(uint64_t deviceAddress, const char* tableEntryName) {
        if (deviceAddress != 0) return true;

        Logging::Error() << "FrameAddressTable entry '" << tableEntryName << "' has no valid device address\n";
        return false;
    }
}

namespace VulkanRenderer {
    bool UpdateFrameAddressTable() {
        const VulkanFrameData& frameData = GetCurrentFrameData();

        FrameAddressTable table{};
        table.renderItemBuffer = GetDeviceAddressOrZero(frameData.buffers.instanceData);
        table.viewportDataBuffer = GetDeviceAddressOrZero(frameData.buffers.viewportData);
        table.rendererDataBuffer = GetDeviceAddressOrZero(frameData.buffers.rendererData);
        table.materialBuffer = GetDeviceAddressOrZero(frameData.buffers.materials);
        table.lightBuffer = GetDeviceAddressOrZero(frameData.buffers.lights);
        table.spriteSheetRenderItemBuffer = GetDeviceAddressOrZero(frameData.buffers.spriteSheetInstanceData);
        table.uiRenderItemBuffer = GetDeviceAddressOrZero(frameData.buffers.uiRenderItems);
        table.tileLightBuffer = GetDeviceAddressOrZero(frameData.buffers.tileLights);
        table.tileWorldBoundsBuffer = GetDeviceAddressOrZero(frameData.buffers.tileWorldBounds);

        bool valid = true;
        valid &= ValidateDeviceAddress(table.renderItemBuffer, "renderItemBuffer");
        valid &= ValidateDeviceAddress(table.viewportDataBuffer, "viewportDataBuffer");
        valid &= ValidateDeviceAddress(table.rendererDataBuffer, "rendererDataBuffer");
        valid &= ValidateDeviceAddress(table.materialBuffer, "materialBuffer");
        valid &= ValidateDeviceAddress(table.lightBuffer, "lightBuffer");
        valid &= ValidateDeviceAddress(table.spriteSheetRenderItemBuffer, "spriteSheetRenderItemBuffer");
        valid &= ValidateDeviceAddress(table.uiRenderItemBuffer, "uiRenderItemBuffer");
        valid &= ValidateDeviceAddress(table.tileLightBuffer, "tileLightBuffer");
        valid &= ValidateDeviceAddress(table.tileWorldBoundsBuffer, "tileWorldBoundsBuffer");

        VulkanBuffer* tableBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.frameAddressTable);
        valid &= ValidateDeviceAddress(tableBuffer ? tableBuffer->GetDeviceAddress() : 0, "FrameAddressTable");
        if (!valid) return false;

        if (!UpdateBuffer(tableBuffer, &table, sizeof(table))) {
            Logging::Error() << "Failed to update the current FrameAddressTable buffer\n";
            return false;
        }
        return true;
    }

    uint64_t GetFrameAddressTableDeviceAddress() {
        const VulkanFrameData& frameData = GetCurrentFrameData();
        return GetDeviceAddressOrZero(frameData.buffers.frameAddressTable);
    }
}
