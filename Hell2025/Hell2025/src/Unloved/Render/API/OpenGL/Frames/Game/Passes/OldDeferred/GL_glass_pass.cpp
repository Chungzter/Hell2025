#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Hell/Render/API/OpenGL/GL_back_end.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Viewport/ViewportManager.h"
#include "World/LegacyWorld.h"

#include "Hell/ResourceManagement/ResourceManager.h"

#include "res/shaders/common/gl_fixed_bindings.glsl"

namespace OpenGLRenderer {

    void GlassPass() {
        ProfilerOpenGLZoneFunction();

        OpenGLRasterizerStateManager::ForceRasterizerState("GlassPass");

        const DrawCommandsSet& drawInfoSet = Unloved::RenderDataManager::GetDrawInfoSet();
        const std::vector<ViewportData>& viewportData = Unloved::RenderDataManager::GetViewportData();

        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("Glass");
        OpenGLShader* compositeShader = OpenGL::ResourceManager::GetShaderPtr("GlassComposite");
        OpenGLFrameBuffer* gBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");
        OpenGLShadowMap* flashLightShadowMapsFBO = OpenGL::ResourceManager::GetShadowMapPtr("FlashlightShadowMaps");

        if (!shader) return;
        if (!compositeShader) return;
        if (!gBuffer) return;
        if (!flashLightShadowMapsFBO) return;

        // TODO: explicitly bind all other ssbos used by this render pass
        OpenGL::BindSSBO(6, "Materials");

        OpenGL::BindShader("Glass");
        OpenGL::SetUniformBool("u_flipNormalMapY", ShouldFlipNormalMapY());

        gBuffer->Bind();
        gBuffer->DrawBuffer("Glass");

        glBindVertexArray(Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetVAO());
        glBindTextureUnit(7, GetTextureHandleByName("Flashlight2"));
        glBindTextureUnit(TEX_IDX_SHADOW_MAP_FLASHLIGHT, flashLightShadowMapsFBO->GetDepthTextureHandle());

        // Forward render each glass render item into each viewport
        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGLRenderer::SetViewport(gBuffer, viewport);
            OpenGL::SetUniformInt("u_viewportIndex", i);

            Unloved::Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(i);

            for (const RenderItem& renderItem : drawInfoSet.glass[i]) {
                OpenGL::SetUniformMat4("u_modelMatrix", renderItem.modelMatrix);

                Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(renderItem.meshId);
                if (!mesh) continue;

                glActiveTexture(GL_TEXTURE4);
                glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(renderItem.baseColorTextureIndex)->GetGLTexture().GetHandle());
                glActiveTexture(GL_TEXTURE5);
                glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(renderItem.normalMapTextureIndex)->GetGLTexture().GetHandle());
                glActiveTexture(GL_TEXTURE6);
                glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(renderItem.rmaTextureIndex)->GetGLTexture().GetHandle());

                glDrawElementsBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), mesh->baseVertex);
            }
        }

        // Composite that render back into the lighting texture
        gBuffer->SetViewport();
        OpenGL::BindShader("GlassComposite");
        glBindImageTexture(0, gBuffer->GetColorAttachmentHandleByName("Lighting"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
        glBindImageTexture(1, gBuffer->GetColorAttachmentHandleByName("Glass"), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);
        OpenGL::DispatchCompute(gBuffer->GetWidth() / 16, gBuffer->GetHeight() / 4, 1);

        glDepthMask(GL_TRUE);
    }
}
