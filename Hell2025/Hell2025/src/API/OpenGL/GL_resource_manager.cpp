#include "GL_resource_manager.h"

#include "Hell/Containers/SlotMap.h"
#include "Hell/ResourceManagement/ResourceID.h"

namespace OpenGLResourceManager {

    namespace {
        Hell::SlotMap<OpenGLGenericMesh> g_genericMeshes;
        Hell::SlotMap<OpenGLMeshBuffer> g_meshBuffers;
        Hell::SlotMap<OpenGLTexture> g_textures;
    }

    void CleanUp() {
        for (OpenGLGenericMesh& genericMesh : g_genericMeshes) {
            genericMesh.CleanUp();
        }

        for (OpenGLMeshBuffer& meshBuffer : g_meshBuffers) {
            meshBuffer.Reset();
        }

        for (OpenGLTexture& texture : g_textures) {
            texture.MakeBindlessTextureNonResident();
            texture.Reset();
        }

        g_genericMeshes.clear();
        g_meshBuffers.clear();
        g_textures.clear();
    }

    // OpenGL Generic Mesh

    uint64_t CreateGenericMesh() {
        uint64_t id = Hell::ResourceManagement::GetNextID(Hell::ResourceManagement::ResourceType::OPENGL_GENERIC_MESH);
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
        uint64_t id = Hell::ResourceManagement::GetNextID(Hell::ResourceManagement::ResourceType::OPENGL_MESH_BUFFER);
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

    // OpenGL Texture

    uint64_t CreateTexture() {
        uint64_t id = Hell::ResourceManagement::GetNextID(Hell::ResourceManagement::ResourceType::OPENGL_TEXTURE);
        g_textures.emplace_with_id(id);
        return id;
    }

    OpenGLTexture& GetTexture(uint64_t id) {
        OpenGLTexture* texture = GetTexturePtr(id);
        if (texture) {
            return *texture;
        }
        else {
            static OpenGLTexture invalid;
            return invalid;
        }
    }

    OpenGLTexture* GetTexturePtr(uint64_t id) {
        return g_textures.get(id);
    }

    void RemoveTexture(uint64_t id) {
        if (g_textures.contains(id)) {
            OpenGLTexture* texture = g_textures.get(id);
            texture->MakeBindlessTextureNonResident();
            texture->Reset();
            g_textures.erase(id);
        }
    }
}
