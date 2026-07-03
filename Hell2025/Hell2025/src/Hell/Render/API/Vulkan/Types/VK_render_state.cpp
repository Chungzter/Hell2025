#include "VK_render_state.h"

VulkanRenderTargetInfo& VulkanRenderState::AddColorTarget(const std::string& imageName) {
    if (colorTargetCount >= MAX_RENDER_TARGET_COUNT) {
        return colorTargets[MAX_RENDER_TARGET_COUNT - 1];
    }

    VulkanRenderTargetInfo& target = colorTargets[colorTargetCount++];
    target = VulkanRenderTargetInfo();
    target.imageName = imageName;
    return target;
}

VulkanRenderTargetInfo& VulkanRenderState::SetDepthTarget(const std::string& imageName) {
    hasDepthTarget = true;
    depthTarget = VulkanRenderTargetInfo();
    depthTarget.imageName = imageName;
    return depthTarget;
}

void VulkanRenderState::CleanUp() {
    // Intentionally blank
}
