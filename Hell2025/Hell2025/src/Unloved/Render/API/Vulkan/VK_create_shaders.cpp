#include "VK_renderer.h"

#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"

namespace VulkanRenderer {

    void CreateShaders() {
        VulkanResourceManager::CreateShader("FullscreenTriangle", { "VK_fullscreen_triangle.vert", "VK_solid_color.frag" });
        VulkanResourceManager::CreateShader("Present", { "VK_fullscreen_triangle.vert", "VK_present.frag" });
        VulkanResourceManager::CreateShader("Visibility", { "VK_visibility.vert", "VK_visibility.frag" });
        VulkanResourceManager::CreateShader("VisibilityAlphaDiscard", { "VK_visibility.vert", "VK_visibility_alpha_discard.frag" });
        VulkanResourceManager::CreateShader("VisibilitySkinned", { "VK_visibility_skinned.vert", "VK_visibility.frag" });
        VulkanResourceManager::CreateShader("VisibilitySkinnedAlphaDiscard", { "VK_visibility_skinned.vert", "VK_visibility_alpha_discard.frag" });
        VulkanResourceManager::CreateShader("VisibilityDebug", { "VK_fullscreen_triangle.vert", "VK_visibility_debug.frag" });
        VulkanResourceManager::CreateShader("MaterialResolve", { "VK_fullscreen_triangle.vert", "VK_material_resolve.frag" });
        VulkanResourceManager::CreateShader("ComputeSkinning", { "VK_compute_skinning.comp" });
        VulkanResourceManager::CreateShader("ComputeRedTest", { "VK_compute_red_test.comp" });
        VulkanResourceManager::CreateShader("UI", { "VK_ui.vert", "VK_ui.frag" });
    }
}
