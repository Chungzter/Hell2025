#pragma once
#include <Hell/Enums.h>
#include "API/OpenGL/Types/GL_heightmap_mesh.h"
#include "Types/GL_texture.h"
#include "Types/Renderer/Texture.h"
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
    void UpdateTextureBaking();
    void AllocateTextureMemory(Texture& texture);
    void ImmediateBake(QueuedTextureBake& queuedTextureBake);
    void AsyncBakeQueuedTextureBake(QueuedTextureBake& queuedTextureBake);
    void CleanUpBakingPBOs();
    const std::vector<GLuint64>& GetBindlessTextureIDs();

    // Buffers
    void UploadVertexData(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices);
    void UploadWeightedVertexDataOLD();
    void AllocateSkinnedVertexBufferSpace(uint32_t vertexCount);

    void SetDepthClearValue(float value);

    OpenGLHeightMapMesh& GetHeightMapMesh();

    GLuint GetVertexDataVAO();
    GLuint GetVertexDataVBO();
    GLuint GetVertexDataEBO();
    GLuint GetWeightedVertexDataVAO();
    GLuint GetWeightedVertexDataVBO();
    GLuint GetWeightedVertexDataEBO();
    GLuint GetSkinnedVertexDataVAO();
    GLuint GetSkinnedVertexDataVBO();
}