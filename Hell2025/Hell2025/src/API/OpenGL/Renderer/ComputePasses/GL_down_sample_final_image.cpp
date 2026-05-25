#include "API/OpenGL/Renderer/GL_renderer.h"

namespace OpenGLRenderer {

    void DownSampleFinalImage() {
        OpenGLFrameBuffer* gBuffer = GetFrameBufferOLD("GBuffer");
        OpenGLFrameBuffer* finalImageFbo = GetFrameBufferOLD("FinalImage");

        int dstWidth = finalImageFbo->GetWidth();
        int dstHeight = finalImageFbo->GetHeight();

        int groupSizeX = 8;
        int groupSizeY = 8;

        int dispatchGroupCountX = (dstWidth + groupSizeX - 1) / groupSizeX;
        int dispatchGroupCountY = (dstHeight + groupSizeY - 1) / groupSizeY;

        BindShader("DownSample2xBox");
        BindImageTexture(0, gBuffer->GetColorAttachmentHandleByName("Lighting"), GL_READ_ONLY, GL_RGBA16F);
        BindImageTexture(1, finalImageFbo->GetColorAttachmentHandleByName("Color"), GL_WRITE_ONLY, GL_RGBA16F);
        BindTextureUnit(2, gBuffer->GetColorAttachmentHandleByName("Lighting"));

        glDispatchCompute(dispatchGroupCountX, dispatchGroupCountY, 1);

        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }
}