#include "Hell/Render/API/OpenGL/GL_back_end.h"
#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Viewport/ViewportManager.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Unloved/Objects/Lighting/Light.h"
#include "Unloved/Systems/ShadowMaps/ShadowMapManager.h"
#include "Unloved/World/World.h"
#include "World/LegacyWorld.h"

#include <cstddef>
#include <initializer_list>

using namespace Hell;            // lil bit cursed dis

namespace OpenGL::Renderer {

    using namespace Unloved;     // lil bit cursed dis

    namespace {
        constexpr uint32_t POINT_SHADOW_FACE_COUNT = 6;
        constexpr uint32_t POINT_SHADOW_INSTANCE_INDEX_MASK = (1u << VIEWPORT_INDEX_SHIFT) - 1;

        struct alignas(16) OpenGLPointShadowFaceData {
            glm::mat4 projectionView = glm::mat4(1.0f);
            glm::vec4 lightPositionRadius = glm::vec4(0.0f);
            glm::uvec4 arrayLayer = glm::uvec4(0);
        };

        struct OpenGLPointShadowDrawBatch {
            size_t byteOffset = 0;
            GLsizei count = 0;
        };

        size_t GetPointShadowFaceDrawCommandCount(const PointLightShadowMapDrawCommands& drawCommands, uint32_t shadowMapIndex, uint32_t faceIndex) {
            return drawCommands.procedural[shadowMapIndex][faceIndex].size()
                + drawCommands.assetGeometry[shadowMapIndex][faceIndex].size()
                + drawCommands.assetGeometrySkinnedNonDeforming[shadowMapIndex][faceIndex].size()
                + drawCommands.assetGeometrySkinned[shadowMapIndex][faceIndex].size()
                + drawCommands.assetGeometryAlphaDiscard[shadowMapIndex][faceIndex].size()
                + drawCommands.assetGeometryHair[shadowMapIndex][faceIndex].size()
                + drawCommands.assetGeometrySkinnedNonDeformingAlphaDiscard[shadowMapIndex][faceIndex].size()
                + drawCommands.assetGeometrySkinnedNonDeformingHair[shadowMapIndex][faceIndex].size()
                + drawCommands.assetGeometrySkinnedAlphaDiscard[shadowMapIndex][faceIndex].size()
                + drawCommands.assetGeometrySkinnedHair[shadowMapIndex][faceIndex].size();
        }

        size_t GetPointShadowDrawCommandCount(const PointLightShadowMapDrawCommands& drawCommands, const std::vector<ShadowMapInfo>& shadowMapInfos, uint32_t shadowMapCount) {
            size_t count = 0;
            for (const ShadowMapInfo& shadowMapInfo : shadowMapInfos) {
                if (shadowMapInfo.shadowMapIndex < 0 || shadowMapInfo.shadowMapIndex >= MAX_SHADOW_MAP_ARRAY_LEVELS || shadowMapInfo.shadowMapIndex >= static_cast<int32_t>(shadowMapCount)) continue;
                const uint32_t shadowMapIndex = static_cast<uint32_t>(shadowMapInfo.shadowMapIndex);
                for (uint32_t faceIndex = 0; faceIndex < POINT_SHADOW_FACE_COUNT; faceIndex++) {
                    count += GetPointShadowFaceDrawCommandCount(drawCommands, shadowMapIndex, faceIndex);
                }
            }
            return count;
        }

        void AppendPointShadowDrawCommands(std::vector<DrawIndexedIndirectCommand>& destination, uint32_t faceDataIndex, std::initializer_list<const std::vector<DrawIndexedIndirectCommand>*> sources) {
            for (const std::vector<DrawIndexedIndirectCommand>* source : sources) {
                if (!source || source->empty()) continue;
                for (const DrawIndexedIndirectCommand& command : *source) {
                    DrawIndexedIndirectCommand& encodedCommand = destination.emplace_back(command);
                    encodedCommand.baseInstance = (faceDataIndex << VIEWPORT_INDEX_SHIFT) | (command.baseInstance & POINT_SHADOW_INSTANCE_INDEX_MASK);
                }
            }
        }

        OpenGLPointShadowDrawBatch AppendPointShadowDrawBatch(std::vector<DrawIndexedIndirectCommand>& destination, const std::vector<DrawIndexedIndirectCommand>& source) {
            OpenGLPointShadowDrawBatch batch;
            batch.byteOffset = destination.size() * sizeof(DrawIndexedIndirectCommand);
            batch.count = static_cast<GLsizei>(source.size());
            destination.insert(destination.end(), source.begin(), source.end());
            return batch;
        }

        void DrawPointShadowBatch(const OpenGLPointShadowDrawBatch& batch) {
            if (batch.count == 0) return;
            glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, reinterpret_cast<const void*>(batch.byteOffset), batch.count, 0);
        }
    }


    void RenderFlashLightShadowMaps();
    void PointLightShadowPass();
    void PreparePointLightShadowMapArray(OpenGLShadowCubeMapArray* destination, OpenGLShadowCubeMapArray* source, const std::vector<ShadowMapInfo>& shadowMapInfoSet);
    void RenderPointLightShadowMapArray(OpenGLShadowCubeMapArray* shadowMaps, const std::vector<ShadowMapInfo>& shadowMapInfoSet, const PointLightShadowMapDrawCommands& drawCommands, const char* faceDataBufferName, const char* drawCommandBufferName);
    void RenderMoonLightCascadedShadowMaps();

    void RenderShadowMaps() {
        RenderFlashLightShadowMaps();
        PointLightShadowPass();
        RenderMoonLightCascadedShadowMaps();
    }

    void RenderFlashLightShadowMaps() {
        ProfilerOpenGLZoneFunction();

        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("ShadowMap");
        OpenGLShadowMap* shadowMapsFBO = OpenGL::ResourceManager::GetShadowMapPtr("FlashlightShadowMaps");
        //const DrawCommandsSet& drawInfoSet = Unloved::RenderDataManager::GetDrawInfoSet();
        const FlashLightShadowMapDrawInfo& flashLightShadowMapDrawInfo = Unloved::RenderDataManager::GetFlashLightShadowMapDrawInfo();
        MeshBuffer& meshBufferProcedural = Hell::ResourceManager::GetMeshBuffer("Procedural");
        OpenGLMeshBuffer& glMeshBufferAssets = OpenGL::ResourceManager::GetMeshBuffer("AssetGeometry");
        OpenGLMeshBuffer& glMeshBufferProcedural = OpenGL::ResourceManager::GetMeshBuffer("Procedural");

        glm::mat4 heightMapModelMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(HEIGHTMAP_SCALE_XZ, HEIGHTMAP_SCALE_Y, HEIGHTMAP_SCALE_XZ)); // move to height map manager

        glEnable(GL_DEPTH_TEST);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        shadowMapsFBO->Bind();
        shadowMapsFBO->SetViewport();

        OpenGL::BindShader("ShadowMap");
        OpenGL::BindSSBO(SSBO_IDX_INSTANCE_DATA, "InstanceData");

        for (int i = 0; i < Unloved::Session::GetLocalPlayerCount(); i++) {
            shadowMapsFBO->BindLayer(i);
            shadowMapsFBO->ClearLayer(i);

            glm::mat4 lightProjectionView = Unloved::Session::GetLocalPlayerByViewportIndex(i)->GetFlashlightProjectionView();
            OpenGL::SetUniformMat4("u_projectionView", lightProjectionView);

            Unloved::Frustum frustum;
            frustum.Update(lightProjectionView);

            // Scene geometry
            OpenGL::SetUniformBool("u_useInstanceData", true);
            glCullFace(GL_FRONT);
            glBindVertexArray(glMeshBufferAssets.GetVAO());

            MultiDrawIndirect(flashLightShadowMapDrawInfo.flashlightShadowMapGeometry[i]);

            // Heightfield chunks
            std::vector<HeightMapChunk>& chunks = LegacyWorld::GetHeightMapChunks();
            OpenGL::SetUniformBool("u_useInstanceData", false);
            if (!chunks.empty()) {
                MeshBuffer& heightMapMeshBuffer = Hell::ResourceManager::GetMeshBuffer("HeightMapGeometry");
                OpenGLMeshBuffer& glHeightMapMeshBuffer = OpenGL::ResourceManager::GetMeshBuffer("HeightMapGeometry");
                glBindVertexArray(glHeightMapMeshBuffer.GetVAO());
                OpenGL::SetUniformMat4("u_modelMatrix", heightMapModelMatrix);

                for (uint32_t chunkIndex : flashLightShadowMapDrawInfo.heightMapChunkIndices[i]) {
                    HeightMapChunk& chunk = chunks[chunkIndex];
                    Mesh* mesh = heightMapMeshBuffer.GetMeshById(chunk.meshId);
                    if (!mesh) continue;

                    int indexCount = mesh->indexCount;
                    int baseVertex = mesh->baseVertex;
                    int baseIndex = mesh->baseIndex;
                    void* indexOffset = (GLvoid*)(baseIndex * sizeof(GLuint));
                    int instanceCount = 1;
                    int viewportIndex = i;
                    if (indexCount > 0) {
                        glDrawElementsInstancedBaseVertexBaseInstance(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, indexOffset, instanceCount, baseVertex, viewportIndex);
                    }
                }
            }

            // Procedural
            OpenGL::SetUniformMat4("u_modelMatrix", glm::mat4(1.0f));

            glBindVertexArray(glMeshBufferProcedural.GetVAO());

            const std::vector<RenderItem>& renderItems = Unloved::RenderDataManager::GetRenderItemsProcedural();
            for (const RenderItem& renderItem : renderItems) {

                Mesh* mesh = meshBufferProcedural.GetMeshById(renderItem.meshId);
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

    void PointLightShadowPass() {
        ProfilerOpenGLZoneFunction();

        OpenGLShader* opaqueShader = OpenGL::ResourceManager::GetShaderPtr("ShadowCubeMap");
        OpenGLShader* alphaDiscardShader = OpenGL::ResourceManager::GetShaderPtr("ShadowCubeMapAlphaDiscard");
        if (!opaqueShader || !alphaDiscardShader) return;

        const std::vector<ShadowMapInfo>& staticHiResShadowMaps = ShadowMapManager::GetStaticDirtyHiResShadowMaps();
        const std::vector<ShadowMapInfo>& staticLowResShadowMaps = ShadowMapManager::GetStaticDirtyLowResShadowMaps();
        const std::vector<ShadowMapInfo>& compositeHiResShadowMaps = ShadowMapManager::GetCompositeDirtyHiResShadowMaps();
        const std::vector<ShadowMapInfo>& compositeLowResShadowMaps = ShadowMapManager::GetCompositeDirtyLowResShadowMaps();
        if (staticHiResShadowMaps.empty() && staticLowResShadowMaps.empty() && compositeHiResShadowMaps.empty() && compositeLowResShadowMaps.empty()) return;

        OpenGL::BindShader("ShadowCubeMap");
        OpenGL::BindSSBO(SSBO_IDX_SAMPLERS, "Samplers");
        OpenGL::BindSSBO(SSBO_IDX_MATERIALS, "Materials");
        OpenGL::BindSSBO(SSBO_IDX_INSTANCE_DATA, "InstanceData");

		OpenGLRasterizerState state;
		state.depthMask = true;
		state.depthTestEnabled = true;
		state.depthFunc = GL_LESS;
		state.blendEnable = false;
		state.cullfaceEnable = true;
		state.cullfaceMode = GL_FRONT;
        OpenGL::RasterizerStateManager::ForceRasterizerState(state);

        const DrawCommandsSet& drawInfoSet = Unloved::RenderDataManager::GetDrawInfoSet();
        OpenGLShadowCubeMapArray* staticHiRes = OpenGL::ResourceManager::GetShadowCubeMapArrayPtr("StaticHiRes");
        OpenGLShadowCubeMapArray* staticLowRes = OpenGL::ResourceManager::GetShadowCubeMapArrayPtr("StaticLowRes");
        OpenGLShadowCubeMapArray* compositeHiRes = OpenGL::ResourceManager::GetShadowCubeMapArrayPtr("HiRes");
        OpenGLShadowCubeMapArray* compositeLowRes = OpenGL::ResourceManager::GetShadowCubeMapArrayPtr("LowRes");
        const bool staticCacheEnabled = ShadowMapManager::StaticCacheEnabled();

        if (staticCacheEnabled && (!staticHiRes || !staticLowRes)) return;

        {
            ProfilerOpenGLZone("Point Shadow Static Cache");
            PreparePointLightShadowMapArray(staticHiRes, nullptr, staticHiResShadowMaps);
            PreparePointLightShadowMapArray(staticLowRes, nullptr, staticLowResShadowMaps);
            RenderPointLightShadowMapArray(staticHiRes, staticHiResShadowMaps, drawInfoSet.staticHiResShadowMapDrawCommands, "PointShadowStaticHiResFaceData", "PointShadowStaticHiResDrawCommands");
            RenderPointLightShadowMapArray(staticLowRes, staticLowResShadowMaps, drawInfoSet.staticLowResShadowMapDrawCommands, "PointShadowStaticLowResFaceData", "PointShadowStaticLowResDrawCommands");
        }

        OpenGLShadowCubeMapArray* hiResSource = staticCacheEnabled ? staticHiRes : nullptr;
        OpenGLShadowCubeMapArray* lowResSource = staticCacheEnabled ? staticLowRes : nullptr;

        {
            ProfilerOpenGLZone("Point Shadow Composite");
            PreparePointLightShadowMapArray(compositeHiRes, hiResSource, compositeHiResShadowMaps);
            PreparePointLightShadowMapArray(compositeLowRes, lowResSource, compositeLowResShadowMaps);
            RenderPointLightShadowMapArray(compositeHiRes, compositeHiResShadowMaps, drawInfoSet.compositeHiResShadowMapDrawCommands, "PointShadowHiResFaceData", "PointShadowHiResDrawCommands");
            RenderPointLightShadowMapArray(compositeLowRes, compositeLowResShadowMaps, drawInfoSet.compositeLowResShadowMapDrawCommands, "PointShadowLowResFaceData", "PointShadowLowResDrawCommands");
        }
    }

    void PreparePointLightShadowMapArray(OpenGLShadowCubeMapArray* destination, OpenGLShadowCubeMapArray* source, const std::vector<ShadowMapInfo>& shadowMapInfoSet) {
        if (!destination) return;

        // A source seeds a cached composite
        // No source selects the uncached "clear and render" path

        for (const ShadowMapInfo& shadowMapInfo : shadowMapInfoSet) {
            const int shadowMapIndex = shadowMapInfo.shadowMapIndex;
            if (shadowMapIndex == -1) continue;

            if (source) {
                BlitShadowCubeMapArray(*source, *destination, shadowMapIndex, shadowMapIndex);
            }
            else {
                destination->ClearDepthLayer(shadowMapIndex, 1.0f);
            }
        }
    }

    void RenderPointLightShadowMapArray(OpenGLShadowCubeMapArray* shadowMaps, const std::vector<ShadowMapInfo>& shadowMapInfoSet, const PointLightShadowMapDrawCommands& drawCommands, const char* faceDataBufferName, const char* drawCommandBufferName) {
        if (!shadowMaps || shadowMapInfoSet.empty()) return;

        OpenGLSSBO* faceDataBuffer = OpenGL::ResourceManager::GetSSBOPtr(faceDataBufferName);
        OpenGLSSBO* drawCommandBuffer = OpenGL::ResourceManager::GetSSBOPtr(drawCommandBufferName);
        if (!faceDataBuffer || !drawCommandBuffer) return;

        OpenGLMeshBuffer& meshBufferAssets = OpenGL::ResourceManager::GetMeshBuffer("AssetGeometry");
        OpenGLMeshBuffer& meshBufferProcedural = OpenGL::ResourceManager::GetMeshBuffer("Procedural");

        OpenGLRasterizerState solidShadowState;
        solidShadowState.depthMask = true;
        solidShadowState.depthTestEnabled = true;
        solidShadowState.depthFunc = GL_LESS;
        solidShadowState.blendEnable = false;
        solidShadowState.cullfaceEnable = true;
        solidShadowState.cullfaceMode = GL_FRONT;

        OpenGLRasterizerState alphaShadowState = solidShadowState;
        alphaShadowState.cullfaceEnable = false;

        std::vector<OpenGLPointShadowFaceData> faceData;
        std::vector<DrawIndexedIndirectCommand> proceduralCommands;
        std::vector<DrawIndexedIndirectCommand> opaqueAssetCommands;
        std::vector<DrawIndexedIndirectCommand> opaqueSkinnedCommands;
        std::vector<DrawIndexedIndirectCommand> alphaTestedAssetCommands;
        std::vector<DrawIndexedIndirectCommand> alphaTestedSkinnedCommands;
        faceData.reserve(shadowMapInfoSet.size() * POINT_SHADOW_FACE_COUNT);

        const size_t drawCommandCount = GetPointShadowDrawCommandCount(drawCommands, shadowMapInfoSet, shadowMaps->GetLayerCount());
        proceduralCommands.reserve(drawCommandCount / 5);
        opaqueAssetCommands.reserve(drawCommandCount / 5);
        opaqueSkinnedCommands.reserve(drawCommandCount / 5);
        alphaTestedAssetCommands.reserve(drawCommandCount / 5);
        alphaTestedSkinnedCommands.reserve(drawCommandCount / 5);

        for (const ShadowMapInfo& shadowMapInfo : shadowMapInfoSet) {
            if (shadowMapInfo.shadowMapIndex < 0 || shadowMapInfo.shadowMapIndex >= MAX_SHADOW_MAP_ARRAY_LEVELS || shadowMapInfo.shadowMapIndex >= static_cast<int32_t>(shadowMaps->GetLayerCount())) continue;

            Light* light = Unloved::World::GetLightByObjectId(shadowMapInfo.lightId);
            if (!light) continue;

            const uint32_t shadowMapIndex = static_cast<uint32_t>(shadowMapInfo.shadowMapIndex);
            for (uint32_t faceIndex = 0; faceIndex < POINT_SHADOW_FACE_COUNT; faceIndex++) {
                const uint32_t faceDataIndex = static_cast<uint32_t>(faceData.size());
                OpenGLPointShadowFaceData& currentFaceData = faceData.emplace_back();
                currentFaceData.projectionView = light->GetProjectionView(faceIndex);
                currentFaceData.lightPositionRadius = glm::vec4(light->GetPosition(), light->GetRadius());
                currentFaceData.arrayLayer.x = shadowMapIndex * POINT_SHADOW_FACE_COUNT + faceIndex;

                AppendPointShadowDrawCommands(proceduralCommands, faceDataIndex, { &drawCommands.procedural[shadowMapIndex][faceIndex] });
                AppendPointShadowDrawCommands(opaqueAssetCommands, faceDataIndex, { &drawCommands.assetGeometry[shadowMapIndex][faceIndex], &drawCommands.assetGeometrySkinnedNonDeforming[shadowMapIndex][faceIndex] });
                AppendPointShadowDrawCommands(opaqueSkinnedCommands, faceDataIndex, { &drawCommands.assetGeometrySkinned[shadowMapIndex][faceIndex] });
                AppendPointShadowDrawCommands(alphaTestedAssetCommands, faceDataIndex, { &drawCommands.assetGeometryAlphaDiscard[shadowMapIndex][faceIndex], &drawCommands.assetGeometryHair[shadowMapIndex][faceIndex], &drawCommands.assetGeometrySkinnedNonDeformingAlphaDiscard[shadowMapIndex][faceIndex], &drawCommands.assetGeometrySkinnedNonDeformingHair[shadowMapIndex][faceIndex] });
                AppendPointShadowDrawCommands(alphaTestedSkinnedCommands, faceDataIndex, { &drawCommands.assetGeometrySkinnedAlphaDiscard[shadowMapIndex][faceIndex], &drawCommands.assetGeometrySkinnedHair[shadowMapIndex][faceIndex] });
            }
        }

        if (faceData.empty() || drawCommandCount == 0) return;

        std::vector<DrawIndexedIndirectCommand> combinedCommands;
        combinedCommands.reserve(drawCommandCount);
        const OpenGLPointShadowDrawBatch proceduralBatch = AppendPointShadowDrawBatch(combinedCommands, proceduralCommands);
        const OpenGLPointShadowDrawBatch opaqueAssetBatch = AppendPointShadowDrawBatch(combinedCommands, opaqueAssetCommands);
        const OpenGLPointShadowDrawBatch opaqueSkinnedBatch = AppendPointShadowDrawBatch(combinedCommands, opaqueSkinnedCommands);
        const OpenGLPointShadowDrawBatch alphaTestedAssetBatch = AppendPointShadowDrawBatch(combinedCommands, alphaTestedAssetCommands);
        const OpenGLPointShadowDrawBatch alphaTestedSkinnedBatch = AppendPointShadowDrawBatch(combinedCommands, alphaTestedSkinnedCommands);

        if (combinedCommands.empty()) return;

        faceDataBuffer->Update(sizeof(OpenGLPointShadowFaceData) * faceData.size(), faceData.data());
        drawCommandBuffer->Update(sizeof(DrawIndexedIndirectCommand) * combinedCommands.size(), combinedCommands.data());
        faceDataBuffer->Bind(SSBO_IDX_POINT_SHADOW_FACE_DATA);
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, drawCommandBuffer->GetHandle());

        glViewport(0, 0, shadowMaps->GetSize(), shadowMaps->GetSize());
        glBindFramebuffer(GL_FRAMEBUFFER, shadowMaps->GetHandle());
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadowMaps->GetDepthTexture(), 0);

        OpenGL::BindShader("ShadowCubeMap");
        OpenGL::RasterizerStateManager::SetRasterizerState(solidShadowState);

        if (proceduralBatch.count > 0) {
            glBindVertexArray(meshBufferProcedural.GetVAO());
            DrawPointShadowBatch(proceduralBatch);
        }
        if (opaqueAssetBatch.count > 0) {
            glBindVertexArray(meshBufferAssets.GetVAO());
            DrawPointShadowBatch(opaqueAssetBatch);
        }
        if (opaqueSkinnedBatch.count > 0) {
            glBindVertexArray(OpenGL::BackEnd::GetSkinnedVertexDataVAO());
            glBindBuffer(GL_ARRAY_BUFFER, OpenGL::BackEnd::GetSkinnedVertexDataVBO());
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshBufferAssets.GetEBO());
            DrawPointShadowBatch(opaqueSkinnedBatch);
        }

        if (alphaTestedAssetBatch.count > 0 || alphaTestedSkinnedBatch.count > 0) {
            OpenGL::BindShader("ShadowCubeMapAlphaDiscard");
            OpenGL::RasterizerStateManager::SetRasterizerState(alphaShadowState);

            if (alphaTestedAssetBatch.count > 0) {
                glBindVertexArray(meshBufferAssets.GetVAO());
                DrawPointShadowBatch(alphaTestedAssetBatch);
            }
            if (alphaTestedSkinnedBatch.count > 0) {
                glBindVertexArray(OpenGL::BackEnd::GetSkinnedVertexDataVAO());
                glBindBuffer(GL_ARRAY_BUFFER, OpenGL::BackEnd::GetSkinnedVertexDataVBO());
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshBufferAssets.GetEBO());
                DrawPointShadowBatch(alphaTestedSkinnedBatch);
            }
        }
    }


    void RenderMoonLightCascadedShadowMaps() {
        ProfilerOpenGLZoneFunction();

        const DrawCommandsSet& drawInfoSet = Unloved::RenderDataManager::GetDrawInfoSet();

        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("ShadowMap");
        OpenGLShadowMapArray* shadowMapArray = OpenGL::ResourceManager::GetShadowMapArrayPtr("MoonlightCSM");

        if (!shader) return;
        if (!shadowMapArray) return;

        MeshBuffer& meshBufferProcedural = Hell::ResourceManager::GetMeshBuffer("Procedural");
        OpenGLMeshBuffer& glMeshBufferAssets = OpenGL::ResourceManager::GetMeshBuffer("AssetGeometry");
        OpenGLMeshBuffer& glMeshBufferProcedural = OpenGL::ResourceManager::GetMeshBuffer("Procedural");

        int viewportCount = std::min(4, Unloved::Session::GetLocalPlayerCount());

        OpenGLRasterizerState state;
        state.depthMask = true;
        state.depthTestEnabled = true;
        state.blendEnable = false;
        state.cullfaceEnable = false;
        OpenGL::RasterizerStateManager::ForceRasterizerState(state);

        for (int j = 0; j < viewportCount; j++) {
            Unloved::Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(j);
            if (!player || !player->ViewportIsVisible()) continue;

            const ViewportData& viewportData = Unloved::RenderDataManager::GetViewportData()[j];

            OpenGL::BindShader("ShadowMap");
            OpenGL::BindSSBO(SSBO_IDX_INSTANCE_DATA, "InstanceData");
            OpenGL::SetUniformBool("u_useInstanceData", false);

            size_t numLayers = SHADOW_CASCADE_COUNT;

            shadowMapArray->Bind();
            shadowMapArray->SetViewport();

            //glEnable(GL_CULL_FACE);
            //glCullFace(GL_FRONT);  // peter panning

            for (size_t i = 0; i < numLayers; ++i) {

                //int textureLayer = i + (viewportCount * j * numLayers);
                int textureLayer = int(i) + (j * int(numLayers)); // numLayers == SHADOW_CASCADE_COUNT

                shadowMapArray->SetTextureLayer(textureLayer);
                shadowMapArray->ClearDepth();

                const glm::mat4& lightProjectionView = viewportData.csmLightProjectionView[i];

                OpenGL::SetUniformMat4("u_projectionView", lightProjectionView);

                // Geometry
                glBindVertexArray(glMeshBufferAssets.GetVAO());

                OpenGL::SetUniformBool("u_useInstanceData", true);
                MultiDrawIndirect(drawInfoSet.moonLightCascades[j][i]);

                OpenGL::SetUniformBool("u_useInstanceData", false);
                OpenGL::SetUniformMat4("u_modelMatrix", glm::mat4(1.0f));

                // Procedural
                glBindVertexArray(glMeshBufferProcedural.GetVAO());

                //glDisable(GL_CULL_FACE);
                const std::vector<RenderItem>& renderItems = Unloved::RenderDataManager::GetRenderItemsProcedural();
                for (const RenderItem& renderItem : renderItems) {
                    Mesh* mesh = meshBufferProcedural.GetMeshById(renderItem.meshId);
                    if (!mesh) continue;

                    int indexCount = mesh->indexCount;
                    int baseVertex = renderItem.baseVertex;
                    int baseIndex = renderItem.baseIndex;
                    glDrawElementsBaseVertex(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * baseIndex), baseVertex);
                }

                // Weather boards
                //MeshBuffer weatherboardMeshBuffer = LegacyWorld::GetWeatherBoardMeshBuffer();
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
