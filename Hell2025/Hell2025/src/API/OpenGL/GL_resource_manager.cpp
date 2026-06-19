#include "GL_resource_manager.h"

#include "Hell/Containers/SlotMap.h"
#include "Hell/Enums.h"
#include "Hell/UniqueId.h"

namespace OpenGLResourceManager {

    namespace {
        Hell::SlotMap<OpenGLGenericMesh> g_genericMeshes;
        Hell::SlotMap<OpenGLMeshBuffer> g_meshBuffers;
    }

    void CleanUp() {
        for (OpenGLGenericMesh& genericMesh : g_genericMeshes) {
            genericMesh.CleanUp();
        }

        for (OpenGLMeshBuffer& meshBuffer : g_meshBuffers) {
            meshBuffer.Reset();
        }

        g_genericMeshes.clear();
        g_meshBuffers.clear();
    }

    // OpenGL Generic Mesh

    uint64_t CreateGenericMesh() {
        uint64_t id = UniqueID::GetNextObjectId(ObjectType::GL_GENERIC_MESH);
        g_genericMeshes.emplace_with_id(id);
        return id;
    }

    OpenGLGenericMesh& GetGenericMesh(uint64_t id) {
        OpenGLGenericMesh* mesh3D = GetGenericMeshPtr(id);
        if (mesh3D) {
            return *mesh3D;
        }
        else {
            static OpenGLGenericMesh invalid;
            return invalid;
        }
    }

    OpenGLGenericMesh* GetGenericMeshPtr(uint64_t id) {
        return g_genericMeshes.get(id);
    }

    void RemoveGenericMesh(uint64_t id) {
        if (g_genericMeshes.contains(id)) {
            g_genericMeshes.get(id)->CleanUp();
            g_genericMeshes.erase(id);
        }
    }

    // OpenGL Mesh Buffer

    uint64_t CreateMeshBuffer() {
        uint64_t id = UniqueID::GetNextObjectId(ObjectType::GL_MESH_BUFFER);
        g_meshBuffers.emplace_with_id(id);
        return id;
    }

    OpenGLMeshBuffer& GetMeshBuffer(uint64_t id) {
        OpenGLMeshBuffer* meshBuffer = GetMeshBufferPtr(id);
        if (meshBuffer) {
            return *meshBuffer;
        }
        else {
            static OpenGLMeshBuffer invalid;
            return invalid;
        }
    }

    OpenGLMeshBuffer* GetMeshBufferPtr(uint64_t id) {
        return g_meshBuffers.get(id);
    }

    void RemoveMeshBuffer(uint64_t id) {
        if (g_meshBuffers.contains(id)) {
            g_meshBuffers.get(id)->Reset();
            g_meshBuffers.erase(id);
        }
    }
}
