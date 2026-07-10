#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Render/API/Vulkan/Types/vk_acceleration_structure.h"
#include "Hell/Render/API/Vulkan/Types/vk_buffer.h"
#include "Hell/Render/API/Vulkan/Types/vk_descriptor_set.h"
#include "Hell/Render/API/Vulkan/Types/vk_mesh_buffer.h"

#include "Unloved/Render/API/Vulkan/RayTracing/VK_acceleration_structure_utils.h"
#include "Unloved/Render/API/Vulkan/RayTracing/VK_raytracing_scene.h"
#include "Unloved/Render/API/Vulkan/RayTracing/VK_transient_blas_builder.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"

#include <algorithm>
#include <cstddef>

namespace VulkanRenderer {
    namespace {
        RayTracingScene g_lightingRayTracingScene;

        void AddPersistentInstances(RayTracingScene& scene, const std::vector<StaticRayTracingInstance>& instances);
        size_t CountSkinnedGeometryRanges(const std::vector<SkinnedRayTracingGroup>& groups);
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

        const std::vector<StaticRayTracingInstance>& persistentInstances = Unloved::RenderDataManager::GetStaticRayTracingInstances();
        const std::vector<SkinnedRayTracingGroup>& skinnedGroups = Unloved::RenderDataManager::GetSkinnedRayTracingGroups();

        g_lightingRayTracingScene.Clear();
        g_lightingRayTracingScene.Reserve(persistentInstances.size() + skinnedGroups.size(), persistentInstances.size() + CountSkinnedGeometryRanges(skinnedGroups));

        TransientBLASBuilder::BeginFrame();

        AddPersistentInstances(g_lightingRayTracingScene, persistentInstances);

        if (skinnedVertexBuffer) {
            TransientBLASBuilder::AddSkinnedRayTracingGroups(frameData, *assetMeshBuffer, *skinnedVertexBuffer, skinnedGroups, g_lightingRayTracingScene);
        }
        else {
            TransientBLASBuilder::ReleaseFrameSlots(frameData);
        }

        if (!g_lightingRayTracingScene.HasInstances()) return;
        if (!g_lightingRayTracingScene.Upload(commandBuffer, frameData)) return;

        uint32_t instanceCapacity = g_lightingRayTracingScene.GetInstanceCount();
        if (instanceCapacity > frameData.accelerationStructures.rayQueryTLASInstanceCapacity) {
            if (!g_lightingRayTracingScene.ResizeTLAS(frameData, instanceCapacity)) return;
        }

        VkDeviceSize scratchAlignment = AccelerationStructureScratchAlignment();
        VkDeviceSize requiredScratchSize = std::max(TransientBLASBuilder::GetScratchSize(), g_lightingRayTracingScene.GetTLASScratchSize(frameData)) + scratchAlignment;

        // One scratch buffer backs transient BLAS and TLAS builds
        if (!EnsureBufferSize(frameData.buffers.rayQueryScratch, requiredScratchSize)) return;

        VulkanBuffer* scratchBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.rayQueryScratch);
        if (!scratchBuffer) return;

        uint64_t scratchBaseAddress = AlignUp(scratchBuffer->GetDeviceAddress(), scratchAlignment);

        bool recordedBLASBuilds = TransientBLASBuilder::RecordBuilds(commandBuffer, frameData, scratchBaseAddress);
        if (recordedBLASBuilds) {
            RecordAccelerationStructureBuildBarrier(commandBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR);
        }

        // TLAS sees the final instance list
        g_lightingRayTracingScene.RecordTLASBuild(commandBuffer, frameData, scratchBaseAddress);

        // Lighting reads this immediately after
        RecordAccelerationStructureBuildBarrier(commandBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR);

        g_lightingRayTracingScene.BindDescriptor(frameData, rayTracingDescriptorSet, 0);
    }

    namespace {
        void AddPersistentInstances(RayTracingScene& scene, const std::vector<StaticRayTracingInstance>& instances) {
            // Static BLAS is already built by mesh buffer code
            for (const StaticRayTracingInstance& instance : instances) {
                if (instance.vulkanBlasId == 0 || instance.vertexBufferDeviceAddress == 0 || instance.indexBufferDeviceAddress == 0) continue;

                VulkanAccelerationStructure* blas = VulkanResourceManager::GetAccelerationStructure(instance.vulkanBlasId);
                if (!blas || blas->GetHandle() == VK_NULL_HANDLE || blas->GetDeviceAddress() == 0 || !blas->m_built) continue;

                scene.AddInstance(blas->GetDeviceAddress(), TransformMatrixKHR(instance.modelMatrix), instance.vertexBufferDeviceAddress, instance.indexBufferDeviceAddress, instance.range);
            }
        }

        size_t CountSkinnedGeometryRanges(const std::vector<SkinnedRayTracingGroup>& groups) {
            size_t count = 0;
            for (const SkinnedRayTracingGroup& group : groups) {
                count += group.ranges.size();
            }
            return count;
        }
    }
}
