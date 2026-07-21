#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Render/API/Vulkan/Types/vk_acceleration_structure.h"
#include "Hell/Render/API/Vulkan/Types/vk_buffer.h"
#include "Hell/Render/API/Vulkan/Types/vk_descriptor_set.h"
#include "Hell/Render/API/Vulkan/Types/vk_mesh_buffer.h"
#include "Hell/Render/VertexAttributes.h"

#include "Unloved/Render/API/Vulkan/RayTracing/VK_acceleration_structure_utils.h"
#include "Unloved/Render/API/Vulkan/RayTracing/VK_raytracing_scene.h"
#include "Unloved/Render/API/Vulkan/RayTracing/VK_transient_blas_builder.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace VulkanRenderer {
    namespace {
        struct QueuedMultiMeshBLASBuild {
            uint64_t blasId = 0;
            uint64_t vertexBufferDeviceAddress = 0;
            uint64_t indexBufferDeviceAddress = 0;
            std::vector<RayQueryMeshInstance> meshInstances;
            VkDeviceSize scratchSize = 0;
            VkDeviceSize scratchOffset = 0;
            uint64_t geometryHash = 0;
        };

        RayQueryScene g_lightingRayQueryScene;
        std::vector<QueuedMultiMeshBLASBuild> g_multiMeshBLASBuilds;
        std::unordered_map<uint64_t, uint64_t> g_multiMeshBLASGeometryHashes;
        VkDeviceSize g_multiMeshBLASScratchSize = 0;

        void AddPersistentBLASInstances(RayQueryScene& scene, const std::vector<RayQueryBLASInstance>& instances);
        void BeginMultiMeshBLASBuilds();
        void AddMultiMeshBLASes(RayQueryScene& scene, const std::vector<RayQueryMultiMeshBLAS>& blases);
        bool RecordMultiMeshBLASBuilds(VkCommandBuffer commandBuffer, uint64_t scratchBaseAddress);
        VkDeviceSize GetMultiMeshBLASScratchSize();
        VkDeviceSize AllocateMultiMeshBLASScratch(VkDeviceSize scratchSize);
        bool MeshFitsBuffers(const RayQueryMesh& mesh, VkDeviceSize vertexBufferSize, VkDeviceSize indexBufferSize);
        bool HasUsableBLAS(uint64_t blasId);
        uint64_t HashRayQueryMultiMeshBLAS(const RayQueryMultiMeshBLAS& blas, const std::vector<RayQueryMeshInstance>& meshInstances);
        void HashMix(uint64_t& hash, uint64_t value);
        size_t CountMultiMeshBLASMeshInstances(const std::vector<RayQueryMultiMeshBLAS>& blases);
        size_t CountTransientMeshInstances(const std::vector<TransientRayQueryBLASInstance>& instances);
    }

    void UpdateRayTracing(VkCommandBuffer commandBuffer) {
        ProfilerVulkanZoneFunction();

        VulkanFrameData& frameData = GetCurrentFrameData();

        VulkanDescriptorSetResource* rayTracingDescriptorSetResource = VulkanResourceManager::GetDescriptorSetResource("RayQueryDescriptorSet");
        VulkanDescriptorSet* rayTracingDescriptorSet = rayTracingDescriptorSetResource ? &rayTracingDescriptorSetResource->GetSet(GetCurrentFrameIndex()) : nullptr;
        VulkanMeshBuffer* assetMeshBuffer = VulkanResourceManager::GetMeshBuffer("AssetGeometry");
        VulkanBuffer* skinnedVertexBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.skinnedVertices);

        if (!rayTracingDescriptorSet) return;
        if (!assetMeshBuffer) return;

        const std::vector<RayQueryBLASInstance>& persistentInstances = Unloved::RenderDataManager::GetRayQueryBLASInstances();
        const std::vector<RayQueryMultiMeshBLAS>& multiMeshBLASes = Unloved::RenderDataManager::GetRayQueryMultiMeshBLASes();
        const std::vector<TransientRayQueryBLASInstance>& transientInstances = Unloved::RenderDataManager::GetTransientRayQueryBLASInstances();

        g_lightingRayQueryScene.Clear();
        g_lightingRayQueryScene.Reserve(
            persistentInstances.size() + multiMeshBLASes.size() + transientInstances.size(),
            persistentInstances.size() + CountMultiMeshBLASMeshInstances(multiMeshBLASes) + CountTransientMeshInstances(transientInstances)
        );

        TransientBLASBuilder::BeginFrame();
        BeginMultiMeshBLASBuilds();

        AddPersistentBLASInstances(g_lightingRayQueryScene, persistentInstances);
        AddMultiMeshBLASes(g_lightingRayQueryScene, multiMeshBLASes);

        if (skinnedVertexBuffer) {
            TransientBLASBuilder::AddTransientRayQueryBLASInstances(frameData, *assetMeshBuffer, *skinnedVertexBuffer, transientInstances, g_lightingRayQueryScene);
        }
        else {
            TransientBLASBuilder::ReleaseFrameSlots(frameData);
        }

        if (!g_lightingRayQueryScene.HasInstances()) return;
        if (!g_lightingRayQueryScene.Upload(commandBuffer, frameData)) return;

        uint32_t instanceCapacity = g_lightingRayQueryScene.GetInstanceCount();
        if (instanceCapacity > frameData.accelerationStructures.rayQueryTLASInstanceCapacity) {
            if (!g_lightingRayQueryScene.ResizeTLAS(frameData, instanceCapacity)) return;
        }

        VkDeviceSize scratchAlignment = AccelerationStructureScratchAlignment();
        VkDeviceSize blasScratchSize = AlignUp(TransientBLASBuilder::GetScratchSize(), scratchAlignment) + GetMultiMeshBLASScratchSize();
        VkDeviceSize requiredScratchSize = std::max(blasScratchSize, g_lightingRayQueryScene.GetTLASScratchSize(frameData)) + scratchAlignment;

        // One scratch buffer backs BLAS and TLAS builds
        if (!EnsureBufferSize(frameData.buffers.rayQueryScratch, requiredScratchSize)) return;

        VulkanBuffer* scratchBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.rayQueryScratch);
        if (!scratchBuffer) return;

        uint64_t scratchBaseAddress = AlignUp(scratchBuffer->GetDeviceAddress(), scratchAlignment);

        bool recordedBLASBuilds = TransientBLASBuilder::RecordBuilds(commandBuffer, frameData, scratchBaseAddress);
        uint64_t multiMeshScratchBaseAddress = scratchBaseAddress + AlignUp(TransientBLASBuilder::GetScratchSize(), scratchAlignment);
        bool recordedMultiMeshBLASBuilds = RecordMultiMeshBLASBuilds(commandBuffer, multiMeshScratchBaseAddress);
        if (recordedBLASBuilds || recordedMultiMeshBLASBuilds) {
            RecordAccelerationStructureBuildBarrier(commandBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR);
        }

        // TLAS sees the final instance list
        g_lightingRayQueryScene.RecordTLASBuild(commandBuffer, frameData, scratchBaseAddress);

        // Lighting reads this immediately after
        RecordAccelerationStructureBuildBarrier(commandBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR);

        g_lightingRayQueryScene.BindDescriptor(frameData, rayTracingDescriptorSet, 0);
    }

    namespace {
        void AddPersistentBLASInstances(RayQueryScene& scene, const std::vector<RayQueryBLASInstance>& instances) {
            // Persistent BLAS is already built by mesh buffer code
            for (const RayQueryBLASInstance& instance : instances) {
                if (instance.vulkanBlasId == 0 || instance.vertexBufferDeviceAddress == 0 || instance.indexBufferDeviceAddress == 0) continue;

                VulkanAccelerationStructure* blas = VulkanResourceManager::GetAccelerationStructure(instance.vulkanBlasId);
                if (!blas || blas->GetHandle() == VK_NULL_HANDLE || blas->GetDeviceAddress() == 0 || !blas->m_built) continue;

                scene.AddBLASInstance(blas->GetDeviceAddress(), TransformMatrixKHR(instance.modelMatrix), instance.vertexBufferDeviceAddress, instance.indexBufferDeviceAddress, instance.meshInstance);
            }
        }

        void BeginMultiMeshBLASBuilds() {
            g_multiMeshBLASBuilds.clear();
            g_multiMeshBLASScratchSize = 0;
        }

        void AddMultiMeshBLASes(RayQueryScene& scene, const std::vector<RayQueryMultiMeshBLAS>& blases) {
            for (const RayQueryMultiMeshBLAS& blas : blases) {
                if (blas.vulkanBlasId == 0 || blas.vertexBufferDeviceAddress == 0 || blas.indexBufferDeviceAddress == 0 || blas.meshInstances.empty()) continue;

                std::vector<RayQueryMeshInstance> meshInstances;
                meshInstances.reserve(blas.meshInstances.size());
                for (const RayQueryMeshInstance& meshInstance : blas.meshInstances) {
                    if (MeshFitsBuffers(meshInstance.mesh, blas.vertexBufferByteSize, blas.indexBufferByteSize)) {
                        meshInstances.push_back(meshInstance);
                    }
                }
                if (meshInstances.empty()) continue;

                uint64_t geometryHash = HashRayQueryMultiMeshBLAS(blas, meshInstances);
                auto hashIt = g_multiMeshBLASGeometryHashes.find(blas.vulkanBlasId);
                bool needsBuild = !HasUsableBLAS(blas.vulkanBlasId) || hashIt == g_multiMeshBLASGeometryHashes.end() || hashIt->second != geometryHash;

                if (needsBuild) {
                    VkAccelerationStructureBuildSizesInfoKHR sizeInfo = QueryBottomLevelBuildSize(
                        blas.vertexBufferDeviceAddress,
                        blas.indexBufferDeviceAddress,
                        meshInstances,
                        VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR
                    );

                    if (sizeInfo.accelerationStructureSize == 0 || !PrepareAccelerationStructure(blas.vulkanBlasId, VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR, sizeInfo)) {
                        continue;
                    }

                    QueuedMultiMeshBLASBuild& build = g_multiMeshBLASBuilds.emplace_back();
                    build.blasId = blas.vulkanBlasId;
                    build.vertexBufferDeviceAddress = blas.vertexBufferDeviceAddress;
                    build.indexBufferDeviceAddress = blas.indexBufferDeviceAddress;
                    build.meshInstances = meshInstances;
                    build.scratchSize = sizeInfo.buildScratchSize;
                    build.scratchOffset = AllocateMultiMeshBLASScratch(build.scratchSize);
                    build.geometryHash = geometryHash;
                }

                VulkanAccelerationStructure* accelerationStructure = VulkanResourceManager::GetAccelerationStructure(blas.vulkanBlasId);
                if (!accelerationStructure || accelerationStructure->GetHandle() == VK_NULL_HANDLE || accelerationStructure->GetDeviceAddress() == 0) continue;

                scene.AddBLASInstance(accelerationStructure->GetDeviceAddress(), TransformMatrixKHR(blas.modelMatrix), blas.vertexBufferDeviceAddress, blas.indexBufferDeviceAddress, meshInstances);
            }
        }

        bool RecordMultiMeshBLASBuilds(VkCommandBuffer commandBuffer, uint64_t scratchBaseAddress) {
            if (g_multiMeshBLASBuilds.empty()) return false;

            size_t geometryCount = 0;
            for (const QueuedMultiMeshBLASBuild& build : g_multiMeshBLASBuilds) {
                geometryCount += build.meshInstances.size();
            }

            std::vector<VkAccelerationStructureGeometryKHR> geometries;
            std::vector<VkAccelerationStructureBuildGeometryInfoKHR> buildInfos;
            std::vector<VkAccelerationStructureBuildRangeInfoKHR> rangeInfos;
            std::vector<const VkAccelerationStructureBuildRangeInfoKHR*> rangeInfoPtrs;
            std::vector<uint64_t> blasIds;
            std::vector<uint64_t> geometryHashes;

            geometries.reserve(geometryCount);
            buildInfos.reserve(g_multiMeshBLASBuilds.size());
            rangeInfos.reserve(geometryCount);
            rangeInfoPtrs.reserve(g_multiMeshBLASBuilds.size());
            blasIds.reserve(g_multiMeshBLASBuilds.size());
            geometryHashes.reserve(g_multiMeshBLASBuilds.size());

            for (const QueuedMultiMeshBLASBuild& build : g_multiMeshBLASBuilds) {
                VulkanAccelerationStructure* accelerationStructure = VulkanResourceManager::GetAccelerationStructure(build.blasId);
                if (!accelerationStructure || accelerationStructure->GetHandle() == VK_NULL_HANDLE || build.meshInstances.empty()) continue;

                size_t geometryOffset = geometries.size();
                size_t rangeInfoOffset = rangeInfos.size();

                for (const RayQueryMeshInstance& meshInstance : build.meshInstances) {
                    geometries.push_back(CreateTriangleGeometry(build.vertexBufferDeviceAddress, build.indexBufferDeviceAddress, meshInstance.mesh, GetRayQueryGeometryFlags(meshInstance.material)));

                    VkAccelerationStructureBuildRangeInfoKHR& rangeInfo = rangeInfos.emplace_back();
                    rangeInfo.primitiveCount = meshInstance.mesh.indexCount / 3;
                    rangeInfo.primitiveOffset = 0;
                    rangeInfo.firstVertex = 0;
                    rangeInfo.transformOffset = 0;
                }

                VkAccelerationStructureBuildGeometryInfoKHR buildInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
                buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
                buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
                buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
                buildInfo.dstAccelerationStructure = accelerationStructure->GetHandle();
                buildInfo.geometryCount = static_cast<uint32_t>(build.meshInstances.size());
                buildInfo.pGeometries = geometries.data() + geometryOffset;
                buildInfo.scratchData.deviceAddress = scratchBaseAddress + build.scratchOffset;
                buildInfos.push_back(buildInfo);

                rangeInfoPtrs.push_back(rangeInfos.data() + rangeInfoOffset);
                blasIds.push_back(build.blasId);
                geometryHashes.push_back(build.geometryHash);
            }

            if (buildInfos.empty()) return false;

            vkCmdBuildAccelerationStructuresKHR(commandBuffer, static_cast<uint32_t>(buildInfos.size()), buildInfos.data(), rangeInfoPtrs.data());

            for (size_t i = 0; i < blasIds.size(); i++) {
                VulkanAccelerationStructure* accelerationStructure = VulkanResourceManager::GetAccelerationStructure(blasIds[i]);
                if (accelerationStructure) {
                    accelerationStructure->m_built = true;
                }
                g_multiMeshBLASGeometryHashes[blasIds[i]] = geometryHashes[i];
            }

            return true;
        }

        VkDeviceSize GetMultiMeshBLASScratchSize() {
            return g_multiMeshBLASScratchSize;
        }

        VkDeviceSize AllocateMultiMeshBLASScratch(VkDeviceSize scratchSize) {
            VkDeviceSize scratchOffset = AlignUp(g_multiMeshBLASScratchSize, AccelerationStructureScratchAlignment());
            g_multiMeshBLASScratchSize = scratchOffset + scratchSize;
            return scratchOffset;
        }

        bool MeshFitsBuffers(const RayQueryMesh& mesh, VkDeviceSize vertexBufferSize, VkDeviceSize indexBufferSize) {
            if (mesh.vertexCount == 0 || mesh.indexCount < 3) return false;
            if (vertexBufferSize == 0 || indexBufferSize == 0) return false;

            uint64_t vertexEnd = static_cast<uint64_t>(mesh.baseVertex) + static_cast<uint64_t>(mesh.vertexCount);
            uint64_t indexEnd = static_cast<uint64_t>(mesh.baseIndex) + static_cast<uint64_t>(mesh.indexCount);
            uint64_t vertexCapacity = vertexBufferSize / sizeof(Vertex);
            uint64_t indexCapacity = indexBufferSize / sizeof(uint32_t);
            return vertexEnd <= vertexCapacity && indexEnd <= indexCapacity;
        }

        bool HasUsableBLAS(uint64_t blasId) {
            VulkanAccelerationStructure* blas = VulkanResourceManager::GetAccelerationStructure(blasId);
            return blas && blas->GetHandle() != VK_NULL_HANDLE && blas->GetDeviceAddress() != 0 && blas->m_built;
        }

        uint64_t HashRayQueryMultiMeshBLAS(const RayQueryMultiMeshBLAS& blas, const std::vector<RayQueryMeshInstance>& meshInstances) {
            uint64_t hash = 1469598103934665603ull;
            HashMix(hash, blas.vertexBufferDeviceAddress);
            HashMix(hash, blas.indexBufferDeviceAddress);
            HashMix(hash, blas.vertexBufferByteSize);
            HashMix(hash, blas.indexBufferByteSize);
            HashMix(hash, blas.sourceGeometryVersion);
            HashMix(hash, meshInstances.size());

            for (const RayQueryMeshInstance& meshInstance : meshInstances) {
                HashMix(hash, meshInstance.mesh.baseVertex);
                HashMix(hash, meshInstance.mesh.baseIndex);
                HashMix(hash, meshInstance.mesh.vertexCount);
                HashMix(hash, meshInstance.mesh.indexCount);
                HashMix(hash, GetRayQueryGeometryFlags(meshInstance.material));
            }

            return hash;
        }

        void HashMix(uint64_t& hash, uint64_t value) {
            hash ^= value;
            hash *= 1099511628211ull;
        }

        size_t CountMultiMeshBLASMeshInstances(const std::vector<RayQueryMultiMeshBLAS>& blases) {
            size_t count = 0;
            for (const RayQueryMultiMeshBLAS& blas : blases) {
                count += blas.meshInstances.size();
            }
            return count;
        }

        size_t CountTransientMeshInstances(const std::vector<TransientRayQueryBLASInstance>& instances) {
            size_t count = 0;
            for (const TransientRayQueryBLASInstance& instance : instances) {
                count += instance.meshInstances.size();
            }
            return count;
        }
    }
}
