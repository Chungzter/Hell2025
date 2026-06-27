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
        BloodDecalDraw();
        BloodDecalComposite();
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

        glMemoryBarrier(GL_ALL_BARRIER_BITS);

        OpenGL::BindShader("BloodDecalsComposite");

        glBindImageTexture(0, gBuffer->GetColorAttachmentHandleByName("BaseColorMetallic"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
        glBindImageTexture(1, gBuffer->GetColorAttachmentHandleByName("RMA"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
        glBindTextureUnit(2, miscFullSizeFBO->GetColorAttachmentHandleByName("BloodScreenSpaceDecalMask"));
        OpenGL::DispatchCompute((gBuffer->GetWidth() + 7) / 8, (gBuffer->GetHeight() + 7) / 8, 1);
    }
}