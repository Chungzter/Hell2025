#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Unloved/Render/Renderer.h"
#include "World/LegacyWorld.h"

namespace OpenGL::Renderer {

    // 1. TAA
    // 2. Tone mapping
    // 3. FXAA

    void ToneMappingPassRE() {
        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer& gBuffer = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
        OpenGLFrameBuffer& scratchFbo = OpenGL::ResourceManager::GetFrameBuffer("Scratch");

        OpenGL::BindShader("PostProcessing");

        OpenGL::BindImageTexture(0, scratchFbo.GetColorAttachmentHandleByName("RGBA16F"), GL_WRITE_ONLY, GL_RGBA16F);
        OpenGL::BindImageTexture(1, gBuffer.GetColorAttachmentHandleByName("Lighting"), GL_READ_ONLY, GL_RGBA16F);

        OpenGL::DispatchCompute((gBuffer.GetWidth() + 7) / 8, (gBuffer.GetHeight() + 7) / 8, 1);
        glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
    }

    void FXAAPassRE() {
        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer& gBuffer = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
        OpenGLFrameBuffer& scratchFbo = OpenGL::ResourceManager::GetFrameBuffer("Scratch");

        gBuffer.Bind();
        gBuffer.SetViewport();
        gBuffer.DrawBuffer("Lighting");

        OpenGL::BindShader("FXAA");
        OpenGL::BindTextureUnit(0, scratchFbo.GetColorAttachmentHandleByName("RGBA16F"));

        OpenGLRasterizerState state;
        state.depthTestEnabled = false;
        state.depthMask = false;
        state.cullfaceEnable = false;
        state.blendEnable = false;
        state.colorMask = true;
        OpenGL::RasterizerStateManager::SetRasterizerState(state);

        RenderFullscreenTriangle();
    }

    void PostProcessingPassRE() {
        RendererSettings& rendererSettings = Unloved::Renderer::GetCurrentRendererSettings();
        RendererOverrideState state = rendererSettings.rendererOverrideState;

        if (state == RendererOverrideState::NONE ||
            state == RendererOverrideState::CAMERA_NDOTL ||
            state == RendererOverrideState::INDIRECT_DIFFUSE) {
            ToneMappingPassRE();
            FXAAPassRE();
        }
    }
}
