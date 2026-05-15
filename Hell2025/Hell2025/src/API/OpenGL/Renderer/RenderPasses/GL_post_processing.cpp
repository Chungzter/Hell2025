#include "../GL_renderer.h"
#include "AssetManagement/AssetManager.h"
#include "Renderer/Renderer.h"
#include "World/World.h"

namespace OpenGLRenderer {

    void FXAA() {
        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer& gBuffer = GetFrameBuffer("GBuffer");
        OpenGLFrameBuffer& postProcessingFbo = GetFrameBuffer("PostProcessing");
        OpenGLShader& shader = GetShader("FXAA");

        shader.Bind();

        BlitFrameBuffer(&gBuffer, &postProcessingFbo, "FinalLighting", "Scratch", GL_COLOR_BUFFER_BIT, GL_NEAREST);

        BindImageTexture(0, gBuffer.GetColorAttachmentHandleByName("FinalLighting"), GL_READ_WRITE, GL_RGBA16F);
        BindTextureUnit(1, postProcessingFbo.GetColorAttachmentHandleByName("Scratch"));

        glDispatchCompute(gBuffer.GetWidth() / 8, gBuffer.GetHeight() / 8, 1);
    }

    void PostProcessingPass() {
        ProfilerOpenGLZoneFunction();

        RendererSettings& rendererSettings = Renderer::GetCurrentRendererSettings();

        // Only post process the following modes
        if (rendererSettings.rendererOverrideState == RendererOverrideState::NONE || // This means the final lit image
            rendererSettings.rendererOverrideState == RendererOverrideState::CAMERA_NDOTL ||
            rendererSettings.rendererOverrideState == RendererOverrideState::INDIRECT_DIFFUSE) {

			OpenGLFrameBuffer* gBuffer = GetFrameBufferOLD("GBuffer");
			OpenGLFrameBuffer* miscFullSizeFBO = GetFrameBufferOLD("MiscFullSize");
			OpenGLFrameBuffer* msaaFbo = GetFrameBufferOLD("MSAA");
            OpenGLShader* shader = GetShaderOLD("PostProcessing");

			if (!gBuffer) return;
			if (!miscFullSizeFBO) return;
			if (!msaaFbo) return;
            if (!shader) return;

			shader->Bind();
			shader->SetBool("u_msaaRenderer", false);
            
            glBindImageTexture(0, gBuffer->GetColorAttachmentHandleByName("FinalLighting"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
            glBindImageTexture(1, gBuffer->GetColorAttachmentHandleByName("Normal"), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);
            glBindImageTexture(2, miscFullSizeFBO->GetColorAttachmentHandleByName("ViewportIndex"), 0, GL_FALSE, 0, GL_READ_ONLY, GL_R8UI);

			BindTextureUnit(3, gBuffer->GetColorAttachmentHandleByName("WorldPosition"));
			BindTextureUnit(4, gBuffer->GetDepthAttachmentHandle());
			BindTextureUnit(5, msaaFbo->GetDepthAttachmentHandle());

            glDispatchCompute(gBuffer->GetWidth() / 8, gBuffer->GetHeight() / 8, 1);

            FXAA();
        }
    }
}