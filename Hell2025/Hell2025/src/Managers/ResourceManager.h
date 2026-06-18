#pragma once

#include "Types/Renderer/MeshBuffer.h"

#include <cstdint>
#include <string>

namespace ResourceManager {

    void Init();

    MeshBuffer& CreateMeshBuffer(const std::string& name);
    MeshBuffer& GetMeshBuffer(const std::string& name);
    MeshBuffer* GetMeshBufferPtr(const std::string& name);
}

