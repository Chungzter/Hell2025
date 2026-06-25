#include "API/OpenGL/Renderer/GL_renderer.h"
#include "API/OpenGL/GL_backend.h"
#include "Hell/Backend/BackEnd.h"
#include "Debug/DebugDraw.h"
#include "Renderer/RenderDataManager.h"
#include "Viewport/ViewportManager.h"
#include "World/LegacyWorld.h"

#include "Hell/Logging.h"

// Pretty sure this whole file is unused
// Pretty sure this whole file is unused
// Pretty sure this whole file is unused
// Pretty sure this whole file is unused
// Pretty sure this whole file is unused

namespace OpenGLRenderer {

    void ReserveLightAABBSSBOStorage() {
        uint32_t size = LegacyWorld::GetLightCount() * sizeof(glm::vec4) * 2;
        ReserveSSBO("LightAABBs", size);
    }

    void RenderWorldPosition(uint32_t lightIndex);
    void ComputeMinMax(uint32_t lightIndex);

    void DrawHouse(OpenGLShader* shader);
    void DrawHeightMap(OpenGLShader* shader, Light* light);
    void DebugDrawLightAABB(uint32_t lightIndex);

    void ComputeLightAABBs() {
    }

    void RenderWorldPosition(uint32_t lightIndex) {
        ProfilerOpenGLZoneFunctionLightGreen();

        OpenGLShader* shader = GetShaderOLD("LightAABBPosition");
        if (!shader) return;

        Light* light = LegacyWorld::GetLightByIndex(lightIndex);
        if (!light) return;

        OpenGLCubemapFrameBuffer& fbo = GetCubemapFrameBuffer("LightAABB");
        fbo.Bind();
        fbo.SetViewport();

        shader->Bind();
        BindSSBO(5, "Lights");

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        glDisable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);

        glm::vec3 lightPos = light->GetPosition();
        float radius = light->GetRadius();

        shader->SetInt("u_lightIndex", lightIndex);
        shader->SetFloat("u_lightRadius", radius);
        shader->SetVec3("u_lightPosition", lightPos);

        const glm::vec3 faceDirs[6] = {
            { 1,  0,  0}, // +X
            {-1,  0,  0}, // -X
            { 0,  1,  0}, // +Y
            { 0, -1,  0}, // -Y
            { 0,  0,  1}, // +Z
            { 0,  0, -1}  // -Z
        };

        for (int face = 0; face < 6; ++face) {
            fbo.BindFaceByIndex(face);
            fbo.ClearFaceDepth(1.0f);

            // Clear to the far extent of the light radius for this face
            glm::vec3 farPoint = lightPos + (faceDirs[face] * radius);
            fbo.ClearFaceColor(glm::vec4(farPoint, 1.0f));

            shader->SetInt("u_faceIndex", face);
            shader->SetMat4("u_shadowMatrix", light->GetProjectionView(face));

            DrawHouse(shader);
            DrawHeightMap(shader, light);
        }
        glBindVertexArray(0);
    }

    void DrawHouse(OpenGLShader* shader) {
        Logging::Fatal() << "You called the suss function DrawHouse()\n";
        //OpenGLMeshBuffer& houseMeshBuffer = LegacyWorld::GetHouseMeshBuffer().GetGLMeshBuffer();
        //glBindVertexArray(houseMeshBuffer.GetVAO());
        //
        //shader->SetMat4("u_model", glm::mat4(1.0f));
        //
        //for (const HouseRenderItem& renderItem : RenderDataManager::GetHouseRenderItems()) {
        //    glDrawElementsBaseVertex(
        //        GL_TRIANGLES,
        //        renderItem.indexCount,
        //        GL_UNSIGNED_INT,
        //        (void*)(sizeof(unsigned int) * renderItem.baseIndex),
        //        renderItem.baseVertex
        //    );
        //}
        //
    }

    void DrawHeightMap(OpenGLShader* shader, Light* light) {
        OpenGLHeightMapMesh& heightMapMesh = OpenGLBackEnd::GetHeightMapMesh();

        Transform transform;
        transform.scale = glm::vec3(HEIGHTMAP_SCALE_XZ, HEIGHTMAP_SCALE_Y, HEIGHTMAP_SCALE_XZ);
        glm::mat4 modelMatrix = transform.to_mat4();
        glm::mat4 inverseModelMatrix = glm::inverse(modelMatrix);

        shader->SetMat4("u_model", modelMatrix);

        glBindVertexArray(heightMapMesh.GetVAO());

        int verticesPerChunk = 33 * 33;
        int verticesPerHeightMap = verticesPerChunk * 8 * 8;
        int indicesPerChunk = 32 * 32 * 6;
        int indicesPerHeightMap = indicesPerChunk * 8 * 8;

        for (HeightMapChunk& chunk : LegacyWorld::GetHeightMapChunks()) {

            // Skip any chunks that don't intersect the light radius
            AABB chunkAABB(chunk.aabbMin, chunk.aabbMax);
            if (!chunkAABB.IntersectsSphere(light->GetPosition(), light->GetRadius())) {
                continue;
            }

            int indexCount = INDICES_PER_CHUNK;
            int baseVertex = 0;
            int baseIndex = chunk.baseIndex;
            void* indexOffset = (GLvoid*)(baseIndex * sizeof(GLuint));
            int instanceCount = 1;
            glDrawElementsInstancedBaseVertexBaseInstance(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, indexOffset, instanceCount, baseVertex, 0);
        }
    }

    uint32_t FloatToUint(float f) {
        uint32_t u;
        memcpy(&u, &f, sizeof(float));
        return (u & 0x80000000) ? ~u : u | 0x80000000;
    }

    void ComputeMinMax(uint32_t lightIndex) {
        OpenGLShader* shader = GetShaderOLD("LightAABBMinMax");
        OpenGLSSBO* ssbo = GetSSBO("LightAABBs");
        Light* light = LegacyWorld::GetLightByIndex(lightIndex);

        if (!shader) return;
        if (!ssbo) return;
        if (!light) return;

        OpenGLCubemapFrameBuffer& fbo = GetCubemapFrameBuffer("LightAABB");

        unsigned int minBits = 0xFFFFFFFF;
        unsigned int maxBits = 0;

        // Reset with flipped bits
        struct { uint32_t x, y, z, w; } minU, maxU;
        minU.x = maxU.x = FloatToUint(light->GetPosition().x);
        minU.y = maxU.y = FloatToUint(light->GetPosition().y);
        minU.z = maxU.z = FloatToUint(light->GetPosition().z);
        minU.w = maxU.w = 0;

        size_t baseOffset = lightIndex * sizeof(glm::vec4) * 2;
        glNamedBufferSubData(ssbo->GetHandle(), baseOffset, sizeof(minU), &minU);
        glNamedBufferSubData(ssbo->GetHandle(), baseOffset + sizeof(glm::vec4), sizeof(maxU), &maxU);

        shader->Bind();
        shader->SetInt("u_lightIndex", lightIndex);
        shader->SetInt("u_resolution", fbo.GetSize());

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, fbo.GetColorHandle());
        shader->SetInt("u_WorldPosCubemap", 0);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, ssbo->GetHandle());

        uint32_t numGroups = fbo.GetSize() / 16;
        //glDispatchCompute(numGroups, numGroups, 1);
        glDispatchCompute(1, 1, 1);

        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

	void DebugDrawLightAABB(uint32_t lightIndex) {
		static GLuint vao = 0;
		if (vao == 0) {
			glGenVertexArrays(1, &vao);
			glBindVertexArray(vao);
		}

		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		OpenGLFrameBuffer* gBuffer = GetFrameBufferOLD("GBuffer");
		gBuffer->Bind();
		gBuffer->DrawBuffer("Lighting");

		BindShader("DebugLightAABB");

		BindSSBO(4, "Lights");
		BindSSBO(5, "LightAABBs");

		glBindVertexArray(vao);

		for (int i = 0; i < 4; i++) {
			Viewport* viewport = ViewportManager::GetViewportByIndex(i);
			if (viewport->IsVisible()) {
				OpenGLRenderer::SetViewport(gBuffer, viewport);
				SetUniformMat4("u_projectionView", RenderDataManager::GetViewportData()[i].projectionView);
                SetUniformInt("u_lightIndex", lightIndex);
                glDrawArrays(GL_LINE_STRIP, 0, 16);
			}
		}

        DebugDraw::DrawPoint(LegacyWorld::GetLightByIndex(lightIndex)->GetPosition(), YELLOW);
	}
}