#pragma once

#include "Hell/ResourceManagement/Types/GenericMesh.h"
#include "Hell/ResourceManagement/Types/MeshBuffer.h"

#include <cstdint>
#include <string>

namespace Hell::MemoryTracker {
    struct MemoryReport;
}

namespace Hell::ResourceManager {

    void CleanUp();
    void AppendMemoryReport(MemoryTracker::MemoryReport& report);

    GenericMesh& CreateGenericMesh(const std::string& name);
    GenericMesh& GetGenericMesh(const std::string& name);
    GenericMesh* GetGenericMeshPtr(const std::string& name);

    MeshBuffer& CreateMeshBuffer(const std::string& name);
    MeshBuffer& GetMeshBuffer(const std::string& name);
    MeshBuffer* GetMeshBufferPtr(const std::string& name);
}
