#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Hell/Render/API/OpenGL/GL_back_end.h"
#include "Hell/Backend/BackEnd.h"
#include "Unloved/Editor/Editor.h"
#include "Unloved/Objects/Props/GameObject.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Viewport/ViewportManager.h"
#include "Unloved/World/World.h"

#include "Hell/ResourceManagement/ResourceManager.h"

#include "../../../../../../res/shaders/common/gl_fixed_bindings.glsl"


namespace OpenGLRenderer {
    using namespace Unloved;


    void FurPass() {
        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer* gBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");
        OpenGLFrameBuffer* hairFrameBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("Hair");
        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("Fur");
        OpenGLShader* compositeShader = OpenGL::ResourceManager::GetShaderPtr("FurComposite");
        OpenGLShadowCubeMapArray* hiResShadowMaps = OpenGL::ResourceManager::GetShadowCubeMapArrayPtr("HiRes");
        OpenGLShadowCubeMapArray* lowResShadowMaps = OpenGL::ResourceManager::GetShadowCubeMapArrayPtr("LowRes");
        OpenGLShadowMap* flashLightShadowMapsFBO = OpenGL::ResourceManager::GetShadowMapPtr("FlashlightShadowMaps");

        if (!gBuffer) return;
        if (!hairFrameBuffer) return;
        if (!shader) return;
        if (!compositeShader) return;
        if (!hiResShadowMaps) return;
        if (!lowResShadowMaps) return;
        if (!flashLightShadowMapsFBO) return;

        const std::vector<ViewportData>& viewportData = Unloved::RenderDataManager::GetViewportData();
        Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("AssetGeometry");

        glBindVertexArray(meshBuffer.GetVAO());

        gBuffer->Bind();
        gBuffer->DrawBuffers({ "Lighting" });

        OpenGL::BindShader("Fur");

        OpenGLRasterizerStateManager::ForceRasterizerState("GeometryPass_Blended");

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

        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_2D, GetTextureHandleByName("BlueNoise"));
        glBindTextureUnit(TEX_IDX_SHADOW_MAP_HI_RES, hiResShadowMaps->GetDepthTexture());
        glBindTextureUnit(TEX_IDX_SHADOW_MAP_LOW_RES, lowResShadowMaps->GetDepthTexture());
        glBindTextureUnit(8, GetTextureHandleByName("Flashlight2"));
        glBindTextureUnit(TEX_IDX_SHADOW_MAP_FLASHLIGHT, flashLightShadowMapsFBO->GetDepthTextureHandle());

        // Non skinned models
        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (viewport->IsVisible()) {
                OpenGLRenderer::SetViewport(gBuffer, viewport);

                auto& gameObjects = Unloved::World::GetGameObjects();
                if (gameObjects.size() == 0) {
                    continue;
                }

                GameObject& bunny = gameObjects[0];

                for (const RenderItem& renderItem : bunny.GetRenderItems()) {

                    uint32_t meshId = renderItem.meshId;
                    glm::mat4 modelMatrix = renderItem.modelMatrix;
                    glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(modelMatrix)));

                    Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(meshId);

                    OpenGL::SetUniformMat4("u_model", modelMatrix);
                    OpenGL::SetUniformMat3("u_normalMatrix", normalMatrix);
                    OpenGL::SetUniformInt("u_viewportIndex", i);
                    OpenGL::SetUniformInt("u_hairLayerCount", hairLayerCount);

                    glActiveTexture(GL_TEXTURE4);
                    glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(renderItem.baseColorTextureIndex)->GetGLTexture().GetHandle());
                    glActiveTexture(GL_TEXTURE5);
                    glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(renderItem.normalMapTextureIndex)->GetGLTexture().GetHandle());
                    glActiveTexture(GL_TEXTURE6);
                    glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(renderItem.rmaTextureIndex)->GetGLTexture().GetHandle());
                    glActiveTexture(GL_TEXTURE9);
                    glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByName("Kangaroo_FurMask")->GetGLTexture().GetHandle());

                    glDrawElementsInstancedBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (GLvoid*)(mesh->baseIndex * sizeof(GLuint)), hairLayerCount, mesh->baseVertex);

                }
            }
        }

        //glDepthMask(GL_TRUE);

        // Skinned models
        //glBindVertexArray(OpenGL::BackEnd::GetSkinnedVertexDataVAO());
        //glBindBuffer(GL_ARRAY_BUFFER, OpenGL::BackEnd::GetSkinnedVertexDataVBO());
        //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, OpenGL::BackEnd::GetWeightedVertexDataEBO());
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
        //        std::vector<AnimatedGameObject*> animatedgameObjects = Unloved::RenderDataManager::GetAnimatedGameObjectToSkin();
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


        const DrawCommandsSet& drawInfoSet = Unloved::RenderDataManager::GetDrawInfoSet();

        OpenGLFrameBuffer* gBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");
        OpenGLFrameBuffer* hairFrameBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("Hair");
        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("Fur");
        OpenGLShader* compositeShader = OpenGL::ResourceManager::GetShaderPtr("FurComposite");
        OpenGLShadowCubeMapArray* hiResShadowMaps = OpenGL::ResourceManager::GetShadowCubeMapArrayPtr("HiRes");
        OpenGLShadowMap* flashLightShadowMapsFBO = OpenGL::ResourceManager::GetShadowMapPtr("FlashlightShadowMaps");

        if (!gBuffer) return;
        if (!hairFrameBuffer) return;
        if (!shader) return;
        if (!compositeShader) return;
        if (!hiResShadowMaps) return;
        if (!flashLightShadowMapsFBO) return;

        const std::vector<ViewportData>& viewportData = Unloved::RenderDataManager::GetViewportData();

        glBindVertexArray(Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetVAO());

        hairFrameBuffer->Bind();
        hairFrameBuffer->ClearAttachment("Lighting", 0, 0, 0, 0);
        hairFrameBuffer->DrawBuffers({ "Lighting" });

        OpenGL::BindShader("Fur");

        OpenGLRasterizerStateManager::SetRasterizerState("GeometryPass_Default");

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
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (viewport->IsVisible()) {
                OpenGLRenderer::SetViewport(hairFrameBuffer, viewport);

                GameObject& bunny = Unloved::World::GetGameObjects()[0];

                for (const RenderItem& renderItem : bunny.GetRenderItems()) {

                    uint32_t meshId = renderItem.meshId;
                    glm::mat4 modelMatrix = renderItem.modelMatrix;

                    Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(meshId);

                    //if (mesh->name != "fur") continue;

                    OpenGL::SetUniformMat4("u_model", modelMatrix);
                    OpenGL::SetUniformInt("u_viewportIndex", i);
                    OpenGL::SetUniformInt("u_hairLayerCount", hairLayerCount);

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


        glBindVertexArray(OpenGL::BackEnd::GetSkinnedVertexDataVAO());
        glBindBuffer(GL_ARRAY_BUFFER, OpenGL::BackEnd::GetSkinnedVertexDataVBO());
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, OpenGL::BackEnd::GetWeightedVertexDataEBO());

        glBindVertexArray(OpenGL::BackEnd::GetSkinnedVertexDataVAO());
        glBindBuffer(GL_ARRAY_BUFFER, OpenGL::BackEnd::GetSkinnedVertexDataVBO());
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, OpenGL::BackEnd::GetWeightedVertexDataEBO());

        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (viewport->IsVisible()) {
                OpenGLRenderer::SetViewport(hairFrameBuffer, viewport);

                std::vector<AnimatedGameObject*> animatedgameObjects = Unloved::RenderDataManager::GetAnimatedGameObjectToSkin();

                for (AnimatedGameObject* animatedGameObject : animatedgameObjects) {

                    std::vector<RenderItem>& renderItems = animatedGameObject->GetRenderItems();

                    for (const RenderItem& renderItem : renderItems) {

                        if (renderItem.furLength == 0) continue;

                        uint32_t meshId = renderItem.meshId;
                        glm::mat4 modelMatrix = renderItem.modelMatrix;

                        Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(meshId);

                        OpenGL::SetUniformMat4("u_model", modelMatrix);
                        OpenGL::SetUniformInt("u_viewportIndex", i);
                        OpenGL::SetUniformInt("u_hairLayerCount", hairLayerCount);

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
        OpenGL::BindShader("FurComposite");
        glBindImageTexture(0, gBuffer->GetColorAttachmentHandleByName("Lighting"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
        glBindTextureUnit(1, hairFrameBuffer->GetColorAttachmentHandleByName("Lighting"));
        OpenGL::DispatchCompute((gBuffer->GetWidth() + 7) / 8, (gBuffer->GetHeight() + 7) / 8, 1);
    }
*/
}
