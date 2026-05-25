#include "API/OpenGL/Renderer/GL_renderer.h"
#include "API/OpenGL/GL_backend.h"
#include "BackEnd/BackEnd.h"
#include "Config/Config.h"
#include "Viewport/ViewportManager.h"
#include "Renderer/RenderDataManager.h"

#include "AssetManagement/AssetManager.h"
#include "Audio/Audio.h"
#include "Input/Input.h"
#include "Renderer/Renderer.h"
#include "Core/Game.h"
#include "Core/Debug.h"
#include "Util/Util.h"

namespace OpenGLRenderer {

    bool g_cullFace = false;

    void RenderHairLayer(int peelCount);

    void UpdateHairDebugInput() {
        RendererSettings& renderSettings = Renderer::GetCurrentRendererSettings();
        int peelCount = renderSettings.depthPeelCount;
        if (Input::KeyPressed(HELL_KEY_RIGHT) && peelCount < 7) {
            Audio::PlayAudio("UI_Select.wav", 1.0f);
            renderSettings.depthPeelCount++;
            Debug::BlitQuickDebugMessage("Hair depth peel count: " + std::to_string(renderSettings.depthPeelCount));
        }
        if (Input::KeyPressed(HELL_KEY_LEFT) && peelCount > 0) {
            Audio::PlayAudio("UI_Select.wav", 1.0f);
            renderSettings.depthPeelCount--;
            Debug::BlitQuickDebugMessage("Hair depth peel count: " + std::to_string(renderSettings.depthPeelCount));
        }

        if (Input::KeyPressed(HELL_KEY_Z)) {
            Audio::PlayAudio("UI_Select.wav", 1.0f);
            g_cullFace = !g_cullFace;
            Debug::BlitQuickDebugMessage("Hair cull face enabled: " + Util::BoolToString(g_cullFace));
        }
    }

    void HairPass() {
        ProfilerOpenGLZoneFunction();

        const DrawCommandsSet& drawInfoSet = RenderDataManager::GetDrawInfoSet();
        const RendererSettings& renderSettings = Renderer::GetCurrentRendererSettings();
        const Resolutions& resolutions = Config::GetResolutions();

        OpenGLFrameBuffer* gBuffer = GetFrameBufferOLD("GBuffer");
        OpenGLFrameBuffer* hairFrameBuffer = GetFrameBufferOLD("Hair");
        OpenGLShader* depthPeelShader = GetShaderOLD("HairDepthPeel");
        OpenGLShader* hairLightingShader = GetShaderOLD("HairLighting");
        OpenGLShader* finalCompositeShader = GetShaderOLD("HairFinalComposite");
        OpenGLShadowCubeMapArray* hiResShadowMaps = GetShadowCubeMapArrayOLD("HiRes");

        if (!finalCompositeShader) return;
        if (!gBuffer) return;
        if (!hairFrameBuffer) return;
        if (!depthPeelShader) return;
        if (!hairLightingShader) return;
        if (!hiResShadowMaps) return;

        UpdateHairDebugInput();

        // Clear textures do initial values
        hairFrameBuffer->Bind();
        hairFrameBuffer->ClearAttachment("Composite", 0, 0, 0, 0);
        hairFrameBuffer->ClearAttachmentR("ViewspaceDepthPrevious", 0.0f);

        // Bind skinned VBO to weighted VAO
        glBindVertexArray(OpenGLBackEnd::GetSkinnedVertexDataVAO());
        glBindBuffer(GL_ARRAY_BUFFER, OpenGLBackEnd::GetSkinnedVertexDataVBO());
        //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, OpenGLBackEnd::GetWeightedVertexDataEBO());
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, OpenGLBackEnd::GetVertexDataEBO());

        for (int j = 0; j < renderSettings.depthPeelCount; j++) {

            ForceRasterizerState("GeometryPass_Default");
            glDrawBuffer(GL_NONE);
            glDepthFunc(GL_LESS);
            glDepthMask(GL_TRUE);
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

            hairFrameBuffer->ClearDepthAttachment();

            depthPeelShader->Bind();
            BindImageTexture(0, hairFrameBuffer->GetColorAttachmentHandleByName("ViewspaceDepthPrevious"), GL_READ_ONLY, GL_R32F);
            BindTextureUnit(1, gBuffer->GetDepthAttachmentHandle());

            if (!g_cullFace) glDisable(GL_CULL_FACE);

            // Standard hair depth
            glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
            for (int i = 0; i < 4; i++) {
                if (drawInfoSet.hair[i].empty()) continue;

                Viewport* viewport = ViewportManager::GetViewportByIndex(i);
                if (!viewport->IsVisible()) continue;
                
                OpenGLRenderer::SetViewport(hairFrameBuffer, viewport);
                if (BackEnd::RenderDocFound()) {
                    SplitMultiDrawIndirect(depthPeelShader, drawInfoSet.hair[i], false, false);
                }
                else {
                    MultiDrawIndirect(drawInfoSet.hair[i]);
                }
            }

            // Skinned hair
            glBindVertexArray(OpenGLBackEnd::GetSkinnedVertexDataVAO());
            for (int i = 0; i < 4; i++) {
                if (drawInfoSet.skinnedHair[i].empty()) continue;

                Viewport* viewport = ViewportManager::GetViewportByIndex(i);
                if (!viewport->IsVisible()) continue;

                OpenGLRenderer::SetViewport(hairFrameBuffer, viewport);
                if (BackEnd::RenderDocFound()) {
                    SplitMultiDrawIndirect(depthPeelShader, drawInfoSet.skinnedHair[i], false, false);
                }
                else {
                    MultiDrawIndirect(drawInfoSet.skinnedHair[i]);
                }
            }

            // Color pass
            ForceRasterizerState("HairLighting");

            if (!g_cullFace) glDisable(GL_CULL_FACE);

            hairFrameBuffer->DrawBuffers({ "Composite", "ViewspaceDepthPrevious" });

            int compositeBufferIndex = 0;
            int viewspaceDepthBufferIndex = 1;

            // Enable blending for draw buffer 0 (composite texture)
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            glDepthMask(GL_FALSE);
            glEnable(GL_BLEND);
            glDepthFunc(GL_EQUAL);
            glBlendEquationSeparatei(compositeBufferIndex, GL_FUNC_ADD, GL_FUNC_ADD);
            glBlendFuncSeparatei(compositeBufferIndex, /* RGB */ GL_ONE_MINUS_DST_ALPHA, GL_ONE, /* A*/ GL_ONE_MINUS_DST_ALPHA, GL_ONE);

            // Disable blending on draw buffer 1 (previous depth texture)
            glDisablei(GL_BLEND, viewspaceDepthBufferIndex);

            hairLightingShader->Bind();
            hairLightingShader->SetVec3("u_moonlightDir", Game::GetMoonlightDirection());
            glBindTextureUnit(4, AssetManager::GetTextureByName("Flashlight2")->GetGLTexture().GetHandle());

            glActiveTexture(GL_TEXTURE9);
            glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, hiResShadowMaps->GetDepthTexture());

            // Standard hair color
            glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
            for (int i = 0; i < 4; i++) {
                if (drawInfoSet.hair[i].empty()) continue;

                Viewport* viewport = ViewportManager::GetViewportByIndex(i);
                if (!viewport->IsVisible()) continue;

                OpenGLRenderer::SetViewport(hairFrameBuffer, viewport);
                if (BackEnd::RenderDocFound()) {
                    SplitMultiDrawIndirect(hairLightingShader, drawInfoSet.hair[i], true, false);
                }
                else {
                    MultiDrawIndirect(drawInfoSet.hair[i]);
                }
            }

            // Skinned hair color
            glBindVertexArray(OpenGLBackEnd::GetSkinnedVertexDataVAO());
            for (int i = 0; i < 4; i++) {
                if (drawInfoSet.skinnedHair[i].empty()) continue;

                Viewport* viewport = ViewportManager::GetViewportByIndex(i);
                if (!viewport->IsVisible()) continue;

                OpenGLRenderer::SetViewport(hairFrameBuffer, viewport);
                if (BackEnd::RenderDocFound()) {
                    SplitMultiDrawIndirect(hairLightingShader, drawInfoSet.skinnedHair[i], true, false);
                }
                else {
                    MultiDrawIndirect(drawInfoSet.skinnedHair[i]);
                }
            }
        }

        // Composite peeled final color back into gbuffer
        finalCompositeShader->Bind();

        BindImageTexture(0, gBuffer->GetColorAttachmentHandleByName("Lighting"), GL_READ_WRITE, GL_RGBA16F);
        BindTextureUnit(1, hairFrameBuffer->GetColorAttachmentHandleByName("Composite"));

        glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
        glDispatchCompute((gBuffer->GetWidth() + 7) / 8, (gBuffer->GetHeight() + 7) / 8, 1);

        // Clean up
        glBindVertexArray(0);
    }
}