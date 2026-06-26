#include "../GL_renderer.h"
#include "Renderer/Renderer.h"
#include "World/LegacyWorld.h"

namespace OpenGLRenderer {

    // 1. TAA
    // 2. Tone mapping
    // 3. FXAA

    void ToneMappingPassRE() {
        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer& gBuffer = OpenGL::ResourceManager::GetFrameBuffer("GBufferRE");
        OpenGLFrameBuffer& scratchFbo = OpenGL::ResourceManager::GetFrameBuffer("Scratch");

        OpenGL::BindShader("PostProcessing");

        OpenGL::BindImageTexture(0, scratchFbo.GetColorAttachmentHandleByName("RGBA16F"), GL_WRITE_ONLY, GL_RGBA16F);
        OpenGL::BindImageTexture(1, gBuffer.GetColorAttachmentHandleByName("Lighting"), GL_READ_ONLY, GL_RGBA16F);

        OpenGL::DispatchCompute(gBuffer.GetWidth() / 8, gBuffer.GetHeight() / 8, 1);
    }

    void FXAAPassRE() {
        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer& gBuffer = OpenGL::ResourceManager::GetFrameBuffer("GBufferRE");
        OpenGLFrameBuffer& scratchFbo = OpenGL::ResourceManager::GetFrameBuffer("Scratch");

        OpenGL::BindShader("FXAA");

        OpenGL::BindImageTexture(0, gBuffer.GetColorAttachmentHandleByName("Lighting"), GL_WRITE_ONLY, GL_RGBA16F);
        OpenGL::BindTextureUnit(1, scratchFbo.GetColorAttachmentHandleByName("RGBA16F"));

        OpenGL::DispatchCompute(gBuffer.GetWidth() / 8, gBuffer.GetHeight() / 8, 1);
    }

    void PostProcessingPassRE() {
        RendererSettings& rendererSettings = Renderer::GetCurrentRendererSettings();
        RendererOverrideState state = rendererSettings.rendererOverrideState;

        if (state == RendererOverrideState::NONE ||
            state == RendererOverrideState::CAMERA_NDOTL ||
            state == RendererOverrideState::INDIRECT_DIFFUSE) {
            ToneMappingPassRE();
            FXAAPassRE();
        }
    }
}