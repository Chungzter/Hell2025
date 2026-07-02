#include "../GL_renderer.h"
#include "Unloved/Render/Renderer.h"
#include "World/LegacyWorld.h"

namespace OpenGLRenderer {

    // 1. TAA
    // 2. Tone mapping
    // 3. FXAA

    void ToneMapping() {
        //ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer& gBuffer = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
        OpenGLFrameBuffer& scratchFbo = OpenGL::ResourceManager::GetFrameBuffer("Scratch");

        OpenGL::BindShader("PostProcessing");

        OpenGL::BindImageTexture(0, scratchFbo.GetColorAttachmentHandleByName("RGBA16F"), GL_WRITE_ONLY, GL_RGBA16F);
        OpenGL::BindImageTexture(1, gBuffer.GetColorAttachmentHandleByName("Lighting"), GL_READ_ONLY, GL_RGBA16F);

        OpenGL::DispatchCompute(gBuffer.GetWidth() / 8, gBuffer.GetHeight() / 8, 1);
    }

    void FXAA() {
        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer& gBuffer = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
        OpenGLFrameBuffer& scratchFbo = OpenGL::ResourceManager::GetFrameBuffer("Scratch");

        OpenGL::BindShader("FXAA");

        OpenGL::BindImageTexture(0, gBuffer.GetColorAttachmentHandleByName("Lighting"), GL_WRITE_ONLY, GL_RGBA16F);
        OpenGL::BindTextureUnit(1, scratchFbo.GetColorAttachmentHandleByName("RGBA16F"));

        OpenGL::DispatchCompute(gBuffer.GetWidth() / 8, gBuffer.GetHeight() / 8, 1);
    }

    void PostProcessingPass() {
        RendererSettings& rendererSettings = Unloved::Renderer::GetCurrentRendererSettings();
        RendererOverrideState state = rendererSettings.rendererOverrideState;

        if (state == RendererOverrideState::NONE ||
            state == RendererOverrideState::CAMERA_NDOTL ||
            state == RendererOverrideState::INDIRECT_DIFFUSE) {
            ToneMapping();
            FXAA();
        }
    }
}