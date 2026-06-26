#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Hell/Render/API/OpenGL/GL_back_end.h"
#include "Unloved/Session/Session.h"
#include "Renderer/RenderDataManager.h"
#include "Renderer/Renderer.h"
#include "Viewport/ViewportManager.h"
#include "World/LegacyWorld.h"

#include "Hell/ResourceManagement/ResourceManager.h"

namespace OpenGLRenderer {

    void StainedGlassPass() {
        ProfilerOpenGLZoneFunction();

        OpenGLRasterizerStateManager::ForceRasterizerState("GlassPass");

        const std::vector<ViewportData>& viewportData = RenderDataManager::GetViewportData();

        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("StainedGlass");
        OpenGLShader* compositeShader = OpenGL::ResourceManager::GetShaderPtr("GlassComposite");
        OpenGLFrameBuffer* miscFullSizeFrameBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("MiscFullSize");
        OpenGLShadowMap* flashLightShadowMapsFBO = OpenGL::ResourceManager::GetShadowMapPtr("FlashlightShadowMaps");

        std::string gBufferName = (Renderer::GetRendererMode() == RendererMode::RE_STYLE) ? "GBufferRE" : "GBuffer";
        OpenGLFrameBuffer& gBuffer = OpenGL::ResourceManager::GetFrameBuffer(gBufferName);

        if (!shader) return;
        if (!compositeShader) return;
        if (!flashLightShadowMapsFBO) return;

        OpenGL::BindShader("StainedGlass");
        OpenGL::SetUniformBool("u_flipNormalMapY", ShouldFlipNormalMapY());

        gBuffer.Bind();
        gBuffer.DrawBuffer("Lighting");

        glBindVertexArray(Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetVAO());
        glBindTextureUnit(0, gBuffer.GetDepthAttachmentHandle());
        glBindTextureUnit(7, GetTextureHandleByName("Flashlight2"));
        glBindTextureUnit(8, flashLightShadowMapsFBO->GetDepthTextureHandle());
        glBindTextureUnit(9, miscFullSizeFrameBuffer->GetColorAttachmentHandleByName("GaussianFinalLightingIntermediate"));

        // Forward render each glass render item into each viewport
        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(i);
            if (!player) continue;

            OpenGLRenderer::SetViewport(&gBuffer, viewport);
            OpenGL::SetUniformInt("u_viewportIndex", i);

            // Sort by distance to camera
            static std::vector<RenderItem> sortedRenderItems;
            sortedRenderItems = RenderDataManager::GetRenderItemsStainedGlass();

            std::sort(sortedRenderItems.begin(), sortedRenderItems.end(), [player](RenderItem& a, RenderItem& b) {
                float distA = glm::distance(player->GetCameraPosition(), glm::vec3(a.modelMatrix[3]));
                float distB = glm::distance(player->GetCameraPosition(), glm::vec3(b.modelMatrix[3]));
                return distA > distB;
            });

            for (const RenderItem& renderItem : sortedRenderItems) {
                glm::vec3 tintColor = { renderItem.tintColorR , renderItem.tintColorG , renderItem.tintColorB };

                OpenGL::SetUniformMat4("u_modelMatrix", renderItem.modelMatrix);
                OpenGL::SetUniformVec3("u_tintColor", tintColor);
                OpenGL::SetUniformInt("u_meshIndex", renderItem.meshId);

                Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(renderItem.meshId);
                if (!mesh) continue;

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(renderItem.baseColorTextureIndex)->GetGLTexture().GetHandle());
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(renderItem.normalMapTextureIndex)->GetGLTexture().GetHandle());
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(renderItem.rmaTextureIndex)->GetGLTexture().GetHandle());

                glDrawElementsBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), mesh->baseVertex);
            }
        }

        // Composite that render back into the lighting texture
        //glBindImageTexture(0, gBuffer.GetColorAttachmentHandleByName("Lighting"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
        //glBindImageTexture(1, gBuffer.GetColorAttachmentHandleByName("Glass"), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);
        //OpenGL::DispatchCompute(gBuffer.GetWidth() / 16, gBuffer.GetHeight() / 4, 1);
        //
        //glDepthMask(GL_TRUE);
    }
}
