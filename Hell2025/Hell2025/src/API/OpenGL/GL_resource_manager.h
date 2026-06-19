#pragma once
#include "API/OpenGL/Types/GL_generic_mesh.h"
#include "API/OpenGL/Types/GL_mesh_buffer.h"

namespace OpenGLResourceManager {
    void CleanUp();

    uint64_t CreateGenericMesh();
    OpenGLGenericMesh& GetGenericMesh(uint64_t id);
    OpenGLGenericMesh* GetGenericMeshPtr(uint64_t id);
    void RemoveGenericMesh(uint64_t id);

    uint64_t CreateMeshBuffer();
    OpenGLMeshBuffer& GetMeshBuffer(uint64_t id);
    OpenGLMeshBuffer* GetMeshBufferPtr(uint64_t id);
    void RemoveMeshBuffer(uint64_t id);
}