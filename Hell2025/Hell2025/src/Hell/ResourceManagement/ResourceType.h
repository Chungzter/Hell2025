#pragma once

#include <cstdint>

namespace Hell::ResourceManagement {

enum class ResourceType : uint16_t {
    GL_GENERIC_MESH,
    GL_MESH_BUFFER,
    VK_GENERIC_MESH,
    VK_MESH_BUFFER,
    TEXTURE,
    UNDEFINED
};

}
