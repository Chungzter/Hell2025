#include "ResourceManager.h"

#include "Hell/Logging.h"
#include "Hell/MemoryTracker/MemoryTracker.h"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

namespace Hell::ResourceManager {

    namespace {
        std::unordered_map<std::string, GenericMesh> g_genericMeshes;
        std::unordered_map<std::string, IESProfile> g_iesProfiles;
        std::unordered_map<std::string, MeshBuffer> g_meshBuffers;
        std::unordered_map<std::string, Texture> g_textures;

        std::vector<std::string> g_textureNamesByBindlessIndex;
    }

    void CleanUp() {
        for (auto& object : g_genericMeshes) { object.second.CleanUp(); } g_genericMeshes.clear();
        g_iesProfiles.clear();
        for (auto& object : g_meshBuffers)   { object.second.CleanUp(); } g_meshBuffers.clear();
        for (auto& object : g_textures)      { object.second.CleanUp(); } g_textures.clear();

        g_textureNamesByBindlessIndex.clear();
    }

    void AppendMemoryReport(MemoryTracker::MemoryReport& report) {
        if (!g_genericMeshes.empty()) {
            MemoryTracker::MemoryReportCategory& category = report.categories.emplace_back();
            category.name = "Generic Meshes";
            category.entries.reserve(g_genericMeshes.size());

            for (const auto& [name, genericMesh] : g_genericMeshes) {
                MemoryTracker::MemoryReportEntry& entry = category.entries.emplace_back();
                entry.name = name;
                entry.cpuBytes = genericMesh.GetCPUAllocatedByteCount();
                entry.gpuBytes = genericMesh.GetGPUAllocatedByteCount();
            }

            std::sort(category.entries.begin(), category.entries.end(), [](const auto& a, const auto& b) {
                return a.name < b.name;
            });
        }

        if (!g_meshBuffers.empty()) {
            MemoryTracker::MemoryReportCategory& category = report.categories.emplace_back();
            category.name = "Mesh Buffers";
            category.entries.reserve(g_meshBuffers.size());

            for (const auto& [name, meshBuffer] : g_meshBuffers) {
                MemoryTracker::MemoryReportEntry& entry = category.entries.emplace_back();
                entry.name = name;
                entry.cpuBytes = meshBuffer.GetCPUAllocatedByteCount();
                entry.gpuBytes = meshBuffer.GetGPUAllocatedByteCount();
            }

            std::sort(category.entries.begin(), category.entries.end(), [](const auto& a, const auto& b) {
                return a.name < b.name;
            });
        }

        if (!g_iesProfiles.empty()) {
            MemoryTracker::MemoryReportCategory& category = report.categories.emplace_back();
            category.name = "IES Profiles";
            category.entries.reserve(g_iesProfiles.size());

            for (const auto& [name, iesProfile] : g_iesProfiles) {
                MemoryTracker::MemoryReportEntry& entry = category.entries.emplace_back();
                entry.name = name;
                entry.cpuBytes = iesProfile.GetCPUAllocatedByteCount();
            }

            std::sort(category.entries.begin(), category.entries.end(), [](const auto& a, const auto& b) {
                return a.name < b.name;
            });
        }

        if (!g_textures.empty()) {
            MemoryTracker::MemoryReportCategory& category = report.categories.emplace_back();
            category.name = "Textures";
            category.entries.reserve(g_textures.size());

            for (const auto& [name, texture] : g_textures) {
                MemoryTracker::MemoryReportEntry& entry = category.entries.emplace_back();
                entry.name = name;
                entry.cpuBytes = texture.GetCPUAllocatedByteCount();
                entry.gpuBytes = texture.GetGPUAllocatedByteCount();
            }

            std::sort(category.entries.begin(), category.entries.end(), [](const auto& a, const auto& b) {
                return a.name < b.name;
            });
        }
    }

    // Generic Mesh

    GenericMesh& CreateGenericMesh(const std::string& name) {
        auto it = g_genericMeshes.find(name);

        if (it != g_genericMeshes.end()) {
            Logging::Fatal() << "ResourceManager::CreateGenericMesh(..) failed: '" << name << "' already exists\n";
            return it->second;
        }

        auto result = g_genericMeshes.emplace(name, GenericMesh(name));
        return result.first->second;
    }

    GenericMesh& GetGenericMesh(const std::string& name) {
        auto it = g_genericMeshes.find(name);

        if (it == g_genericMeshes.end()) {
            Logging::Error() << "ResourceManager::GetGenericMesh(..) failed: '" << name << "' does not exist\n";

            static GenericMesh invalid;
            return invalid;
        }

        return it->second;
    }

    GenericMesh* GetGenericMeshPtr(const std::string& name) {
        auto it = g_genericMeshes.find(name);

        if (it == g_genericMeshes.end()) {
            Logging::Error() << "ResourceManager::GetGenericMeshPtr(..) failed: '" << name << "' does not exist\n";
            return nullptr;
        }

        return &it->second;
    }

    // IES Profile

    IESProfile& CreateIESProfile(const std::string& name) {
        auto it = g_iesProfiles.find(name);

        if (it != g_iesProfiles.end()) {
            Logging::Fatal() << "ResourceManager::CreateIESProfile(..) failed: '" << name << "' already exists\n";
            return it->second;
        }

        auto result = g_iesProfiles.emplace(name, IESProfile(name));
        return result.first->second;
    }

    IESProfile& GetIESProfile(const std::string& name) {
        auto it = g_iesProfiles.find(name);

        if (it == g_iesProfiles.end()) {
            Logging::Error() << "ResourceManager::GetIESProfile(..) failed: '" << name << "' does not exist\n";

            static IESProfile invalid;
            return invalid;
        }

        return it->second;
    }

    IESProfile* GetIESProfilePtr(const std::string& name) {
        auto it = g_iesProfiles.find(name);

        if (it == g_iesProfiles.end()) {
            Logging::Error() << "ResourceManager::GetIESProfilePtr(..) failed: '" << name << "' does not exist\n";
            return nullptr;
        }

        return &it->second;
    }

    // Mesh Buffer

    MeshBuffer& CreateMeshBuffer(const std::string& name) {
        auto it = g_meshBuffers.find(name);

        if (it != g_meshBuffers.end()) {
            Logging::Fatal() << "ResourceManager::CreateMeshBuffer(..) failed: '" << name << "' already exists\n";
            return it->second;
        }

        auto result = g_meshBuffers.emplace(name, MeshBuffer(name));
        return result.first->second;
    }

    MeshBuffer& GetMeshBuffer(const std::string& name) {
        auto it = g_meshBuffers.find(name);

        if (it == g_meshBuffers.end()) {
            Logging::Error() << "ResourceManager::GetMeshBuffer(..) failed: '" << name << "' does not exist\n";

            static MeshBuffer invalid;
            return invalid;
        }

        return it->second;
    }

    MeshBuffer* GetMeshBufferPtr(const std::string& name) {
        auto it = g_meshBuffers.find(name);

        if (it == g_meshBuffers.end()) {
            Logging::Error() << "ResourceManager::GetMeshBufferPtr(..) failed: '" << name << "' does not exist\n";
            return nullptr;
        }

        return &it->second;
    }

    // Texture

    Texture& CreateTexture(const std::string& name) {
        auto it = g_textures.find(name);

        if (it != g_textures.end()) {
            Logging::Fatal() << "ResourceManager::CreateTexture(..) failed: '" << name << "' already exists\n";
            return it->second;
        }

        auto result = g_textures.try_emplace(name);
        Texture& texture = result.first->second;
        const int32_t bindlessIndex = static_cast<int32_t>(g_textureNamesByBindlessIndex.size());

        texture.SetBindlessIndex(bindlessIndex);

        g_textureNamesByBindlessIndex.push_back(name);

        return texture;
    }

    std::unordered_map<std::string, Texture>& GetTextures() {
        return g_textures;
    }

    Texture* GetTextureByName(const std::string& name) {
        auto it = g_textures.find(name);

        if (it == g_textures.end()) {
            Logging::Error() << "ResourceManager::GetTextureByName() failed because texture '" << name << "' does not exist\n";
            return nullptr;
        }

        return &it->second;
    }

    Texture* GetTextureByBindlessIndex(int32_t bindlessIndex) {
        if (bindlessIndex < 0 || static_cast<size_t>(bindlessIndex) >= g_textureNamesByBindlessIndex.size()) {
            Logging::Error() << "ResourceManager::GetTextureByBindlessIndex() failed because bindless index '" << bindlessIndex << "' does not exist\n";
            return nullptr;
        }

        return GetTextureByName(g_textureNamesByBindlessIndex[bindlessIndex]);
    }

    int32_t GetTextureBindlessIndexByName(const std::string& name, bool ignoreWarning) {
        auto it = g_textures.find(name);
        if (it != g_textures.end()) {
            return it->second.GetBindlessIndex();
        }

        if (!ignoreWarning) {
            Logging::Fatal() << "ResourceManager::GetTextureBindlessIndexByName() failed because texture '" << name << "' does not exist\n";
        }

        return -1;
    }

    void ReserveTextureStorage(size_t textureCount) {
        g_textures.reserve(textureCount);
        g_textureNamesByBindlessIndex.reserve(textureCount);
    }
}
