#include "API/OpenGL/Renderer/GL_Renderer.h"
#include "Renderer/RenderDataManager.h"
#include "Viewport/ViewportManager.h"
#include "World/World.h"

#include "Input/Input.h"

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

        OpenGLFrameBuffer* miscFullSizeFBO = GetFrameBufferOLD("MiscFullSize");
        OpenGLShader* shader = GetShaderOLD("BloodDecalsDraw");
        OpenGLFrameBuffer* gBuffer = GetFrameBufferOLD("GBuffer");

        if (!miscFullSizeFBO) return;
        if (!shader) return;
        if (!gBuffer) return;

        miscFullSizeFBO->Bind();
        miscFullSizeFBO->SetViewport();
        miscFullSizeFBO->DrawBuffers({ "ScreenSpaceBloodDecalMask" });

        shader->Bind();
        shader->SetInt("u_tileXCount", GetTileCountX());
        shader->SetInt("u_tileYCount", GetTileCountY());

        BindSSBO(7, "TileBloodDecals");
        BindSSBO(8, "BloodDecalInstances");
        BindSSBO(9, "BloodDecalIndices");

        glBindTextureUnit(0, gBuffer->GetColorAttachmentHandleByName("RMA"));
        glBindTextureUnit(1, gBuffer->GetColorAttachmentHandleByName("NormalXYRoughnessMisc"));
        glBindTextureUnit(2, gBuffer->GetDepthAttachmentHandle());

        if (BackEnd::RenderDocFound()) {
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

        OpenGLShader* shader = GetShaderOLD("BloodDecalsComposite");
        OpenGLFrameBuffer* miscFullSizeFBO = GetFrameBufferOLD("MiscFullSize");
        OpenGLFrameBuffer* gBuffer = GetFrameBufferOLD("GBuffer");

        if (!miscFullSizeFBO) return;
        if (!shader) return;
        if (!gBuffer) return;

        glMemoryBarrier(GL_ALL_BARRIER_BITS);

        shader->Bind();

        glBindImageTexture(0, gBuffer->GetColorAttachmentHandleByName("BaseColorMetallic"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
        glBindImageTexture(1, gBuffer->GetColorAttachmentHandleByName("RMA"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
        glBindTextureUnit(2, miscFullSizeFBO->GetColorAttachmentHandleByName("ScreenSpaceBloodDecalMask"));
        glDispatchCompute((gBuffer->GetWidth() + 7) / 8, (gBuffer->GetHeight() + 7) / 8, 1);
    }
}