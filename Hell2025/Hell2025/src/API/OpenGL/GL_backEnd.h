#pragma once


#include "Hell/Common.h"
#include "Hell/Render/VertexAttributes.h"

#include "API/OpenGL/Types/GL_heightmap_mesh.h"
#include "Game/Types.h"
#include "Types/GL_texture.h"
#include "Hell/ResourceManagement/Types/Texture.h"
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <string>
#include <iostream>

namespace OpenGLBackEnd {
    // Core
    void Init();
	void BeginFrame();
	bool CheckSupport();

    // Textures
    void AllocateTextureMemory(Texture& texture);
    const std::vector<GLuint64>& GetBindlessTextureIDs();

    // Buffers
    void AllocateSkinnedVertexBufferSpace(uint32_t vertexCount);

    void SetDepthClearValue(float value);

    OpenGLHeightMapMesh& GetHeightMapMesh();

    GLuint GetSkinnedVertexDataVAO();
    GLuint GetSkinnedVertexDataVBO();
}
