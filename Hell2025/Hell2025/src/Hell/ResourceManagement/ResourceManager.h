#pragma once

#include "Hell/ResourceManagement/Types/GenericMesh.h"
#include "Hell/ResourceManagement/Types/IESProfile.h"
#include "Hell/ResourceManagement/Types/MeshBuffer.h"
#include "Hell/ResourceManagement/Types/Texture.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>

namespace Hell::MemoryTracker {
    struct MemoryReport;
}

namespace Hell::ResourceManager {

    void CleanUp();
    void AppendMemoryReport(MemoryTracker::MemoryReport& report);

    GenericMesh& CreateGenericMesh(const std::string& name);
    GenericMesh& GetGenericMesh(const std::string& name);
    GenericMesh* GetGenericMeshPtr(const std::string& name);

    IESProfile& CreateIESProfile(const std::string& name);
    IESProfile& GetIESProfile(const std::string& name);
    IESProfile* GetIESProfilePtr(const std::string& name);

    MeshBuffer& CreateMeshBuffer(const std::string& name);
    MeshBuffer& GetMeshBuffer(const std::string& name);
    MeshBuffer* GetMeshBufferPtr(const std::string& name);

    Texture& CreateTexture(const std::string& name);
    std::unordered_map<std::string, Texture>& GetTextures();
    Texture* GetTextureByName(const std::string& name);
    Texture* GetTextureByBindlessIndex(int32_t bindlessIndex);
    int32_t GetTextureBindlessIndexByName(const std::string& name, bool ignoreWarning = true);
    void ReserveTextureStorage(size_t textureCount);
}
