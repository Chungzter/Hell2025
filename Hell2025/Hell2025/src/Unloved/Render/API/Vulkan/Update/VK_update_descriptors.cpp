#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"

#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/ResourceManagement/Types/Texture.h"
#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Render/API/Vulkan/Types/vk_allocated_image.h"
#include "Hell/Render/API/Vulkan/Types/vk_descriptor_set.h"
#include "Hell/Render/API/Vulkan/Types/vk_sampler.h"
#include "Hell/Render/API/Vulkan/Types/vk_texture.h"
#include "Unloved/Render/API/Vulkan/VK_descriptor_indices.h"

namespace VulkanRenderer {

    void UpdateBindlessRenderTargetDescriptors() {
        VulkanDescriptorSet* staticDescriptorSet = VulkanResourceManager::GetDescriptorSet("StaticDescriptorSet");
        if (!staticDescriptorSet) return;

        bool dirty = false;

        if (VulkanResourceManager::AllocatedImageExists("Present")) {
            AllocatedImage* presentImage = VulkanResourceManager::GetAllocatedImage("Present");
            if (presentImage) {
                staticDescriptorSet->WriteImage(DESC_IDX_TEXTURES, presentImage->GetImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VULKAN_TEXTURE_IDX_PRESENT);
                dirty = true;
            }
        }

        if (VulkanResourceManager::AllocatedImageExists("GBufferRE.Visibility")) {
            AllocatedImage* visibilityImage = VulkanResourceManager::GetAllocatedImage("GBufferRE.Visibility");
            if (visibilityImage) {
                staticDescriptorSet->WriteImage(DESC_IDX_UINT_TEXTURES, visibilityImage->GetImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VULKAN_UINT_TEXTURE_IDX_GBUFFER_VISIBILITY);
                dirty = true;
            }
        }

        if (VulkanResourceManager::AllocatedImageExists("GBufferRE.Depth")) {
            AllocatedImage* depthImage = VulkanResourceManager::GetAllocatedImage("GBufferRE.Depth");
            if (depthImage) {
                staticDescriptorSet->WriteImage(DESC_IDX_TEXTURES, depthImage->GetDepthOnlyImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VULKAN_TEXTURE_IDX_GBUFFER_DEPTH);
                dirty = true;
            }
        }

        if (VulkanResourceManager::AllocatedImageExists("BaseColorMetallic")) {
            AllocatedImage* baseColorImage = VulkanResourceManager::GetAllocatedImage("BaseColorMetallic");
            if (baseColorImage) {
                staticDescriptorSet->WriteImage(DESC_IDX_TEXTURES, baseColorImage->GetImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VULKAN_TEXTURE_IDX_BASE_COLOR_METALLIC);
                dirty = true;
            }
        }

        if (VulkanResourceManager::AllocatedImageExists("NormalXYRoughnessMisc")) {
            AllocatedImage* normalImage = VulkanResourceManager::GetAllocatedImage("NormalXYRoughnessMisc");
            if (normalImage) {
                staticDescriptorSet->WriteImage(DESC_IDX_TEXTURES, normalImage->GetImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VULKAN_TEXTURE_IDX_NORMAL_XY_ROUGHNESS_MISC);
                dirty = true;
            }
        }

        if (VulkanResourceManager::AllocatedImageExists("Lighting")) {
            AllocatedImage* lightingImage = VulkanResourceManager::GetAllocatedImage("Lighting");
            if (lightingImage) {
                staticDescriptorSet->WriteImage(DESC_IDX_STORAGE_IMAGES_RGBA16F, lightingImage->GetImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VULKAN_STORAGE_IMAGE_IDX_GBUFFER_LIGHTING);
                dirty = true;
            }
        }

        if (dirty) {
            staticDescriptorSet->Update();
        }
    }

    void UpdateBindlessTextureDescriptors() {
        VulkanDescriptorSet* staticDescriptorSet = VulkanResourceManager::GetDescriptorSet("StaticDescriptorSet");
        VulkanSampler* linearSampler = VulkanResourceManager::GetSampler("Linear");
        VulkanSampler* nearestSampler = VulkanResourceManager::GetSampler("Nearest");

        if (!staticDescriptorSet || !linearSampler || !nearestSampler) return;

        if (!g_staticSamplersUploaded) {
            staticDescriptorSet->WriteImage(DESC_IDX_SAMPLERS, VK_NULL_HANDLE, linearSampler->GetSampler(), VK_IMAGE_LAYOUT_UNDEFINED, VK_DESCRIPTOR_TYPE_SAMPLER, VULKAN_SAMPLER_IDX_LINEAR);
            staticDescriptorSet->WriteImage(DESC_IDX_SAMPLERS, VK_NULL_HANDLE, nearestSampler->GetSampler(), VK_IMAGE_LAYOUT_UNDEFINED, VK_DESCRIPTOR_TYPE_SAMPLER, VULKAN_SAMPLER_IDX_NEAREST);
            g_staticSamplersUploaded = true;
        }

        for (auto& [name, texture] : Hell::ResourceManager::GetTextures()) {
            if (texture.GetUploadState() != UploadState::UPLOADED) continue;
            if (texture.GetBindlessIndex() < 0) continue;
            if (texture.GetVulkanId() == 0) continue;

            VulkanTexture* vulkanTexture = VulkanResourceManager::GetTexturePtr(texture.GetVulkanId());
            if (!vulkanTexture || vulkanTexture->GetImageView() == VK_NULL_HANDLE) continue;
            if (vulkanTexture->GetSampler() == VK_NULL_HANDLE) continue;

            uint32_t textureIndex = static_cast<uint32_t>(texture.GetBindlessIndex());
            staticDescriptorSet->WriteImage(DESC_IDX_TEXTURES, vulkanTexture->GetImageView(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, textureIndex);
            staticDescriptorSet->WriteImage(DESC_IDX_TEXTURE_SAMPLERS, VK_NULL_HANDLE, vulkanTexture->GetSampler(), VK_IMAGE_LAYOUT_UNDEFINED, VK_DESCRIPTOR_TYPE_SAMPLER, textureIndex);
        }

        staticDescriptorSet->Update();
    }
}
