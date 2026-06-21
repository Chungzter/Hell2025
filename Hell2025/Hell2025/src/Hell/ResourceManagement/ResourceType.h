#pragma once

#include <cstdint>

namespace Hell::ResourceManagement {

enum class ResourceType : uint16_t {
    OPENGL_GENERIC_MESH,
    OPENGL_MESH_BUFFER,
    OPENGL_TEXTURE,
    VULKAN_GENERIC_MESH,
    VULKAN_MESH_BUFFER,
    VULKAN_TEXTURE,
    UNDEFINED
};

}
