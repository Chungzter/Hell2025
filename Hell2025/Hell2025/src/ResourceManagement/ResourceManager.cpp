#include "ResourceManager.h"

#include "Hell/Logging.h"

#include <unordered_map>
#include <string>

namespace ResourceManager {
    namespace {
        std::unordered_map<std::string, Mesh2D> g_mesh2Ds;
        std::unordered_map<std::string, MeshBuffer> g_meshBuffers;
    }

    void Init() {
        CreateMesh2D("UI");
        CreateMeshBuffer("Procedural");
    }

    Mesh2D& CreateMesh2D(const std::string& name) {
        auto it = g_mesh2Ds.find(name);

        if (it != g_mesh2Ds.end()) {
            Logging::Fatal() << "ResourceManager::CreateMesh2D(..) failed: '" << name << "' already exists\n";
            return it->second;
        }

        auto result = g_mesh2Ds.emplace(name, Mesh2D(name));
        return result.first->second;
    }

    Mesh2D& GetMesh2D(const std::string& name) {
        auto it = g_mesh2Ds.find(name);

        if (it == g_mesh2Ds.end()) {
            Logging::Error() << "ResourceManager::GetMesh2D(..) failed: '" << name << "' does not exist\n";

            static Mesh2D invalid;
            return invalid;
        }

        return it->second;
    }

    Mesh2D* GetMesh2DPtr(const std::string& name) {
        auto it = g_mesh2Ds.find(name);

        if (it == g_mesh2Ds.end()) {
            Logging::Error() << "ResourceManager::GetMesh2DPtr(..) failed: '" << name << "' does not exist\n";
            return nullptr;
        }

        return &it->second;
    }

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