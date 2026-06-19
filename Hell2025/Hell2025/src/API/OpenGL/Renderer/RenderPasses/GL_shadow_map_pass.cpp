#include "API/OpenGL/GL_backend.h"
#include "API/OpenGL/Renderer/GL_renderer.h"
#include "AssetManagement/AssetManager.h"
#include "Core/Game.h"
#include "Renderer/RenderDataManager.h"
#include "Renderer/Renderer.h"
#include "Viewport/ViewportManager.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "World/World.h"

#include "Ragdoll/RagdollManager.h"

using namespace Hell;

namespace OpenGLRenderer {

    void RenderFlashLightShadowMaps();
    void RenderPointLightShadowMaps();
    void RenderMoonLightCascadedShadowMaps();

    void RenderShadowMaps() {
        RenderFlashLightShadowMaps();
        RenderPointLightShadowMaps();
        RenderMoonLightCascadedShadowMaps();
    }

    void RenderFlashLightShadowMaps() {
        ProfilerOpenGLZoneFunction();

        OpenGLShader* shader = GetShaderOLD("ShadowMap");
        OpenGLShadowMap* shadowMapsFBO = GetShadowMapOLD("FlashlightShadowMaps");
        OpenGLHeightMapMesh& heightMapMesh = OpenGLBackEnd::GetHeightMapMesh();
        //const DrawCommandsSet& drawInfoSet = RenderDataManager::GetDrawInfoSet();
        const FlashLightShadowMapDrawInfo& flashLightShadowMapDrawInfo = RenderDataManager::GetFlashLightShadowMapDrawInfo();
        
        glm::mat4 heightMapModelMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(HEIGHTMAP_SCALE_XZ, HEIGHTMAP_SCALE_Y, HEIGHTMAP_SCALE_XZ)); // move to height map manager

        glEnable(GL_DEPTH_TEST);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        shadowMapsFBO->Bind();
        shadowMapsFBO->SetViewport();

        shader->Bind();

        for (int i = 0; i < Game::GetLocalPlayerCount(); i++) {
            shadowMapsFBO->BindLayer(i);
            shadowMapsFBO->ClearLayer(i);

            glm::mat4 lightProjectionView = Game::GetLocalPlayerByIndex(i)->GetFlashlightProjectionView();
            shader->SetMat4("u_projectionView", lightProjectionView);

            Frustum frustum;
            frustum.Update(lightProjectionView);

            // Scene geometry
            shader->SetBool("u_useInstanceData", true);
            glCullFace(GL_FRONT);
            glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());

            MultiDrawIndirect(flashLightShadowMapDrawInfo.flashlightShadowMapGeometry[i]);

            // Heightfield chunks
            std::vector<HeightMapChunk>& chunks = World::GetHeightMapChunks();
            OpenGLHeightMapMesh& heightMapMesh = OpenGLBackEnd::GetHeightMapMesh();
            glBindVertexArray(heightMapMesh.GetVAO());
            shader->SetMat4("u_modelMatrix", heightMapModelMatrix);
            shader->SetBool("u_useInstanceData", false);

            for (uint32_t chunkIndex : flashLightShadowMapDrawInfo.heightMapChunkIndices[i]) {
                HeightMapChunk& chunk = chunks[chunkIndex];
                int indexCount = INDICES_PER_CHUNK;
                int baseVertex = 0;
                int baseIndex = chunk.baseIndex;
                void* indexOffset = (GLvoid*)(baseIndex * sizeof(GLuint));
                int instanceCount = 1;
                int viewportIndex = i;
                if (indexCount > 0) {
                    glDrawElementsInstancedBaseVertexBaseInstance(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, indexOffset, instanceCount, baseVertex, viewportIndex);
                }
            }

            // Procedural
            shader->SetMat4("u_modelMatrix", glm::mat4(1.0f));

            MeshBuffer& proceduralMeshBuffer = ResourceManager::GetMeshBuffer("Procedural");
            glBindVertexArray(proceduralMeshBuffer.GetVAO());

            const std::vector<RenderItem>& renderItems = RenderDataManager::GetRenderItemsProcedural();
            for (const RenderItem& renderItem : renderItems) {

                Mesh* mesh = proceduralMeshBuffer.GetMeshById(renderItem.meshId);
                if (!mesh) continue;

                int indexCount = mesh->indexCount;
                int baseVertex = renderItem.baseVertex;
                int baseIndex = renderItem.baseIndex;
                glDrawElementsBaseVertex(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * baseIndex), baseVertex);
            }
        }

        glBindVertexArray(0);
        glCullFace(GL_BACK);
    }

    void RenderPointLightShadowMaps() {
        ProfilerOpenGLZoneFunction();

        OpenGLShader* shader = GetShaderOLD("ShadowCubeMap");
        OpenGLShadowCubeMapArray* hiResShadowMaps = GetShadowCubeMapArrayOLD("HiRes");
        const DrawCommandsSet& drawInfoSet = RenderDataManager::GetDrawInfoSet();

        if (!shader) return;
        if (!hiResShadowMaps) return;

        shader->Bind();
        shader->SetBool("u_useInstanceData", true);

        const std::vector<GPULight>& gpuLightsHighRes = RenderDataManager::GetGPULightsHighRes();

		OpenGLRasterizerState state;
		state.depthMask = true;
		state.depthTestEnabled = true;
		state.blendEnable = false;
		state.cullfaceEnable = false;
		state.cullfaceMode = GL_FRONT;
		ForceRasterizerState(state);

        // Clear any shadow map that needs redrawing
        for (int i = 0; i < gpuLightsHighRes.size(); i++) {
            const GPULight& gpuLight = gpuLightsHighRes[i];
            Light* light = World::GetLightByIndex(gpuLight.lightIndex);
            
            if (light->IsDirtyForShadowMaps()) {
                std::cout << i << " is dirty\n";
                hiResShadowMaps->ClearDepthLayer(i, 1.0f);
            }
        }

        glViewport(0, 0, hiResShadowMaps->GetSize(), hiResShadowMaps->GetSize());
        glBindFramebuffer(GL_FRAMEBUFFER, hiResShadowMaps->GetHandle());

        glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());

        for (int i = 0; i < gpuLightsHighRes.size(); i++) {
            const GPULight& gpuLight = gpuLightsHighRes[i];

            Light* light = World::GetLightByIndex(gpuLight.lightIndex);
            if (!light || !light->IsDirtyForShadowMaps()) continue;

            shader->SetFloat("farPlane", light->GetRadius());
            shader->SetVec3("lightPosition", light->GetPosition());
            shader->SetMat4("shadowMatrices[0]", light->GetProjectionView(0));
            shader->SetMat4("shadowMatrices[1]", light->GetProjectionView(1));
            shader->SetMat4("shadowMatrices[2]", light->GetProjectionView(2));
            shader->SetMat4("shadowMatrices[3]", light->GetProjectionView(3));
            shader->SetMat4("shadowMatrices[4]", light->GetProjectionView(4));
            shader->SetMat4("shadowMatrices[5]", light->GetProjectionView(5));

            for (int face = 0; face < 6; ++face) {
                GLuint layer = i * 6 + face;
                shader->SetInt("faceIndex", face);
                glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, hiResShadowMaps->GetDepthTexture(), 0, layer);
                MultiDrawIndirect(drawInfoSet.shadowMapHiRes[i][face]);
            }
        }








        // HAIR

        glBindVertexArray(OpenGLBackEnd::GetSkinnedVertexDataVAO());
        glBindBuffer(GL_ARRAY_BUFFER, OpenGLBackEnd::GetSkinnedVertexDataVBO());
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, OpenGLBackEnd::GetVertexDataEBO());

        for (int i = 0; i < gpuLightsHighRes.size(); i++) {
            const GPULight& gpuLight = gpuLightsHighRes[i];

            Light* light = World::GetLightByIndex(gpuLight.lightIndex);
            if (!light || !light->IsDirtyForShadowMaps()) continue;

            shader->SetFloat("farPlane", light->GetRadius());
            shader->SetVec3("lightPosition", light->GetPosition());
            shader->SetMat4("shadowMatrices[0]", light->GetProjectionView(0));
            shader->SetMat4("shadowMatrices[1]", light->GetProjectionView(1));
            shader->SetMat4("shadowMatrices[2]", light->GetProjectionView(2));
            shader->SetMat4("shadowMatrices[3]", light->GetProjectionView(3));
            shader->SetMat4("shadowMatrices[4]", light->GetProjectionView(4));
            shader->SetMat4("shadowMatrices[5]", light->GetProjectionView(5));

            shader->SetBool("u_useInstanceData", false);

            for (int face = 0; face < 6; ++face) {
                shader->SetInt("faceIndex", face);
                int shadowMapIndex = i;
                GLuint layer = shadowMapIndex * 6 + face;

                glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, hiResShadowMaps->GetDepthTexture(), 0, layer);




                const std::vector<RenderItem>& instanceData = RenderDataManager::GetInstanceData();

                for (const DrawIndexedIndirectCommand& command : drawInfoSet.skinnedStandard[0]) {
                    int viewportIndex = command.baseInstance >> VIEWPORT_INDEX_SHIFT;
                    int instanceOffset = command.baseInstance & ((1 << VIEWPORT_INDEX_SHIFT) - 1);

                    for (GLuint i = 0; i < command.instanceCount; ++i) {
                        const RenderItem& renderItem = instanceData[instanceOffset + i];

                        shader->SetMat4("u_modelMatrix", renderItem.modelMatrix);

                        glDrawElementsBaseVertex(GL_TRIANGLES, command.indexCount, GL_UNSIGNED_INT, (GLvoid*)(command.firstIndex * sizeof(GLuint)), command.baseVertex);
                    }
                }

                for (const DrawIndexedIndirectCommand& command : drawInfoSet.skinnedHair[0]) {
                    int viewportIndex = command.baseInstance >> VIEWPORT_INDEX_SHIFT;
                    int instanceOffset = command.baseInstance & ((1 << VIEWPORT_INDEX_SHIFT) - 1);

                    for (GLuint i = 0; i < command.instanceCount; ++i) {
                        const RenderItem& renderItem = instanceData[instanceOffset + i];

                        shader->SetMat4("u_modelMatrix", renderItem.modelMatrix);

                        glDrawElementsBaseVertex(GL_TRIANGLES, command.indexCount, GL_UNSIGNED_INT, (GLvoid*)(command.firstIndex * sizeof(GLuint)), command.baseVertex);
                    }
                }



            }
        }


















        shader->SetBool("u_useInstanceData", false);
        shader->SetMat4("u_modelMatrix", glm::mat4(1.0f));

        // OPTIMIZE ME!
        // Make lights store a list of their HouseRenderItems per frustum face that is only updated when the map changes
        // That will be when a HousePlane or Wall is added/modified

        MeshBuffer& proceduralMeshBuffer = ResourceManager::GetMeshBuffer("Procedural");
        glBindVertexArray(proceduralMeshBuffer.GetVAO());
        
        for (int i = 0; i < gpuLightsHighRes.size(); i++) {
            const GPULight& gpuLight = gpuLightsHighRes[i];

            Light* light = World::GetLightByIndex(gpuLight.lightIndex);
            if (!light || !light->IsDirtyForShadowMaps()) continue;

            shader->SetFloat("farPlane", light->GetRadius());
            shader->SetVec3("lightPosition", light->GetPosition());
            shader->SetMat4("shadowMatrices[0]", light->GetProjectionView(0));
            shader->SetMat4("shadowMatrices[1]", light->GetProjectionView(1));
            shader->SetMat4("shadowMatrices[2]", light->GetProjectionView(2));
            shader->SetMat4("shadowMatrices[3]", light->GetProjectionView(3));
            shader->SetMat4("shadowMatrices[4]", light->GetProjectionView(4));
            shader->SetMat4("shadowMatrices[5]", light->GetProjectionView(5));

            for (int face = 0; face < 6; ++face) {
                shader->SetInt("faceIndex", face);
                int shadowMapIndex = i;
                GLuint layer = shadowMapIndex * 6 + face;

                glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, hiResShadowMaps->GetDepthTexture(), 0, layer);
                
                Frustum* frustum = light->GetFrustumByFaceIndex(face);
                if (!frustum) return;

                const std::vector<RenderItem>& renderItems = RenderDataManager::GetRenderItemsProcedural();
                for (const RenderItem& renderItem : renderItems) {

                    if (!frustum->IntersectsAABBFast(renderItem)) continue;

                    Mesh* mesh = proceduralMeshBuffer.GetMeshById(renderItem.meshId);
                    if (!mesh) continue;

                    int indexCount = mesh->indexCount;
                    int baseVertex = renderItem.baseVertex;
                    int baseIndex = renderItem.baseIndex;
                    glDrawElementsBaseVertex(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * baseIndex), baseVertex);
                }
            }
        }
    }


    void RenderMoonLightCascadedShadowMaps() {
        ProfilerOpenGLZoneFunction();

        const DrawCommandsSet& drawInfoSet = RenderDataManager::GetDrawInfoSet();

        OpenGLShader* shader = GetShaderOLD("ShadowMap");
        OpenGLShadowMapArray* shadowMapArray = GetShadowMapArrayOLD("MoonlightCSM");

        if (!shader) return;
        if (!shadowMapArray) return;

        int viewportCount = std::min(4, Game::GetLocalPlayerCount());

        for (int j = 0; j < viewportCount; j++) {
            Player* player = Game::GetLocalPlayerByIndex(j);
            if (!player || !player->ViewportIsVisible()) continue;

            const ViewportData& viewportData = RenderDataManager::GetViewportData()[j];

            shader->Bind();
            shader->SetBool("u_useInstanceData", false);

            size_t numLayers = SHADOW_CASCADE_COUNT;

            shadowMapArray->Bind();
            shadowMapArray->SetViewport();

            glDisable(GL_CULL_FACE);
            //glEnable(GL_CULL_FACE);
            //glCullFace(GL_FRONT);  // peter panning

            for (size_t i = 0; i < numLayers; ++i) {

                //int textureLayer = i + (viewportCount * j * numLayers);
                int textureLayer = int(i) + (j * int(numLayers)); // numLayers == SHADOW_CASCADE_COUNT

                shadowMapArray->SetTextureLayer(textureLayer);
                shadowMapArray->ClearDepth();

                const glm::mat4& lightProjectionView = viewportData.csmLightProjectionView[i];

                shader->SetMat4("u_projectionView", lightProjectionView);

                // Geometry
                glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());

                shader->SetBool("u_useInstanceData", true);
                MultiDrawIndirect(drawInfoSet.moonLightCascades[j][i]);

                shader->SetBool("u_useInstanceData", false);
                shader->SetMat4("u_modelMatrix", glm::mat4(1.0f));

                // Procedural
                MeshBuffer& proceduralMeshBuffer = ResourceManager::GetMeshBuffer("Procedural");
                glBindVertexArray(proceduralMeshBuffer.GetVAO());

                //glDisable(GL_CULL_FACE);
                const std::vector<RenderItem>& renderItems = RenderDataManager::GetRenderItemsProcedural();
                for (const RenderItem& renderItem : renderItems) {
                    Mesh* mesh = proceduralMeshBuffer.GetMeshById(renderItem.meshId);
                    if (!mesh) continue;

                    int indexCount = mesh->indexCount;
                    int baseVertex = renderItem.baseVertex;
                    int baseIndex = renderItem.baseIndex;
                    glDrawElementsBaseVertex(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * baseIndex), baseVertex);
                }

                // Weather boards
                //MeshBuffer weatherboardMeshBuffer = World::GetWeatherBoardMeshBuffer();
                //glBindVertexArray(weatherboardMeshBuffer.GetGLMeshBuffer().GetVAO());
                //int indexCount = weatherboardMeshBuffer.GetGLMeshBuffer().GetIndexCount();
                //if (indexCount > 0) {
                //    int baseIndex = 0;
                //    int baseVertex = 0;
                //    glDrawElementsBaseVertex(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * baseIndex), baseVertex);
                //}
            }
        }
        glCullFace(GL_BACK);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}