#pragma once

#include "Hell/Render/VertexAttributes.h"

#include "vk_buffer.h"
#include <vector>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

struct VulkanDetachedMesh {
private:
    VulkanBuffer vertexBuffer;
    VulkanBuffer indexBuffer;
public:
    void UpdateBuffers(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);
    void Cleanup(VkDevice device);
};
