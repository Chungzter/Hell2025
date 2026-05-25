#include "../GL_renderer.h"
#include "AssetManagement/AssetManager.h"
#include "Renderer/Renderer.h"
#include "World/World.h"

namespace OpenGLRenderer {

    // 1. TAA
    // 2. Tone mapping
    // 3. FXAA

    void ToneMapping() {
        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer& gBuffer = GetFrameBuffer("GBuffer");
        OpenGLFrameBuffer& scratchFbo = GetFrameBuffer("Scratch");

        BindShader("PostProcessing");

        BindImageTexture(0, scratchFbo.GetColorAttachmentHandleByName("RGBA16F"), GL_WRITE_ONLY, GL_RGBA16F);
        BindImageTexture(1, gBuffer.GetColorAttachmentHandleByName("Lighting"), GL_READ_ONLY, GL_RGBA16F);

        glDispatchCompute(gBuffer.GetWidth() / 8, gBuffer.GetHeight() / 8, 1);
    }

    void FXAA() {
        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer& gBuffer = GetFrameBuffer("GBuffer");
        OpenGLFrameBuffer& scratchFbo = GetFrameBuffer("Scratch");

        BindShader("FXAA");

        BindImageTexture(0, gBuffer.GetColorAttachmentHandleByName("Lighting"), GL_WRITE_ONLY, GL_RGBA16F);
        BindTextureUnit(1, scratchFbo.GetColorAttachmentHandleByName("RGBA16F"));

        glDispatchCompute(gBuffer.GetWidth() / 8, gBuffer.GetHeight() / 8, 1);
    }

    void PostProcessingPass() {
        RendererSettings& rendererSettings = Renderer::GetCurrentRendererSettings();
        RendererOverrideState state = rendererSettings.rendererOverrideState;

        if (state == RendererOverrideState::NONE ||
            state == RendererOverrideState::CAMERA_NDOTL ||
            state == RendererOverrideState::INDIRECT_DIFFUSE) {
            ToneMapping();
            FXAA();
        }
    }
}