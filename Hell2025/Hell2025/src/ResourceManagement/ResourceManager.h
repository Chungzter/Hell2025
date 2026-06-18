#pragma once

#include "ResourceManagement/Types/Mesh2D.h"
#include "ResourceManagement/Types/MeshBuffer.h"

#include <cstdint>
#include <string>

namespace ResourceManager {
    void Init();

    Mesh2D& CreateMesh2D(const std::string& name);
    Mesh2D& GetMesh2D(const std::string& name);
    Mesh2D* GetMesh2DPtr(const std::string& name);

    MeshBuffer& CreateMeshBuffer(const std::string& name);
    MeshBuffer& GetMeshBuffer(const std::string& name);
    MeshBuffer* GetMeshBufferPtr(const std::string& name);
}

