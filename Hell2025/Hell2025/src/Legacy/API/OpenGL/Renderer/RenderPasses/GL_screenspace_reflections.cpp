#include "../GL_renderer.h"
#include "Hell/Audio.h"
#include "Renderer/Renderer.h"

namespace OpenGLRenderer {
    void ScreenspaceReflectionsPass() {
        if (!Renderer::GetCurrentRendererSettings().screenspaceReflections) 
            return;

        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer* gBuffer = GetFrameBufferOLD("GBuffer");
        OpenGLFrameBuffer* halfSizeFbo = GetFrameBufferOLD("HalfSize");
        OpenGLFrameBuffer* fullSizeFBO = GetFrameBufferOLD("MiscFullSize");
        OpenGLShader* shader = GetShaderOLD("ScreenspaceReflections");

        if (!gBuffer) return;
        if (!shader) return;
        if (!halfSizeFbo) return;
        if (!fullSizeFBO) return;

        // Down sample
        BlitFrameBuffer(gBuffer, halfSizeFbo, "Lighting", "DownsampledFinalLighting", GL_COLOR_BUFFER_BIT, GL_LINEAR);
        glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT);

        // Generate Mipmaps
        glGenerateTextureMipmap(halfSizeFbo->GetColorAttachmentHandleByName("DownsampledFinalLighting"));
        glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);

        shader->Bind();
        BindImageTexture(0, gBuffer->GetColorAttachmentHandleByName("Lighting"), GL_READ_WRITE, GL_RGBA16F);
        BindTextureUnit(1, gBuffer->GetColorAttachmentHandleByName("BaseColorMetallic"));
        BindTextureUnit(2, gBuffer->GetColorAttachmentHandleByName("NormalXYRoughnessMisc"));
        BindTextureUnit(3, gBuffer->GetColorAttachmentHandleByName("RMA"));
        BindTextureUnit(4, gBuffer->GetDepthAttachmentHandle());
        BindTextureUnit(5, halfSizeFbo->GetColorAttachmentHandleByName("DownsampledFinalLighting"));
        BindTextureUnit(6, fullSizeFBO->GetColorAttachmentHandleByName("ViewspaceDepth"));

        glDispatchCompute(gBuffer->GetWidth() / TILE_SIZE, gBuffer->GetHeight() / TILE_SIZE, 1);
    }
}