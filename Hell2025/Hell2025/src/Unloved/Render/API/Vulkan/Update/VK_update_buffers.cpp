#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"

#include "Hell/Debug/DebugDraw.h"
#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/UI/UIBackEnd.h"

#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Render/RendererConstants.h"
#include "Unloved/Systems/DirtyTracker/DirtyTracker.h"

#include <algorithm>
#include <array>
#include <vector>

namespace VulkanRenderer {

    template <typename T>
    void UpdateVectorBuffer(uint64_t bufferId, const std::vector<T>& data) {
        VulkanBuffer* buffer = VulkanResourceManager::GetBuffer(bufferId);
        VkDeviceSize size = sizeof(T) * data.size();
        EnsureBufferSize(buffer, size);
        UpdateBuffer(buffer, data.data(), size);
    }

    template <typename T>
    void UpdateGenericMesh(uint64_t meshId, const std::vector<T>& vertices) {
        VulkanGenericMesh* mesh = VulkanResourceManager::GetGenericMesh(meshId);
        if (!mesh) return;

        mesh->UpdateVertexData(vertices.empty() ? nullptr : vertices.data(), vertices.size(), T::GetLayout());
    }

    void UpdateBuffers() {
        const VulkanFrameData& frameData = GetCurrentFrameData();

        // DDGI

        const std::vector<GPUAABB>& dirtyDoorAABBs = Unloved::DirtyTracker::GetDirtyDoorAABBs();
        UpdateVectorBuffer(frameData.ddgi.dirtyDoorAABBs, dirtyDoorAABBs);

        // Instance data

        const std::vector<RenderItem>& instanceData = Unloved::RenderDataManager::GetInstanceData();
        VulkanBuffer* instanceDataBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.instanceData);
        VkDeviceSize instanceBufferSize = sizeof(RenderItem) * instanceData.size();
        EnsureBufferSize(instanceDataBuffer, instanceBufferSize);
        UpdateBuffer(instanceDataBuffer, instanceData.data(), instanceBufferSize);

        // Lights

        const std::vector<GPULight>& lights = Unloved::RenderDataManager::GetGPULights();
        VulkanBuffer* lightsBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.lights);
        VkDeviceSize lightsBufferSize = sizeof(GPULight) * lights.size();
        EnsureBufferSize(lightsBuffer, lightsBufferSize);
        UpdateBuffer(lightsBuffer, lights.data(), lightsBufferSize);

        // Materials

        const std::vector<Material>& materials = Hell::ResourceManager::GetMaterials();
        VulkanBuffer* materialsBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.materials);
        VkDeviceSize materialsBufferSize = sizeof(Material) * materials.size();
        EnsureBufferSize(materialsBuffer, materialsBufferSize);
        UpdateBuffer(materialsBuffer, materials.data(), materialsBufferSize);

        // Sprite sheet instances

        const std::vector<SpriteSheetRenderItem>& spriteSheetInstanceData = Unloved::RenderDataManager::GetSpriteSheetInstanceData();
        VulkanBuffer* spriteSheetInstanceDataBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.spriteSheetInstanceData);
        VkDeviceSize spriteSheetInstanceDataBufferSize = sizeof(SpriteSheetRenderItem) * spriteSheetInstanceData.size();
        EnsureBufferSize(spriteSheetInstanceDataBuffer, spriteSheetInstanceDataBufferSize);
        UpdateBuffer(spriteSheetInstanceDataBuffer, spriteSheetInstanceData.data(), spriteSheetInstanceDataBufferSize);

        // Renderer data

        const RendererData& rendererData = Unloved::RenderDataManager::GetRendererData();
        VulkanBuffer* rendererDataBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.rendererData);
        VkDeviceSize rendererDataBufferSize = sizeof(RendererData);
        EnsureBufferSize(rendererDataBuffer, rendererDataBufferSize);
        UpdateBuffer(rendererDataBuffer, &rendererData, rendererDataBufferSize);

        // Skinning

        const std::vector<SkinningDispatchGroup>& skinningDispatchGroups= Unloved::RenderDataManager::GetSkinningDispatchGroups();
        const std::vector<SkinningJob>& skinningJobs = Unloved::RenderDataManager::GetSkinningJobs();
        const std::vector<glm::mat4>& skinningTransforms = Unloved::RenderDataManager::GetSkinningTransforms();
        const std::vector<glm::mat4>& previousSkinningTransforms = Unloved::RenderDataManager::GetPreviousSkinningTransforms();

        UpdateVectorBuffer(frameData.buffers.skinningDispatchGroups, skinningDispatchGroups);
        UpdateVectorBuffer(frameData.buffers.skinningJobs, skinningJobs);
        UpdateVectorBuffer(frameData.buffers.skinningTransforms, skinningTransforms);
        UpdateVectorBuffer(frameData.buffers.previousSkinningTransforms, previousSkinningTransforms);

        // Viewport data

        const std::vector<ViewportData>& viewportData = Unloved::RenderDataManager::GetViewportData();
        VulkanBuffer* viewportDataBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.viewportData);
        VkDeviceSize viewportDataBufferSize = sizeof(ViewportData) * viewportData.size();
        EnsureBufferSize(viewportDataBuffer, viewportDataBufferSize);
        UpdateBuffer(viewportDataBuffer, viewportData.data(), viewportDataBufferSize);

        // Debug draw

        UpdateGenericMesh(frameData.genericMeshes.debugLines2D, Hell::DebugDraw::GetLines2D());
        UpdateGenericMesh(frameData.genericMeshes.debugLines3D, Hell::DebugDraw::GetLines3D());
        UpdateGenericMesh(frameData.genericMeshes.debugPoints2D, Hell::DebugDraw::GetPoints2D());
        UpdateGenericMesh(frameData.genericMeshes.debugPoints3D, Hell::DebugDraw::GetPoints3D());

    }

    void UpdateBuffersUI() {
        const VulkanFrameData& frameData = GetCurrentFrameData();

        // UI mesh
        if (VulkanGenericMesh* uiMesh = VulkanResourceManager::GetGenericMesh(frameData.genericMeshes.ui)) {
            const std::vector<Vertex2D>& vertices = UIBackEnd::GetVertices();
            const std::vector<uint32_t>& indices = UIBackEnd::GetIndices();
            uiMesh->UpdateVertexData(vertices.empty() ? nullptr : vertices.data(), vertices.size(), Vertex2D::GetLayout());
            uiMesh->UpdateIndexData(indices);
        }

        // UI render items
        const std::vector<RenderItemUI>& renderItems = UIBackEnd::GetRenderItems();
        VulkanBuffer* renderItemsBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.uiRenderItems);
        VkDeviceSize renderItemsBufferSize = sizeof(RenderItemUI) * renderItems.size();
        EnsureBufferSize(renderItemsBuffer, renderItemsBufferSize);
        UpdateBuffer(renderItemsBuffer, renderItems.data(), renderItemsBufferSize);
    }
}
