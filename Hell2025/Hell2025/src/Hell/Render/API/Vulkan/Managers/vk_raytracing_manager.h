#pragma once
#include "Hell/Render/API/Vulkan/vk_common.h"
#include "Hell/Render/API/Vulkan/vk_types.h"
#include "Hell/Render/API/Vulkan/Types/vk_acceleration_structure.h"
#include "Hell/Render/API/Vulkan/Types/vk_buffer.h"
#include "Hell/ResourceManagement/Types/Mesh.h"

#include <cstdint>
#include <vector>

namespace VulkanRaytracingManager {
    void CreateTLAS(uint64_t id, const std::vector<VkAccelerationStructureInstanceKHR>& instances);
    uint64_t CreateBottomLevelAS(Mesh* mesh);

    // Helpers now return the new VulkanBuffer class
    VulkanBuffer CreateScratchBuffer(VkDeviceSize size);
    //void CreateASBuffer(VulkanAccelerationStructure& as, VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo);

}
