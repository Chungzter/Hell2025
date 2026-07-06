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
#include "Unloved/Render/API/Vulkan/VK_draw.h"
#include "Unloved/Render/API/Vulkan/VK_push_constants.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Render/RendererConstants.h"
#include "Unloved/Viewport/ViewportManager.h"

#include <glm/matrix.hpp>
#include <algorithm>
#include <array>
#include <unordered_set>
#include <vector>

using namespace Unloved;

namespace VulkanRenderer {
    namespace {
        bool g_rayQueryReady = false;

        struct RayQueryInstanceData {
            uint32_t geometryDataOffset = 0;
            uint32_t geometryDataCount = 0;
            uint32_t padding0 = 0;
            uint32_t padding1 = 0;
        };

        struct RayQueryGeometryData {
            uint64_t vertexBufferDeviceAddress = 0;
            uint64_t indexBufferDeviceAddress = 0;
            uint32_t baseVertex = 0;
            uint32_t baseIndex = 0;
            uint32_t vertexCount = 0;
            uint32_t indexCount = 0;
            uint32_t blendingMode = 0;
            int32_t materialIndex = -1;
        };

        struct StaticRayQueryBuild {
            uint64_t blasId = 0;
            uint64_t vertexBufferAddress = 0;
            uint64_t indexBufferAddress = 0;
            RayQueryGeometryRange range;
            VkBuildAccelerationStructureFlagsKHR flags = 0;
            VkDeviceSize scratchSize = 0;
            VkDeviceSize scratchOffset = 0;
        };

        struct SkinnedRayQueryBuild {
            uint64_t blasId = 0;
            size_t slotIndex = 0;
            std::vector<RayQueryGeometryRange> ranges;
            VkDeviceSize scratchSize = 0;
            VkDeviceSize scratchOffset = 0;
            bool update = false;
        };

        struct BottomLevelBuildBatch {
            std::vector<VkAccelerationStructureGeometryKHR> geometries;
            std::vector<VkAccelerationStructureBuildGeometryInfoKHR> buildInfos;
            std::vector<VkAccelerationStructureBuildRangeInfoKHR> rangeInfos;
            std::vector<const VkAccelerationStructureBuildRangeInfoKHR*> rangeInfoPtrs;
            std::vector<uint64_t> blasIds;
            std::vector<size_t> skinnedSlotIndices;

            void Clear() {
                geometries.clear();
                buildInfos.clear();
                rangeInfos.clear();
                rangeInfoPtrs.clear();
                blasIds.clear();
                skinnedSlotIndices.clear();
            }

            void Reserve(size_t buildCount, size_t geometryCount) {
                geometries.reserve(geometryCount);
                buildInfos.reserve(buildCount);
                rangeInfos.reserve(geometryCount);
                rangeInfoPtrs.reserve(buildCount);
                blasIds.reserve(buildCount);
                skinnedSlotIndices.reserve(buildCount);
            }
        };

        std::vector<VkAccelerationStructureInstanceKHR> g_instances;
        std::vector<RayQueryInstanceData> g_rayQueryInstanceData;
        std::vector<RayQueryGeometryData> g_rayQueryGeometryData;
        std::vector<StaticRayQueryBuild> g_staticBuilds;
        std::vector<SkinnedRayQueryBuild> g_skinnedBuilds;
        std::unordered_set<uint32_t> g_proceduralMeshIds;
        std::unordered_set<uint64_t> g_queuedStaticBLASIds;
        BottomLevelBuildBatch g_bottomLevelBuildBatch;

        constexpr size_t NO_SKINNED_SLOT = static_cast<size_t>(-1);

        VkDeviceSize AlignUp(VkDeviceSize value, VkDeviceSize alignment) {
            if (alignment <= 1) return value;
            return ((value + alignment - 1) / alignment) * alignment;
        }

        VkDeviceSize AccelerationStructureScratchAlignment() {
            VkDeviceSize alignment = VulkanDeviceManager::GetAccelerationStructureProperties().minAccelerationStructureScratchOffsetAlignment;
            return alignment == 0 ? 1 : alignment;
        }

        VkBuildAccelerationStructureFlagsKHR SkinnedBLASBuildFlags() {
            return VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR;
        }

        VkBuildAccelerationStructureFlagsKHR StaticBLASBuildFlags() {
            return VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        }

        VkTransformMatrixKHR IdentityTransformMatrixKHR() {
            VkTransformMatrixKHR transform{};
            transform.matrix[0][0] = 1.0f;
            transform.matrix[1][1] = 1.0f;
            transform.matrix[2][2] = 1.0f;
            return transform;
        }

        VkTransformMatrixKHR TransformMatrixKHR(const glm::mat4& matrix) {
            VkTransformMatrixKHR transform{};
            glm::mat4 transposed = glm::transpose(matrix);

            for (int x = 0; x < 3; x++) {
                for (int y = 0; y < 4; y++) {
                    transform.matrix[x][y] = transposed[x][y];
                }
            }

            return transform;
        }

        VkAccelerationStructureInstanceKHR CreateTLASInstance(uint64_t accelerationStructureAddress, VkTransformMatrixKHR transform, uint32_t instanceCustomIndex) {
            VkAccelerationStructureInstanceKHR instance{};
            instance.transform = transform;
            instance.instanceCustomIndex = instanceCustomIndex;
            instance.mask = 0xff;
            instance.instanceShaderBindingTableRecordOffset = 0;
            instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
            instance.accelerationStructureReference = accelerationStructureAddress;
            return instance;
        }

        bool SkinnedRenderItemCastsPointLightShadow(const RenderItem& renderItem) {
            return renderItem.shadowBit == SHADOW_BIT_NONE || (renderItem.shadowBit & SHADOW_BIT_CAST_SHADOW) != 0u;
        }

        RayQueryGeometryRange CreateRayQueryRange(const Mesh& mesh, const RenderItem& renderItem, uint32_t baseVertex) {
            RayQueryGeometryRange range{};
            range.baseVertex = baseVertex;
            range.baseIndex = renderItem.baseIndex;
            range.vertexCount = mesh.vertexCount;
            range.indexCount = mesh.indexCount;
            range.blendingMode = renderItem.blendingMode;
            range.materialIndex = renderItem.materialIndex;
            return range;
        }

        RayQueryGeometryData CreateRayQueryGeometryData(uint64_t vertexBufferAddress, uint64_t indexBufferAddress, const RayQueryGeometryRange& range) {
            RayQueryGeometryData data{};
            data.vertexBufferDeviceAddress = vertexBufferAddress;
            data.indexBufferDeviceAddress = indexBufferAddress;
            data.baseVertex = range.baseVertex;
            data.baseIndex = range.baseIndex;
            data.vertexCount = range.vertexCount;
            data.indexCount = range.indexCount;
            data.blendingMode = range.blendingMode;
            data.materialIndex = range.materialIndex;
            return data;
        }

        void AddRayQueryInstance(std::vector<VkAccelerationStructureInstanceKHR>& instances, std::vector<RayQueryInstanceData>& instanceData, std::vector<RayQueryGeometryData>& geometryData, uint64_t blasDeviceAddress, VkTransformMatrixKHR transform, uint64_t vertexBufferAddress, uint64_t indexBufferAddress, const RayQueryGeometryRange& range) {
            uint32_t instanceCustomIndex = static_cast<uint32_t>(instanceData.size());

            RayQueryInstanceData& data = instanceData.emplace_back();
            data.geometryDataOffset = static_cast<uint32_t>(geometryData.size());
            data.geometryDataCount = 1;

            geometryData.push_back(CreateRayQueryGeometryData(vertexBufferAddress, indexBufferAddress, range));
            instances.push_back(CreateTLASInstance(blasDeviceAddress, transform, instanceCustomIndex));
        }

        void AddRayQueryInstance(std::vector<VkAccelerationStructureInstanceKHR>& instances, std::vector<RayQueryInstanceData>& instanceData, std::vector<RayQueryGeometryData>& geometryData, uint64_t blasDeviceAddress, VkTransformMatrixKHR transform, uint64_t vertexBufferAddress, uint64_t indexBufferAddress, const std::vector<RayQueryGeometryRange>& ranges) {
            if (ranges.empty()) return;

            uint32_t instanceCustomIndex = static_cast<uint32_t>(instanceData.size());

            RayQueryInstanceData& data = instanceData.emplace_back();
            data.geometryDataOffset = static_cast<uint32_t>(geometryData.size());
            data.geometryDataCount = static_cast<uint32_t>(ranges.size());

            geometryData.reserve(geometryData.size() + ranges.size());
            for (const RayQueryGeometryRange& range : ranges) {
                geometryData.push_back(CreateRayQueryGeometryData(vertexBufferAddress, indexBufferAddress, range));
            }

            instances.push_back(CreateTLASInstance(blasDeviceAddress, transform, instanceCustomIndex));
        }

        VkAccelerationStructureGeometryKHR CreateTriangleGeometry(uint64_t vertexBufferAddress, uint64_t indexBufferAddress, const RayQueryGeometryRange& range, uint64_t transformAddress = 0) {
            VkDeviceOrHostAddressConstKHR vertexBufferDeviceAddress{};
            vertexBufferDeviceAddress.deviceAddress = vertexBufferAddress + static_cast<uint64_t>(range.baseVertex) * sizeof(Vertex);

            VkDeviceOrHostAddressConstKHR indexBufferDeviceAddress{};
            indexBufferDeviceAddress.deviceAddress = indexBufferAddress + static_cast<uint64_t>(range.baseIndex) * sizeof(uint32_t);

            VkDeviceOrHostAddressConstKHR transformBufferDeviceAddress{};
            transformBufferDeviceAddress.deviceAddress = transformAddress;

            VkAccelerationStructureGeometryKHR geometry{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
            geometry.flags = 0;
            geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
            geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
            geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
            geometry.geometry.triangles.vertexData = vertexBufferDeviceAddress;
            geometry.geometry.triangles.maxVertex = range.vertexCount - 1;
            geometry.geometry.triangles.vertexStride = sizeof(Vertex);
            geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
            geometry.geometry.triangles.indexData = indexBufferDeviceAddress;
            geometry.geometry.triangles.transformData = transformBufferDeviceAddress;
            return geometry;
        }

        VkAccelerationStructureBuildSizesInfoKHR QueryBottomLevelBuildSize(uint64_t vertexBufferAddress, uint64_t indexBufferAddress, const RayQueryGeometryRange& range, VkBuildAccelerationStructureFlagsKHR flags) {
            VkAccelerationStructureGeometryKHR geometry = CreateTriangleGeometry(vertexBufferAddress, indexBufferAddress, range);

            VkAccelerationStructureBuildGeometryInfoKHR buildInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
            buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
            buildInfo.flags = flags;
            buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
            buildInfo.geometryCount = 1;
            buildInfo.pGeometries = &geometry;

            uint32_t primitiveCount = range.indexCount / 3;
            VkAccelerationStructureBuildSizesInfoKHR sizeInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
            vkGetAccelerationStructureBuildSizesKHR(VulkanDeviceManager::GetDevice(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo, &primitiveCount, &sizeInfo);
            return sizeInfo;
        }

        VkAccelerationStructureBuildSizesInfoKHR QueryBottomLevelBuildSize(uint64_t vertexBufferAddress, uint64_t indexBufferAddress, const std::vector<RayQueryGeometryRange>& ranges, VkBuildAccelerationStructureFlagsKHR flags) {
            VkAccelerationStructureBuildSizesInfoKHR sizeInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
            if (ranges.empty()) return sizeInfo;

            std::vector<VkAccelerationStructureGeometryKHR> geometries;
            std::vector<uint32_t> primitiveCounts;
            geometries.reserve(ranges.size());
            primitiveCounts.reserve(ranges.size());

            for (const RayQueryGeometryRange& range : ranges) {
                geometries.push_back(CreateTriangleGeometry(vertexBufferAddress, indexBufferAddress, range));
                primitiveCounts.push_back(range.indexCount / 3);
            }

            VkAccelerationStructureBuildGeometryInfoKHR buildInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
            buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
            buildInfo.flags = flags;
            buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
            buildInfo.geometryCount = static_cast<uint32_t>(geometries.size());
            buildInfo.pGeometries = geometries.data();

            vkGetAccelerationStructureBuildSizesKHR(VulkanDeviceManager::GetDevice(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo, primitiveCounts.data(), &sizeInfo);
            return sizeInfo;
        }

        uint64_t HashRayQueryRanges(const std::vector<RayQueryGeometryRange>& ranges) {
            uint64_t hash = 1469598103934665603ull;
            auto mix = [&](uint64_t value) {
                hash ^= value;
                hash *= 1099511628211ull;
                };

            mix(ranges.size());
            for (const RayQueryGeometryRange& range : ranges) {
                mix(range.baseVertex);
                mix(range.baseIndex);
                mix(range.vertexCount);
                mix(range.indexCount);
            }

            return hash;
        }

        bool SkinnedBLASSlotMatches(const VulkanFrameData::AccelerationStructures::SkinnedBLASSlot& slot, const std::vector<RayQueryGeometryRange>& ranges) {
            return slot.built &&
                slot.geometryCount == static_cast<uint32_t>(ranges.size()) &&
                slot.geometryHash == HashRayQueryRanges(ranges) &&
                slot.accelerationStructureSize != 0 &&
                (slot.updateScratchSize != 0 || slot.buildScratchSize != 0);
        }

        void StoreSkinnedBLASSlotMetadata(VulkanFrameData::AccelerationStructures::SkinnedBLASSlot& slot, const std::vector<RayQueryGeometryRange>& ranges, const VkAccelerationStructureBuildSizesInfoKHR& sizeInfo) {
            if (!ranges.empty()) {
                slot.baseVertex = ranges[0].baseVertex;
                slot.baseIndex = ranges[0].baseIndex;
                slot.vertexCount = ranges[0].vertexCount;
                slot.indexCount = ranges[0].indexCount;
            }

            slot.geometryCount = static_cast<uint32_t>(ranges.size());
            slot.geometryHash = HashRayQueryRanges(ranges);
            slot.accelerationStructureSize = sizeInfo.accelerationStructureSize;
            slot.buildScratchSize = sizeInfo.buildScratchSize;
            slot.updateScratchSize = sizeInfo.updateScratchSize;
            slot.built = false;
        }

        bool GeometryRangeFitsBuffers(const RayQueryGeometryRange& range, VkDeviceSize vertexBufferSize, VkDeviceSize indexBufferSize) {
            if (range.vertexCount == 0 || range.indexCount < 3) return false;

            uint64_t vertexEnd = static_cast<uint64_t>(range.baseVertex) + static_cast<uint64_t>(range.vertexCount);
            uint64_t indexEnd = static_cast<uint64_t>(range.baseIndex) + static_cast<uint64_t>(range.indexCount);
            uint64_t vertexCapacity = vertexBufferSize / sizeof(Vertex);
            uint64_t indexCapacity = indexBufferSize / sizeof(uint32_t);
            return vertexEnd <= vertexCapacity && indexEnd <= indexCapacity;
        }

        void DestroySkinnedBLASSlot(VulkanFrameData::AccelerationStructures::SkinnedBLASSlot& slot) {
            if (slot.id != 0 && VulkanResourceManager::AccelerationStructureExists(slot.id)) {
                VulkanResourceManager::RemoveAccelerationStructure(slot.id);
            }

            slot = {};
        }

        void DestroySkinnedBLASSlots(std::vector<VulkanFrameData::AccelerationStructures::SkinnedBLASSlot>& slots, size_t firstSlot) {
            if (firstSlot >= slots.size()) return;

            for (size_t i = firstSlot; i < slots.size(); i++) {
                DestroySkinnedBLASSlot(slots[i]);
            }

            slots.resize(firstSlot);
        }

        bool PrepareAccelerationStructure(uint64_t id, VkAccelerationStructureTypeKHR type, VkAccelerationStructureBuildSizesInfoKHR sizeInfo) {
            VulkanAccelerationStructure* accelerationStructure = VulkanResourceManager::GetAccelerationStructure(id);
            if (!accelerationStructure) return false;

            VkDevice device = VulkanDeviceManager::GetDevice();
            accelerationStructure->Cleanup();
            accelerationStructure->CreateBuffer(sizeInfo);

            VkAccelerationStructureCreateInfoKHR createInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
            createInfo.buffer = accelerationStructure->GetBuffer();
            createInfo.size = sizeInfo.accelerationStructureSize;
            createInfo.type = type;
            if (vkCreateAccelerationStructureKHR(device, &createInfo, nullptr, &accelerationStructure->m_handle) != VK_SUCCESS) return false;

            VkAccelerationStructureDeviceAddressInfoKHR addressInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR };
            addressInfo.accelerationStructure = accelerationStructure->m_handle;
            accelerationStructure->m_deviceAddress = vkGetAccelerationStructureDeviceAddressKHR(device, &addressInfo);
            return accelerationStructure->m_deviceAddress != 0;
        }

        void RecordAccelerationStructureBuildBarrier(VkCommandBuffer commandBuffer, VkPipelineStageFlags dstStageMask, VkAccessFlags dstAccessMask) {
            VkMemoryBarrier barrier{ VK_STRUCTURE_TYPE_MEMORY_BARRIER };
            barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
            barrier.dstAccessMask = dstAccessMask;
            vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, dstStageMask, 0, 1, &barrier, 0, nullptr, 0, nullptr);
        }

        void QueueBottomLevelBuild(BottomLevelBuildBatch& batch, uint64_t blasId, uint64_t vertexBufferAddress, uint64_t indexBufferAddress, const RayQueryGeometryRange& range, VkBuildAccelerationStructureFlagsKHR flags, uint64_t scratchBufferAddress, bool update, size_t skinnedSlotIndex) {
            VulkanAccelerationStructure* accelerationStructure = VulkanResourceManager::GetAccelerationStructure(blasId);
            if (!accelerationStructure || accelerationStructure->GetHandle() == VK_NULL_HANDLE) return;

            size_t geometryOffset = batch.geometries.size();
            size_t rangeInfoOffset = batch.rangeInfos.size();
            batch.geometries.emplace_back(CreateTriangleGeometry(vertexBufferAddress, indexBufferAddress, range));

            VkAccelerationStructureBuildGeometryInfoKHR buildInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
            buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
            buildInfo.flags = flags;
            buildInfo.mode = update ? VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR : VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
            buildInfo.srcAccelerationStructure = update ? accelerationStructure->GetHandle() : VK_NULL_HANDLE;
            buildInfo.dstAccelerationStructure = accelerationStructure->GetHandle();
            buildInfo.geometryCount = 1;
            buildInfo.pGeometries = batch.geometries.data() + geometryOffset;
            buildInfo.scratchData.deviceAddress = scratchBufferAddress;
            batch.buildInfos.push_back(buildInfo);

            VkAccelerationStructureBuildRangeInfoKHR& rangeInfo = batch.rangeInfos.emplace_back();
            rangeInfo.primitiveCount = range.indexCount / 3;
            rangeInfo.primitiveOffset = 0;
            rangeInfo.firstVertex = 0;
            rangeInfo.transformOffset = 0;

            batch.rangeInfoPtrs.push_back(batch.rangeInfos.data() + rangeInfoOffset);
            batch.blasIds.push_back(blasId);
            batch.skinnedSlotIndices.push_back(skinnedSlotIndex);
        }

        void QueueBottomLevelBuild(BottomLevelBuildBatch& batch, uint64_t blasId, uint64_t vertexBufferAddress, uint64_t indexBufferAddress, const std::vector<RayQueryGeometryRange>& ranges, VkBuildAccelerationStructureFlagsKHR flags, uint64_t scratchBufferAddress, bool update, size_t skinnedSlotIndex, uint64_t transformBufferAddress = 0) {
            VulkanAccelerationStructure* accelerationStructure = VulkanResourceManager::GetAccelerationStructure(blasId);
            if (!accelerationStructure || accelerationStructure->GetHandle() == VK_NULL_HANDLE || ranges.empty()) return;

            size_t geometryOffset = batch.geometries.size();
            size_t rangeInfoOffset = batch.rangeInfos.size();

            for (size_t i = 0; i < ranges.size(); i++) {
                const RayQueryGeometryRange& range = ranges[i];
                uint64_t transformAddress = transformBufferAddress != 0 ? transformBufferAddress + i * sizeof(VkTransformMatrixKHR) : 0;
                batch.geometries.emplace_back(CreateTriangleGeometry(vertexBufferAddress, indexBufferAddress, range, transformAddress));

                VkAccelerationStructureBuildRangeInfoKHR& rangeInfo = batch.rangeInfos.emplace_back();
                rangeInfo.primitiveCount = range.indexCount / 3;
                rangeInfo.primitiveOffset = 0;
                rangeInfo.firstVertex = 0;
                rangeInfo.transformOffset = 0;
            }

            VkAccelerationStructureBuildGeometryInfoKHR buildInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
            buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
            buildInfo.flags = flags;
            buildInfo.mode = update ? VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR : VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
            buildInfo.srcAccelerationStructure = update ? accelerationStructure->GetHandle() : VK_NULL_HANDLE;
            buildInfo.dstAccelerationStructure = accelerationStructure->GetHandle();
            buildInfo.geometryCount = static_cast<uint32_t>(ranges.size());
            buildInfo.pGeometries = batch.geometries.data() + geometryOffset;
            buildInfo.scratchData.deviceAddress = scratchBufferAddress;
            batch.buildInfos.push_back(buildInfo);

            batch.rangeInfoPtrs.push_back(batch.rangeInfos.data() + rangeInfoOffset);
            batch.blasIds.push_back(blasId);
            batch.skinnedSlotIndices.push_back(skinnedSlotIndex);
        }

        bool RecordBottomLevelBuildBatch(VkCommandBuffer commandBuffer, BottomLevelBuildBatch& batch, VulkanFrameData& frameData) {
            if (batch.buildInfos.empty()) return false;

            {
                vkCmdBuildAccelerationStructuresKHR(commandBuffer, static_cast<uint32_t>(batch.buildInfos.size()), batch.buildInfos.data(), batch.rangeInfoPtrs.data());
            }

            {
                for (size_t i = 0; i < batch.blasIds.size(); i++) {
                    VulkanAccelerationStructure* accelerationStructure = VulkanResourceManager::GetAccelerationStructure(batch.blasIds[i]);
                    if (accelerationStructure) {
                        accelerationStructure->m_built = true;
                    }

                    size_t slotIndex = batch.skinnedSlotIndices[i];
                    if (slotIndex != NO_SKINNED_SLOT && slotIndex < frameData.accelerationStructures.skinnedBLAS.size()) {
                        frameData.accelerationStructures.skinnedBLAS[slotIndex].built = true;
                    }
                }
            }

            return true;
        }

        VkAccelerationStructureBuildSizesInfoKHR QueryTopLevelBuildSize(uint64_t instanceBufferAddress, uint32_t instanceCount) {
            VkAccelerationStructureGeometryInstancesDataKHR instancesData{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR };
            instancesData.arrayOfPointers = VK_FALSE;
            instancesData.data.deviceAddress = instanceBufferAddress;

            VkAccelerationStructureGeometryKHR geometry{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
            geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
            geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
            geometry.geometry.instances = instancesData;

            VkAccelerationStructureBuildGeometryInfoKHR buildInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
            buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
            buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
            buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
            buildInfo.geometryCount = 1;
            buildInfo.pGeometries = &geometry;

            VkAccelerationStructureBuildSizesInfoKHR sizeInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
            vkGetAccelerationStructureBuildSizesKHR(VulkanDeviceManager::GetDevice(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo, &instanceCount, &sizeInfo);
            return sizeInfo;
        }

        void RecordTopLevelBuild(VkCommandBuffer commandBuffer, uint64_t tlasId, uint64_t instanceBufferAddress, uint32_t instanceCount, uint64_t scratchBufferAddress) {
            VulkanAccelerationStructure* accelerationStructure = VulkanResourceManager::GetAccelerationStructure(tlasId);
            if (!accelerationStructure || accelerationStructure->GetHandle() == VK_NULL_HANDLE) return;

            VkAccelerationStructureGeometryInstancesDataKHR instancesData{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR };
            instancesData.arrayOfPointers = VK_FALSE;
            instancesData.data.deviceAddress = instanceBufferAddress;

            VkAccelerationStructureGeometryKHR geometry{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
            geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
            geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
            geometry.geometry.instances = instancesData;

            VkAccelerationStructureBuildGeometryInfoKHR buildInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
            buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
            buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
            buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
            buildInfo.dstAccelerationStructure = accelerationStructure->GetHandle();
            buildInfo.geometryCount = 1;
            buildInfo.pGeometries = &geometry;
            buildInfo.scratchData.deviceAddress = scratchBufferAddress;

            VkAccelerationStructureBuildRangeInfoKHR rangeInfo{ instanceCount, 0, 0, 0 };
            const VkAccelerationStructureBuildRangeInfoKHR* rangeInfoPtr = &rangeInfo;
            vkCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &buildInfo, &rangeInfoPtr);
            accelerationStructure->m_built = true;
        }
    }

    void UpdateRayQueryAccelerationStructures(VkCommandBuffer commandBuffer) {
        ProfilerVulkanZoneFunction();

        g_rayQueryReady = false;

        VulkanFrameData& frameData = GetCurrentFrameData();
        if (frameData.accelerationStructures.rayQueryTLAS == 0) {
            frameData.accelerationStructures.rayQueryTLAS = VulkanResourceManager::CreateAccelerationStructure();
        }

        Hell::MeshBuffer* assetMeshBuffer = Hell::ResourceManager::GetMeshBufferPtr("AssetGeometry");
        Hell::MeshBuffer* proceduralMeshBuffer = Hell::ResourceManager::GetMeshBufferPtr("Procedural");
        VulkanMeshBuffer* assetVulkanMeshBuffer = VulkanResourceManager::GetMeshBuffer("AssetGeometry");
        VulkanMeshBuffer* proceduralVulkanMeshBuffer = VulkanResourceManager::GetMeshBuffer("Procedural");
        VulkanDescriptorSet* staticDescriptorSet = VulkanResourceManager::GetDescriptorSet("StaticDescriptorSet");
        VulkanBuffer* skinnedVertexBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.skinnedVertices);
        VulkanBuffer* rayQueryInstanceBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.rayQueryInstances);
        VulkanBuffer* rayQueryInstanceDataBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.rayQueryInstanceData);
        VulkanBuffer* rayQueryGeometryDataBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.rayQueryGeometryData);
        VulkanBuffer* rayQueryScratchBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.rayQueryScratch);
        
        if (!assetMeshBuffer) return;
        if (!proceduralMeshBuffer) return;
        if (!assetVulkanMeshBuffer) return;
        if (!proceduralVulkanMeshBuffer) return;
        if (!staticDescriptorSet) return;
        if (!rayQueryInstanceBuffer) return;
        if (!rayQueryInstanceDataBuffer) return;
        if (!rayQueryGeometryDataBuffer) return;
        if (!rayQueryScratchBuffer) return;

        g_instances.clear();
        g_rayQueryInstanceData.clear();
        g_rayQueryGeometryData.clear();
        g_staticBuilds.clear();
        g_skinnedBuilds.clear();
        g_proceduralMeshIds.clear();
        g_queuedStaticBLASIds.clear();
        g_bottomLevelBuildBatch.Clear();

        VkDeviceSize scratchAlignment = AccelerationStructureScratchAlignment();
        VkDeviceSize blasScratchArenaSize = 0;

        auto allocateBuildScratch = [&](VkDeviceSize scratchSize) {
            VkDeviceSize scratchOffset = AlignUp(blasScratchArenaSize, scratchAlignment);
            blasScratchArenaSize = scratchOffset + scratchSize;
            return scratchOffset;
            };

        auto queueStaticBLASBuild = [&](Mesh* mesh, uint64_t vertexBufferAddress, uint64_t indexBufferAddress, const RayQueryGeometryRange& range) -> VulkanAccelerationStructure* {
            if (!mesh || mesh->vertexCount == 0 || mesh->indexCount < 3) return nullptr;
            if (vertexBufferAddress == 0 || indexBufferAddress == 0) return nullptr;

            if (mesh->vulkanBlasId == 0 || !VulkanResourceManager::AccelerationStructureExists(mesh->vulkanBlasId)) {
                mesh->vulkanBlasId = VulkanResourceManager::CreateAccelerationStructure();
            }

            VulkanAccelerationStructure* blas = VulkanResourceManager::GetAccelerationStructure(mesh->vulkanBlasId);
            if (!blas) return nullptr;

            if (blas->GetHandle() == VK_NULL_HANDLE || blas->GetDeviceAddress() == 0 || !blas->m_built) {
                if (g_queuedStaticBLASIds.find(mesh->vulkanBlasId) != g_queuedStaticBLASIds.end()) return blas;

                VkAccelerationStructureBuildSizesInfoKHR sizeInfo = QueryBottomLevelBuildSize(vertexBufferAddress, indexBufferAddress, range, StaticBLASBuildFlags());
                if (sizeInfo.accelerationStructureSize == 0) return nullptr;
                if (!PrepareAccelerationStructure(mesh->vulkanBlasId, VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR, sizeInfo)) return nullptr;

                blas = VulkanResourceManager::GetAccelerationStructure(mesh->vulkanBlasId);
                if (!blas || blas->GetDeviceAddress() == 0) return nullptr;

                StaticRayQueryBuild& build = g_staticBuilds.emplace_back();
                build.blasId = mesh->vulkanBlasId;
                build.vertexBufferAddress = vertexBufferAddress;
                build.indexBufferAddress = indexBufferAddress;
                build.range = range;
                build.flags = StaticBLASBuildFlags();
                build.scratchSize = sizeInfo.buildScratchSize;
                build.scratchOffset = allocateBuildScratch(build.scratchSize);
                g_queuedStaticBLASIds.insert(mesh->vulkanBlasId);
            }

            return blas;
            };

        const std::vector<RenderItem>& proceduralRenderItems = Unloved::RenderDataManager::GetRenderItemsProcedural();
        const std::vector<RenderItem>& assetShadowRenderItems = Unloved::RenderDataManager::GetRenderItemsPointLightShadows();
        const std::vector<RayQuerySkinnedGroup>& rayQuerySkinnedGroups = Unloved::RenderDataManager::GetRayQuerySkinnedGroups();
        const std::vector<RenderItem>& skinnedNonDeformingRenderItems = Unloved::RenderDataManager::GetNonDeformingSkinnedMeshRenderItems();
        const std::vector<RenderItem>& skinnedNonDeformingAlphaDiscardRenderItems = Unloved::RenderDataManager::GetNonDeformingSkinnedMeshRenderItemsAlphaDiscard();
        size_t staticAssetRenderItemCount = assetShadowRenderItems.size() + skinnedNonDeformingRenderItems.size() + skinnedNonDeformingAlphaDiscardRenderItems.size();
        size_t rayQuerySkinnedRangeCount = 0;
        for (const RayQuerySkinnedGroup& group : rayQuerySkinnedGroups) {
            rayQuerySkinnedRangeCount += group.ranges.size();
        }

        g_instances.reserve(proceduralRenderItems.size() + staticAssetRenderItemCount + rayQuerySkinnedGroups.size());
        g_rayQueryInstanceData.reserve(g_instances.capacity());
        g_rayQueryGeometryData.reserve(g_instances.capacity() + rayQuerySkinnedRangeCount);
        g_staticBuilds.reserve(proceduralRenderItems.size() + staticAssetRenderItemCount);
        g_queuedStaticBLASIds.reserve(proceduralRenderItems.size() + staticAssetRenderItemCount);

        g_proceduralMeshIds.reserve(proceduralRenderItems.size());
        uint64_t proceduralVertexBufferAddress = proceduralVulkanMeshBuffer->GetVertexBufferAddress();
        uint64_t proceduralIndexBufferAddress = proceduralVulkanMeshBuffer->GetIndexBufferAddress();

        {
            for (const RenderItem& renderItem : proceduralRenderItems) {
                if (!g_proceduralMeshIds.insert(renderItem.meshId).second) continue;

                Mesh* mesh = proceduralMeshBuffer->GetMeshById(renderItem.meshId);
                if (!mesh || mesh->vertexCount == 0 || mesh->indexCount < 3) continue;

                RayQueryGeometryRange range = CreateRayQueryRange(*mesh, renderItem, renderItem.baseVertex);
                VulkanAccelerationStructure* blas = queueStaticBLASBuild(mesh, proceduralVertexBufferAddress, proceduralIndexBufferAddress, range);
                if (blas && blas->GetHandle() != VK_NULL_HANDLE && blas->GetDeviceAddress() != 0) {
                    AddRayQueryInstance(g_instances, g_rayQueryInstanceData, g_rayQueryGeometryData, blas->GetDeviceAddress(), IdentityTransformMatrixKHR(), proceduralVertexBufferAddress, proceduralIndexBufferAddress, range);
                }
            }
        }

        uint64_t assetVertexBufferAddress = assetVulkanMeshBuffer->GetVertexBufferAddress();
        uint64_t assetIndexBufferAddress = assetVulkanMeshBuffer->GetIndexBufferAddress();

        auto collectAssetBackedStaticRenderItems = [&](const std::vector<RenderItem>& renderItems, bool useSkinnedShadowRule, bool shadowFiltered) {
            for (const RenderItem& renderItem : renderItems) {
                if (useSkinnedShadowRule) {
                    if (!SkinnedRenderItemCastsPointLightShadow(renderItem)) continue;
                }
                else if (!shadowFiltered && (renderItem.shadowBit & SHADOW_BIT_CAST_SHADOW) == 0u) {
                    continue;
                }

                Mesh* mesh = assetMeshBuffer->GetMeshById(renderItem.meshId);
                if (!mesh || mesh->vertexCount == 0 || mesh->indexCount < 3) continue;

                RayQueryGeometryRange range = CreateRayQueryRange(*mesh, renderItem, renderItem.baseVertex);
                VulkanAccelerationStructure* blas = queueStaticBLASBuild(mesh, assetVertexBufferAddress, assetIndexBufferAddress, range);
                if (!blas || blas->GetHandle() == VK_NULL_HANDLE || blas->GetDeviceAddress() == 0) continue;

                AddRayQueryInstance(g_instances, g_rayQueryInstanceData, g_rayQueryGeometryData, blas->GetDeviceAddress(), TransformMatrixKHR(renderItem.modelMatrix), assetVertexBufferAddress, assetIndexBufferAddress, range);
            }
            };

        {
            collectAssetBackedStaticRenderItems(assetShadowRenderItems, false, true);
        }

        {
            collectAssetBackedStaticRenderItems(skinnedNonDeformingRenderItems, true, false);
        }

        {
            collectAssetBackedStaticRenderItems(skinnedNonDeformingAlphaDiscardRenderItems, true, false);
        }

        g_skinnedBuilds.reserve(rayQuerySkinnedGroups.size());

        size_t activeSkinnedSlotCount = 0;
        uint64_t skinnedVertexBufferAddress = skinnedVertexBuffer ? skinnedVertexBuffer->GetDeviceAddress() : 0;
        VulkanBuffer* assetIndexBuffer = assetVulkanMeshBuffer->GetIndexBuffer();

        if (skinnedVertexBufferAddress != frameData.accelerationStructures.skinnedVertexBufferAddress) {
            DestroySkinnedBLASSlots(frameData.accelerationStructures.skinnedBLAS, 0);
            frameData.accelerationStructures.skinnedVertexBufferAddress = skinnedVertexBufferAddress;
        }

        if (skinnedVertexBuffer && skinnedVertexBufferAddress != 0 && assetIndexBuffer) {
            VkDeviceSize skinnedVertexBufferSize = skinnedVertexBuffer->GetSize();
            VkDeviceSize assetIndexBufferSize = assetIndexBuffer->GetSize();

            for (const RayQuerySkinnedGroup& group : rayQuerySkinnedGroups) {
                if (group.ranges.empty()) continue;

                std::vector<RayQueryGeometryRange> ranges;
                ranges.reserve(group.ranges.size());
                for (const RayQueryGeometryRange& range : group.ranges) {
                    if (GeometryRangeFitsBuffers(range, skinnedVertexBufferSize, assetIndexBufferSize)) {
                        ranges.push_back(range);
                    }
                }
                if (ranges.empty()) continue;

                size_t blasSlot = activeSkinnedSlotCount;
                if (frameData.accelerationStructures.skinnedBLAS.size() <= blasSlot) {
                    frameData.accelerationStructures.skinnedBLAS.resize(blasSlot + 1);
                }

                VulkanFrameData::AccelerationStructures::SkinnedBLASSlot& slot = frameData.accelerationStructures.skinnedBLAS[blasSlot];
                if (slot.id == 0) {
                    slot.id = VulkanResourceManager::CreateAccelerationStructure();
                }

                VulkanAccelerationStructure* blas = VulkanResourceManager::GetAccelerationStructure(slot.id);
                bool slotHasUsableBLAS = blas && blas->GetHandle() != VK_NULL_HANDLE && blas->GetDeviceAddress() != 0 && blas->m_built;
                bool update = slotHasUsableBLAS && SkinnedBLASSlotMatches(slot, ranges);
                VkDeviceSize scratchSize = update ? (slot.updateScratchSize != 0 ? slot.updateScratchSize : slot.buildScratchSize) : 0;
                bool skinnedBLASReady = true;

                if (!update) {
                    VkAccelerationStructureBuildSizesInfoKHR sizeInfo = QueryBottomLevelBuildSize(skinnedVertexBufferAddress, assetVulkanMeshBuffer->GetIndexBufferAddress(), ranges, SkinnedBLASBuildFlags());
                    if (sizeInfo.accelerationStructureSize == 0 || !PrepareAccelerationStructure(slot.id, VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR, sizeInfo)) {
                        skinnedBLASReady = false;
                    }
                    else {
                        StoreSkinnedBLASSlotMetadata(slot, ranges, sizeInfo);
                        scratchSize = sizeInfo.buildScratchSize;
                        blas = VulkanResourceManager::GetAccelerationStructure(slot.id);
                    }
                }

                if (skinnedBLASReady && blas && blas->GetDeviceAddress() != 0) {
                    activeSkinnedSlotCount++;

                    SkinnedRayQueryBuild& build = g_skinnedBuilds.emplace_back();
                    build.blasId = slot.id;
                    build.slotIndex = blasSlot;
                    build.ranges = ranges;
                    build.update = update;
                    build.scratchSize = scratchSize;
                    build.scratchOffset = allocateBuildScratch(build.scratchSize);
                    AddRayQueryInstance(g_instances, g_rayQueryInstanceData, g_rayQueryGeometryData, blas->GetDeviceAddress(), TransformMatrixKHR(group.modelMatrix), skinnedVertexBufferAddress, assetIndexBufferAddress, ranges);
                }
            }
        }

        DestroySkinnedBLASSlots(frameData.accelerationStructures.skinnedBLAS, activeSkinnedSlotCount);

        if (g_instances.empty()) return;

        VkDeviceSize instanceBufferSize = sizeof(VkAccelerationStructureInstanceKHR) * g_instances.size();
        VkDeviceSize rayQueryInstanceDataSize = sizeof(RayQueryInstanceData) * g_rayQueryInstanceData.size();
        VkDeviceSize rayQueryGeometryDataSize = sizeof(RayQueryGeometryData) * g_rayQueryGeometryData.size();
        {
            if (!EnsureBufferSize(frameData.buffers.rayQueryInstances, instanceBufferSize)) return;
            if (!EnsureBufferSize(frameData.buffers.rayQueryInstanceData, rayQueryInstanceDataSize)) return;
            if (!EnsureBufferSize(frameData.buffers.rayQueryGeometryData, rayQueryGeometryDataSize)) return;

            rayQueryInstanceBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.rayQueryInstances);
            rayQueryInstanceDataBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.rayQueryInstanceData);
            rayQueryGeometryDataBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.rayQueryGeometryData);
            if (!rayQueryInstanceBuffer) return;
            if (!rayQueryInstanceDataBuffer) return;
            if (!rayQueryGeometryDataBuffer) return;

            rayQueryInstanceBuffer->UpdateData(g_instances.data(), instanceBufferSize);
            rayQueryInstanceDataBuffer->UpdateData(g_rayQueryInstanceData.data(), rayQueryInstanceDataSize);
            rayQueryGeometryDataBuffer->UpdateData(g_rayQueryGeometryData.data(), rayQueryGeometryDataSize);

            VkBufferMemoryBarrier instanceUploadBarrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
            instanceUploadBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
            instanceUploadBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
            instanceUploadBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            instanceUploadBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            instanceUploadBarrier.buffer = rayQueryInstanceBuffer->GetBuffer();
            instanceUploadBarrier.offset = 0;
            instanceUploadBarrier.size = instanceBufferSize;
            vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 0, nullptr, 1, &instanceUploadBarrier, 0, nullptr);

            std::array<VkBufferMemoryBarrier, 2> metadataUploadBarriers{};
            for (VkBufferMemoryBarrier& barrier : metadataUploadBarriers) {
                barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
                barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.offset = 0;
            }
            metadataUploadBarriers[0].buffer = rayQueryInstanceDataBuffer->GetBuffer();
            metadataUploadBarriers[0].size = rayQueryInstanceDataSize;
            metadataUploadBarriers[1].buffer = rayQueryGeometryDataBuffer->GetBuffer();
            metadataUploadBarriers[1].size = rayQueryGeometryDataSize;
            vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, static_cast<uint32_t>(metadataUploadBarriers.size()), metadataUploadBarriers.data(), 0, nullptr);
        }

        uint32_t instanceCount = static_cast<uint32_t>(g_instances.size());
        VulkanAccelerationStructure* rayQueryTLAS = VulkanResourceManager::GetAccelerationStructure(frameData.accelerationStructures.rayQueryTLAS);
        bool tlasNeedsCreate =
            !rayQueryTLAS ||
            rayQueryTLAS->GetHandle() == VK_NULL_HANDLE ||
            rayQueryTLAS->GetDeviceAddress() == 0 ||
            instanceCount > frameData.accelerationStructures.rayQueryTLASInstanceCapacity;

        if (tlasNeedsCreate) {
            VkAccelerationStructureBuildSizesInfoKHR tlasSizeInfo = QueryTopLevelBuildSize(rayQueryInstanceBuffer->GetDeviceAddress(), instanceCount);
            if (!PrepareAccelerationStructure(frameData.accelerationStructures.rayQueryTLAS, VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR, tlasSizeInfo)) return;

            frameData.accelerationStructures.rayQueryTLASInstanceCapacity = instanceCount;
            frameData.accelerationStructures.rayQueryTLASScratchSize = tlasSizeInfo.buildScratchSize;
            rayQueryTLAS = VulkanResourceManager::GetAccelerationStructure(frameData.accelerationStructures.rayQueryTLAS);
        }

        if (!rayQueryTLAS || rayQueryTLAS->GetHandle() == VK_NULL_HANDLE || rayQueryTLAS->GetDeviceAddress() == 0) return;

        VkDeviceSize requiredScratchSize = std::max(blasScratchArenaSize, static_cast<VkDeviceSize>(frameData.accelerationStructures.rayQueryTLASScratchSize)) + scratchAlignment;
        {
            if (!EnsureBufferSize(frameData.buffers.rayQueryScratch, requiredScratchSize)) return;
            rayQueryScratchBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.rayQueryScratch);
            if (!rayQueryScratchBuffer) return;
        }

        uint64_t scratchBufferAddress = rayQueryScratchBuffer->GetDeviceAddress();
        uint64_t scratchBaseAddress = AlignUp(scratchBufferAddress, scratchAlignment);

        {
            {
                size_t blasBuildCount = g_staticBuilds.size() + g_skinnedBuilds.size();
                size_t blasGeometryCount = g_staticBuilds.size();
                for (const SkinnedRayQueryBuild& build : g_skinnedBuilds) {
                    blasGeometryCount += build.ranges.size();
                }

                g_bottomLevelBuildBatch.Reserve(blasBuildCount, blasGeometryCount);

                for (const StaticRayQueryBuild& build : g_staticBuilds) {
                    QueueBottomLevelBuild(g_bottomLevelBuildBatch, build.blasId, build.vertexBufferAddress, build.indexBufferAddress, build.range, build.flags, scratchBaseAddress + build.scratchOffset, false, NO_SKINNED_SLOT);
                }

                for (const SkinnedRayQueryBuild& build : g_skinnedBuilds) {
                    QueueBottomLevelBuild(g_bottomLevelBuildBatch, build.blasId, skinnedVertexBufferAddress, assetIndexBufferAddress, build.ranges, SkinnedBLASBuildFlags(), scratchBaseAddress + build.scratchOffset, build.update, build.slotIndex);
                }
            }

            bool recordedBLASBuilds = RecordBottomLevelBuildBatch(commandBuffer, g_bottomLevelBuildBatch, frameData);
            if (recordedBLASBuilds) {
                RecordAccelerationStructureBuildBarrier(commandBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR);
            }
        }

        {
            RecordTopLevelBuild(commandBuffer, frameData.accelerationStructures.rayQueryTLAS, rayQueryInstanceBuffer->GetDeviceAddress(), instanceCount, scratchBaseAddress);
            RecordAccelerationStructureBuildBarrier(commandBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR);
        }

        {
            staticDescriptorSet->WriteAccelerationStructure(DESC_IDX_ACCELERATION_STRUCTURES, rayQueryTLAS->GetHandle());
            staticDescriptorSet->Update();
        }
        g_rayQueryReady = true;
    }
}
