#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Renderer/RenderDataManager.h"
#include "Unloved/Viewport/ViewportManager.h"
#include "World/LegacyWorld.h"


#include "Hell/ResourceManagement/ResourceManager.h"

namespace OpenGLRenderer {
    void BloodDecalTileCulling();
    void BloodDecalDraw();
    void BloodDecalComposite();

    void BloodDecalsPass() {
        BloodDecalTileCulling();
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        BloodDecalDraw();
        glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

        BloodDecalComposite();
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
    }

    void BloodDecalDraw() {
        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer* miscFullSizeFBO = OpenGL::ResourceManager::GetFrameBufferPtr("MiscFullSize");
        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("BloodDecalsDraw");
        OpenGLFrameBuffer* gBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");

        if (!miscFullSizeFBO) return;
        if (!shader) return;
        if (!gBuffer) return;

        miscFullSizeFBO->Bind();
        miscFullSizeFBO->SetViewport();
        miscFullSizeFBO->DrawBuffers({ "BloodScreenSpaceDecalMask" });

        OpenGL::BindShader("BloodDecalsDraw");
        OpenGL::SetUniformInt("u_tileXCount", GetTileCountX());
        OpenGL::SetUniformInt("u_tileYCount", GetTileCountY());

        OpenGLRasterizerState state;
        state.depthTestEnabled = false;
        state.depthMask = false;
        state.blendEnable = false;
        state.cullfaceEnable = false;
        state.colorMask = true;
        OpenGLRasterizerStateManager::ForceRasterizerState(state);

        OpenGL::BindSSBO(7, "TileBloodDecals");
        OpenGL::BindSSBO(8, "BloodDecalInstances");
        OpenGL::BindSSBO(9, "BloodDecalIndices");

        glBindTextureUnit(0, gBuffer->GetColorAttachmentHandleByName("RMA"));
        glBindTextureUnit(1, gBuffer->GetColorAttachmentHandleByName("NormalXYRoughnessMisc"));
        glBindTextureUnit(2, gBuffer->GetDepthAttachmentHandle());

        if (Hell::BackEnd::RenderDocFound()) {
            glBindTextureUnit(3, Hell::ResourceManager::GetTextureByName("BloodDecal4")->GetGLTexture().GetHandle());
            glBindTextureUnit(4, Hell::ResourceManager::GetTextureByName("BloodDecal6")->GetGLTexture().GetHandle());
            glBindTextureUnit(5, Hell::ResourceManager::GetTextureByName("BloodDecal7")->GetGLTexture().GetHandle());
            glBindTextureUnit(6, Hell::ResourceManager::GetTextureByName("BloodDecal9")->GetGLTexture().GetHandle());
        }

        // Draw full screen triangle
        BindEmptyVAO();
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

    void BloodDecalComposite() {
        ProfilerOpenGLZoneFunction();

        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("BloodDecalsComposite");
        OpenGLFrameBuffer* miscFullSizeFBO = OpenGL::ResourceManager::GetFrameBufferPtr("MiscFullSize");
        OpenGLFrameBuffer* gBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");

        if (!miscFullSizeFBO) return;
        if (!shader) return;
        if (!gBuffer) return;

        OpenGL::BindShader("BloodDecalsComposite");

        glBindImageTexture(0, gBuffer->GetColorAttachmentHandleByName("BaseColorMetallic"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
        glBindImageTexture(1, gBuffer->GetColorAttachmentHandleByName("NormalXYRoughnessMisc"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGB10_A2);
        glBindImageTexture(2, gBuffer->GetColorAttachmentHandleByName("VelocityXYOcclusionSubSurface"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
        glBindTextureUnit(3, miscFullSizeFBO->GetColorAttachmentHandleByName("BloodScreenSpaceDecalMask"));
        OpenGL::DispatchCompute((gBuffer->GetWidth() + 7) / 8, (gBuffer->GetHeight() + 7) / 8, 1);
    }
}
