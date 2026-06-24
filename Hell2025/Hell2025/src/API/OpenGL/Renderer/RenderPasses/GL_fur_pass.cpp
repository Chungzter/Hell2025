#include "API/OpenGL/Renderer/GL_renderer.h" 
#include "API/OpenGL/GL_backend.h"
#include "BackEnd/Backend.h"
#include "Editor/Editor.h"
#include "Input/Input.h"
#include "Renderer/RenderDataManager.h"
#include "Viewport/ViewportManager.h"
#include "World/World.h"

#include "Hell/ResourceManagement/ResourceManager.h"

namespace OpenGLRenderer {

    void FurPass() {
        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer* gBuffer = GetFrameBufferOLD("GBuffer"); 
        OpenGLFrameBuffer* hairFrameBuffer = GetFrameBufferOLD("Hair");
        OpenGLShader* shader = GetShaderOLD("Fur");
        OpenGLShader* compositeShader = GetShaderOLD("FurComposite");
        OpenGLShadowCubeMapArray* hiResShadowMaps = GetShadowCubeMapArrayOLD("HiRes");
        OpenGLShadowMap* flashLightShadowMapsFBO = GetShadowMapOLD("FlashlightShadowMaps");

        if (!gBuffer) return;
        if (!hairFrameBuffer) return;
        if (!shader) return;
        if (!compositeShader) return;
        if (!hiResShadowMaps) return;
        if (!flashLightShadowMapsFBO) return;

        const std::vector<ViewportData>& viewportData = RenderDataManager::GetViewportData();
        Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("AssetGeometry");

        glBindVertexArray(meshBuffer.GetVAO());

        gBuffer->Bind();
        gBuffer->DrawBuffers({ "Lighting" });

        shader->Bind();

        ForceRasterizerState("GeometryPass_Blended");

        //static bool skip = false;
        //if (Input::KeyPressed(HELL_KEY_X)) {
        //    skip = !skip;
        //}
        //if (skip) {
        //    return;
        //}

        glEnable(GL_BLEND);
        int hairLayerCount = 15;

        glDepthMask(GL_FALSE);
        glDisable(GL_CULL_FACE);
        //glDisable(GL_BLEND);

        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, GetTextureHandleByName("BlueNoise"));
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, hiResShadowMaps->GetDepthTexture());
        glBindTextureUnit(5, GetTextureHandleByName("Flashlight2"));
        glBindTextureUnit(6, flashLightShadowMapsFBO->GetDepthTextureHandle());

        // Non skinned models
        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (viewport->IsVisible()) {
                OpenGLRenderer::SetViewport(gBuffer, viewport);

                if (World::GetGameObjects().size() == 0) {
                    continue;
                }

                GameObject& bunny = World::GetGameObjects()[0];

                for (const RenderItem& renderItem : bunny.GetRenderItems()) {

                    uint32_t meshId = renderItem.meshId;
                    glm::mat4 modelMatrix = renderItem.modelMatrix;
                    glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(modelMatrix)));

                    Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(meshId);

                    shader->SetMat4("u_model", modelMatrix);
                    shader->SetMat3("u_normalMatrix", normalMatrix);
                    shader->SetInt("u_viewportIndex", i);
                    shader->SetInt("u_hairLayerCount", hairLayerCount);

                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(renderItem.baseColorTextureIndex)->GetGLTexture().GetHandle());
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(renderItem.normalMapTextureIndex)->GetGLTexture().GetHandle());
                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(renderItem.rmaTextureIndex)->GetGLTexture().GetHandle());
                    glActiveTexture(GL_TEXTURE7);
                    glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByName("Kangaroo_FurMask")->GetGLTexture().GetHandle());

                    glDrawElementsInstancedBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (GLvoid*)(mesh->baseIndex * sizeof(GLuint)), hairLayerCount, mesh->baseVertex);
                    
                }
            }
        }

        //glDepthMask(GL_TRUE);

        // Skinned models
        //glBindVertexArray(OpenGLBackEnd::GetSkinnedVertexDataVAO());
        //glBindBuffer(GL_ARRAY_BUFFER, OpenGLBackEnd::GetSkinnedVertexDataVBO());
        //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, OpenGLBackEnd::GetWeightedVertexDataEBO());
        //
        //glActiveTexture(GL_TEXTURE3);
        //glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByName("WaterNormals")->GetGLTexture().GetHandle());
        //glActiveTexture(GL_TEXTURE4);
        //glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, hiResShadowMaps->GetDepthTexture());
        //glBindTextureUnit(5, Hell::ResourceManager::GetTextureByName("Flashlight2")->GetGLTexture().GetHandle());
        //glBindTextureUnit(6, flashLightShadowMapsFBO->GetDepthTextureHandle());
        //
        //if (false)
        //for (int i = 0; i < 4; i++) {
        //    Viewport* viewport = ViewportManager::GetViewportByIndex(i);
        //    if (viewport->IsVisible()) {
        //        OpenGLRenderer::SetViewport(gBuffer, viewport);
        //
        //        std::vector<AnimatedGameObject*> animatedgameObjects = RenderDataManager::GetAnimatedGameObjectToSkin();
        //
        //        for (AnimatedGameObject* animatedGameObject : animatedgameObjects) {
        //
        //            std::vector<RenderItem>& renderItems = animatedGameObject->GetRenderItems();
        //
        //            for (const RenderItem& renderItem : renderItems) {
        //
        //                if (renderItem.furLength == 0) continue;
        //
        //                uint32_t meshId = renderItem.meshId;
        //                glm::mat4 modelMatrix = renderItem.modelMatrix;
        //                glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(modelMatrix)));
        //
        //                Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(meshId);
        //
        //                shader->SetMat4("u_model", modelMatrix);
        //                shader->SetMat3("u_normalMatrix", normalMatrix);
        //                shader->SetInt("u_viewportIndex", i);
        //                shader->SetInt("u_hairLayerCount", hairLayerCount);
        //
        //                glActiveTexture(GL_TEXTURE0);
        //                glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(renderItem.baseColorTextureIndex)->GetGLTexture().GetHandle());
        //                glActiveTexture(GL_TEXTURE1);
        //                glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(renderItem.normalMapTextureIndex)->GetGLTexture().GetHandle());
        //                glActiveTexture(GL_TEXTURE2);
        //                glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(renderItem.rmaTextureIndex)->GetGLTexture().GetHandle());
        //
        //                glDrawElementsInstancedBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (GLvoid*)(mesh->baseIndex * sizeof(GLuint)), hairLayerCount, renderItem.baseSkinnedVertex);
        //            }
        //        }
        //    }
        //}

        glDisable(GL_BLEND);
    }


/*
    void FurPass() {


        const DrawCommandsSet& drawInfoSet = RenderDataManager::GetDrawInfoSet();

        OpenGLFrameBuffer* gBuffer = GetFrameBuffer("GBuffer");
        OpenGLFrameBuffer* hairFrameBuffer = GetFrameBuffer("Hair");
        OpenGLShader* shader = GetShader("Fur");
        OpenGLShader* compositeShader = GetShader("FurComposite");
        OpenGLShadowCubeMapArray* hiResShadowMaps = GetShadowMapArray("HiRes");
        OpenGLShadowMap* flashLightShadowMapsFBO = GetShadowMap("FlashlightShadowMaps");

        if (!gBuffer) return;
        if (!hairFrameBuffer) return;
        if (!shader) return;
        if (!compositeShader) return;
        if (!hiResShadowMaps) return;
        if (!flashLightShadowMapsFBO) return;

        const std::vector<ViewportData>& viewportData = RenderDataManager::GetViewportData();

        glBindVertexArray(Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetVAO());

        hairFrameBuffer->Bind();
        hairFrameBuffer->ClearAttachment("Lighting", 0, 0, 0, 0);
        hairFrameBuffer->DrawBuffers({ "Lighting" });

        shader->Bind();

        SetRasterizerState("GeometryPass_Default");

        static bool skip = false;

        if (Input::KeyPressed(HELL_KEY_X)) {
            skip = !skip;
        }

        if (skip) {
            return;
        }

        glEnable(GL_BLEND);
        int hairLayerCount = 20;

        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (viewport->IsVisible()) {
                OpenGLRenderer::SetViewport(hairFrameBuffer, viewport);

                GameObject& bunny = World::GetGameObjects()[0];

                for (const RenderItem& renderItem : bunny.GetRenderItems()) {

                    uint32_t meshId = renderItem.meshId;
                    glm::mat4 modelMatrix = renderItem.modelMatrix;

                    Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(meshId);

                    //if (mesh->name != "fur") continue;

                    shader->SetMat4("u_model", modelMatrix);
                    shader->SetInt("u_viewportIndex", i);
                    shader->SetInt("u_hairLayerCount", hairLayerCount);

                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(renderItem.baseColorTextureIndex)->GetGLTexture().GetHandle());
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(renderItem.normalMapTextureIndex)->GetGLTexture().GetHandle());
                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(renderItem.rmaTextureIndex)->GetGLTexture().GetHandle());
                    glActiveTexture(GL_TEXTURE3);
                    glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByName("BlueNoise")->GetGLTexture().GetHandle());
                    glActiveTexture(GL_TEXTURE4);
                    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, hiResShadowMaps->GetDepthTexture());
                    glBindTextureUnit(5, Hell::ResourceManager::GetTextureByName("Flashlight2")->GetGLTexture().GetHandle());
                    glBindTextureUnit(6, flashLightShadowMapsFBO->GetDepthTextureHandle());

                    glDrawElementsInstancedBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (GLvoid*)(mesh->baseIndex * sizeof(GLuint)), hairLayerCount, mesh->baseVertex);
                }
            }
        }



        glBindVertexArray(OpenGLBackEnd::GetSkinnedVertexDataVAO());
        glBindBuffer(GL_ARRAY_BUFFER, OpenGLBackEnd::GetSkinnedVertexDataVBO());
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, OpenGLBackEnd::GetWeightedVertexDataEBO());

        glBindVertexArray(OpenGLBackEnd::GetSkinnedVertexDataVAO());
        glBindBuffer(GL_ARRAY_BUFFER, OpenGLBackEnd::GetSkinnedVertexDataVBO());
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, OpenGLBackEnd::GetWeightedVertexDataEBO());

        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (viewport->IsVisible()) {
                OpenGLRenderer::SetViewport(hairFrameBuffer, viewport);

                std::vector<AnimatedGameObject*> animatedgameObjects = RenderDataManager::GetAnimatedGameObjectToSkin();

                for (AnimatedGameObject* animatedGameObject : animatedgameObjects) {

                    std::vector<RenderItem>& renderItems = animatedGameObject->GetRenderItems();

                    for (const RenderItem& renderItem : renderItems) {

                        if (renderItem.furLength == 0) continue;

                        uint32_t meshId = renderItem.meshId;
                        glm::mat4 modelMatrix = renderItem.modelMatrix;

                        Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(meshId);

                        shader->SetMat4("u_model", modelMatrix);
                        shader->SetInt("u_viewportIndex", i);
                        shader->SetInt("u_hairLayerCount", hairLayerCount);

                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(renderItem.baseColorTextureIndex)->GetGLTexture().GetHandle());
                        glActiveTexture(GL_TEXTURE1);
                        glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(renderItem.normalMapTextureIndex)->GetGLTexture().GetHandle());
                        glActiveTexture(GL_TEXTURE2);
                        glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(renderItem.rmaTextureIndex)->GetGLTexture().GetHandle());
                        glActiveTexture(GL_TEXTURE3);
                        glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByName("BlueNoise")->GetGLTexture().GetHandle());
                        glActiveTexture(GL_TEXTURE4);
                        glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, hiResShadowMaps->GetDepthTexture());
                        glBindTextureUnit(5, Hell::ResourceManager::GetTextureByName("Flashlight2")->GetGLTexture().GetHandle());
                        glBindTextureUnit(6, flashLightShadowMapsFBO->GetDepthTextureHandle());

                        glDrawElementsInstancedBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (GLvoid*)(mesh->baseIndex * sizeof(GLuint)), hairLayerCount, renderItem.baseSkinnedVertex);
                    }
                }
            }
        }

        // Add fur to final image
        compositeShader->Bind();
        glBindImageTexture(0, gBuffer->GetColorAttachmentHandleByName("Lighting"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
        glBindTextureUnit(1, hairFrameBuffer->GetColorAttachmentHandleByName("Lighting"));
        glDispatchCompute((gBuffer->GetWidth() + 7) / 8, (gBuffer->GetHeight() + 7) / 8, 1);
    }
*/
}
