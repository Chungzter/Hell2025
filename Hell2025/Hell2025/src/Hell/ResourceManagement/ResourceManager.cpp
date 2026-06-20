#include "ResourceManager.h"

#include "Hell/Logging.h"
#include "Hell/MemoryTracker/MemoryTracker.h"

#include <algorithm>
#include <string>
#include <unordered_map>

namespace Hell::ResourceManager {

    namespace {
        std::unordered_map<std::string, GenericMesh> g_genericMeshes;
        std::unordered_map<std::string, MeshBuffer> g_meshBuffers;
    }

    void CleanUp() {
        for (auto& object : g_genericMeshes) { object.second.CleanUp(); } g_genericMeshes.clear();
        for (auto& object : g_meshBuffers)   { object.second.CleanUp(); } g_meshBuffers.clear();
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
}
