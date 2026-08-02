#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Unloved/Render/Renderer.h"
#include "Unloved/Session/Session.h"
#include "World/LegacyWorld.h"

namespace OpenGL::Renderer {

    namespace {
        bool g_taaHistoryValid = false;
        int g_taaHistoryWidth = 0;
        int g_taaHistoryHeight = 0;
        SplitscreenMode g_taaHistorySplitscreenMode = SplitscreenMode::FULLSCREEN;

        void DispatchTAAViewport(const glm::ivec2& origin, const glm::ivec2& size) {
            OpenGL::SetUniformIVec2("u_viewportOrigin", origin);
            OpenGL::SetUniformIVec2("u_viewportSize", size);
            OpenGL::DispatchCompute((size.x + 15) / 16, (size.y + 15) / 16, 1);
        }

        void DispatchTAAViewports(const glm::ivec2& fullSize, SplitscreenMode splitscreenMode) {
            if (splitscreenMode == SplitscreenMode::FULLSCREEN) {
                DispatchTAAViewport(glm::ivec2(0), fullSize);
                return;
            }

            const int halfWidth = fullSize.x / 2;
            const int halfHeight = fullSize.y / 2;

            if (splitscreenMode == SplitscreenMode::TWO_PLAYER) {
                DispatchTAAViewport(glm::ivec2(0, 0), glm::ivec2(fullSize.x, halfHeight));
                DispatchTAAViewport(glm::ivec2(0, halfHeight), glm::ivec2(fullSize.x, fullSize.y - halfHeight));
                return;
            }

            DispatchTAAViewport(glm::ivec2(0, 0), glm::ivec2(halfWidth, halfHeight));
            DispatchTAAViewport(glm::ivec2(halfWidth, 0), glm::ivec2(fullSize.x - halfWidth, halfHeight));
            DispatchTAAViewport(glm::ivec2(0, halfHeight), glm::ivec2(halfWidth, fullSize.y - halfHeight));
            DispatchTAAViewport(glm::ivec2(halfWidth, halfHeight), glm::ivec2(fullSize.x - halfWidth, fullSize.y - halfHeight));
        }
    }

    // 1. TAA
    // 2. Emissive and bloom
    // 3. Tone mapping
    // 4. FXAA

    void TAAPass() {
        RendererSettings& rendererSettings = Unloved::Renderer::GetCurrentRendererSettings();
        if (!rendererSettings.enableTAA) {
            g_taaHistoryValid = false;
            return;
        }

        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer& gBuffer = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
        OpenGLFrameBuffer& taaFbo = OpenGL::ResourceManager::GetFrameBuffer("TAA");
        const glm::ivec2 renderSize(gBuffer.GetWidth(), gBuffer.GetHeight());
        const SplitscreenMode splitscreenMode = Unloved::Session::GetSplitscreenMode();

        if (g_taaHistoryWidth != renderSize.x ||
            g_taaHistoryHeight != renderSize.y ||
            g_taaHistorySplitscreenMode != splitscreenMode) {
            g_taaHistoryValid = false;
        }

        glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);

        // FidelityFX TAA accumulation: current HDR, depth, history and velocity -> compressed output.
        OpenGL::BindShader("TAA");
        OpenGL::BindTextureUnit(0, gBuffer.GetColorAttachmentHandleByName("Lighting"));
        OpenGL::BindTextureUnit(1, gBuffer.GetDepthAttachmentHandle());
        OpenGL::BindTextureUnit(2, taaFbo.GetColorAttachmentHandleByName("History"));
        OpenGL::BindTextureUnit(3, gBuffer.GetColorAttachmentHandleByName("VelocityXYOcclusionSubSurface"));
        OpenGL::BindImageTexture(0, taaFbo.GetColorAttachmentHandleByName("Output"), GL_WRITE_ONLY, GL_RGBA16F);
        OpenGL::SetUniformBool("u_historyValid", g_taaHistoryValid);
        DispatchTAAViewports(renderSize, splitscreenMode);

        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

        // FidelityFX post pass: restore HDR and replace history after all history reads are complete.
        OpenGL::BindShader("TAAPost");
        OpenGL::BindTextureUnit(0, taaFbo.GetColorAttachmentHandleByName("Output"));
        OpenGL::BindImageTexture(0, gBuffer.GetColorAttachmentHandleByName("Lighting"), GL_WRITE_ONLY, GL_RGBA16F);
        OpenGL::BindImageTexture(1, taaFbo.GetColorAttachmentHandleByName("History"), GL_WRITE_ONLY, GL_RGBA16F);
        OpenGL::DispatchCompute((renderSize.x + 7) / 8, (renderSize.y + 7) / 8, 1);

        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

        g_taaHistoryValid = true;
        g_taaHistoryWidth = renderSize.x;
        g_taaHistoryHeight = renderSize.y;
        g_taaHistorySplitscreenMode = splitscreenMode;
    }

    void ToneMappingPassRE() {
        ProfilerOpenGLZoneFunction();
        OpenGLFrameBuffer& gBuffer = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
        OpenGLFrameBuffer& scratchFbo = OpenGL::ResourceManager::GetFrameBuffer("Scratch");

        OpenGL::BindShader("PostProcessing");
        OpenGL::BindSSBO(SSBO_IDX_RENDERER_DATA, "RendererData");
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
        OpenGL::BindImageTexture(0, scratchFbo.GetColorAttachmentHandleByName("RGBA16F"), GL_WRITE_ONLY, GL_RGBA16F);
        OpenGL::BindImageTexture(1, gBuffer.GetColorAttachmentHandleByName("Lighting"), GL_READ_ONLY, GL_RGBA16F);

        OpenGL::DispatchCompute((scratchFbo.GetWidth() + 7) / 8, (scratchFbo.GetHeight() + 7) / 8, 1);
        glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_FRAMEBUFFER_BARRIER_BIT);
    }

    void FXAAPassRE() {
        RendererSettings& rendererSettings = Unloved::Renderer::GetCurrentRendererSettings();
        OpenGLFrameBuffer& gBuffer = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
        OpenGLFrameBuffer& scratchFbo = OpenGL::ResourceManager::GetFrameBuffer("Scratch");

        if (rendererSettings.enableFXAA) {
            ProfilerOpenGLZoneFunction();

            gBuffer.Bind();
            gBuffer.SetViewport();
            gBuffer.DrawBuffer("Lighting");

            OpenGL::BindShader("FXAA");
            OpenGL::BindSSBO(SSBO_IDX_RENDERER_DATA, "RendererData");
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
        else {
            OpenGL::BlitFrameBuffer(&scratchFbo, &gBuffer, "RGBA16F", "Lighting", GL_COLOR_BUFFER_BIT, GL_LINEAR);
        }
    }

    void PostProcessingPassRE() {
        RendererSettings& rendererSettings = Unloved::Renderer::GetCurrentRendererSettings();
        RendererOverrideState state = rendererSettings.rendererOverrideState;

        if (state == RendererOverrideState::NONE ||
            state == RendererOverrideState::CAMERA_NDOTL ||
            state == RendererOverrideState::INDIRECT_DIFFUSE) {

            TAAPass();
            EmissivePass();
            ToneMappingPassRE();
            FXAAPassRE();
        }
        else {
            g_taaHistoryValid = false;
        }
    }
}
