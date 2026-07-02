#pragma once

#include "Unloved/Render/API/Vulkan/VK_render_state.h"

#include <string>

namespace VulkanRenderer {
    void CreateRenderStates();
    VulkanRenderState& CreateRenderState(const std::string& name);
    VulkanRenderState* GetRenderState(const std::string& name);
}
