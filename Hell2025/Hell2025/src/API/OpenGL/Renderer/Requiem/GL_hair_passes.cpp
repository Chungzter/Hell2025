#include "API/OpenGL/Renderer/GL_renderer.h"
#include "API/OpenGL/GL_backend.h"
#include "Hell/RendereringConstants.h"
#include "Renderer/Renderer.h"
#include "Renderer/RenderDataManager.h"

#include "AssetManagement/AssetManager.h"
#include "Audio/Audio.h"
#include "Core/Debug.h"
#include "Input/Input.h"

namespace OpenGLRenderer {

    void HairDepthPrep() {
        ProfilerOpenGLZoneFunction();

        static uint32_t dummyVao = 0;
        if (dummyVao == 0) {
            glGenVertexArrays(1, &dummyVao);
        }

        OpenGLFrameBuffer& gBuffer = GetFrameBuffer("GBufferRE");
        OpenGLFrameBuffer& hairfbo = GetFrameBuffer("HairRE");

        hairfbo.Bind();
        hairfbo.SetViewport();
        hairfbo.DrawBuffer("Lighting");

        BindShader("HairDepthPrep");
        BindTextureUnit(0, gBuffer.GetDepthAttachmentHandle());

        OpenGLRasterizerState state;
        state.depthTestEnabled = true;
        state.blendEnable = false;
        state.cullfaceEnable = false;
        state.depthMask = true;
        state.colorMask = true;
        state.depthFunc = GL_ALWAYS;

        state.stencilTestEnabled = true;
        state.stencilFunc = GL_EQUAL;
        state.stencilRef = 0;
        state.stencilReadMask = STENCIL_BIT_SKINNED_HAIR;
        state.stencilWriteMask = 0x00;
        state.stencilFailOp = GL_KEEP;
        state.stencilDepthFailOp = GL_KEEP;
        state.stencilPassOp = GL_KEEP;

        SetRasterizerState(state);
        RenderFullscreenTriangle();
    }

    void HairDepthPrePassRE() {
        ProfilerOpenGLZoneFunction();

        const DrawCommandsSet& drawInfoSet = RenderDataManager::GetDrawInfoSet();

        OpenGLFrameBuffer& fbo = GetFrameBuffer("HairRE");
        fbo.Bind();
        fbo.DrawBuffer(GL_NONE);

        OpenGLRasterizerState state;
        state.depthTestEnabled = true;
        state.blendEnable = false;
        state.cullfaceEnable = false;
        state.depthMask = true;
        state.colorMask = false;
        state.depthFunc = GL_GREATER;

        // Masked
        OpenGLShader& shader = GetShader("DepthPrePassAlphaDiscardRE");
        shader.Bind();

        glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
        MultiDrawPerViewport(&fbo, &shader, drawInfoSet.hair, state);
        MultiDrawPerViewport(&fbo, &shader, drawInfoSet.skinnedNonDeformingHair, state);

        glBindVertexArray(OpenGLBackEnd::GetSkinnedVertexDataVAO());
        MultiDrawPerViewport(&fbo, &shader, drawInfoSet.skinnedHair, state);

        glBindVertexArray(0);
    }

    void HairForwardLightingPassRE() {
        ProfilerOpenGLZoneFunction();
        const DrawCommandsSet& drawInfoSet = RenderDataManager::GetDrawInfoSet();

        OpenGLFrameBuffer& fbo = GetFrameBuffer("HairRE");
        OpenGLFrameBuffer& indirectDiffuseFbo = GetFrameBuffer("IndirectDiffuse");

        fbo.Bind();
        fbo.DrawBuffers({ "Lighting" });

        static bool old = true;
        if (Input::KeyPressed(HELL_KEY_NUMPAD_3)) {
            old = !old;
            Audio::PlayAudio(AUDIO_SELECT, 1.0f);

            if (old) {
                Debug::BlitQuickDebugMessage("Hair: OLD");
            }
            else {
                Debug::BlitQuickDebugMessage("Hair: NEW");
            }
        }

        if (old) {
            BindShader("HairLightingForwardOLD");
        } 
        else {
            BindShader("HairLightingForward");
        }

        SetUniformInt("u_renderResolutionScale", 1.0f);
        SetUniformInt("u_hairTextureIndex", AssetManager::GetTextureIndexByName("RatKingHair_FLOW_ID_ROOT", true));
        SetUniformInt("u_hairBlendMapTextureIndex", AssetManager::GetTextureIndexByName("Gold_ALB", true));             // YO!

        BindShadowMapsRE();
        BindTextureUnit(5, indirectDiffuseFbo.GetColorAttachmentHandleByName("Color"));

        OpenGLRasterizerState maskedState;
        maskedState.blendEnable = false;
        maskedState.cullfaceEnable = false;
        maskedState.colorMask = true;
        maskedState.depthFunc = GL_EQUAL;
        maskedState.depthMask = false;
        maskedState.depthTestEnabled = true;

        glBindVertexArray(OpenGLBackEnd::GetSkinnedVertexDataVAO());
        MultiDrawPerViewportRE(fbo, drawInfoSet.skinnedHair, maskedState);

        BindShader("LightingForward");
        SetUniformBool("u_solidAlpha", true);

        glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
        MultiDrawPerViewportRE(fbo, drawInfoSet.hair, maskedState);
        MultiDrawPerViewportRE(fbo, drawInfoSet.skinnedNonDeformingHair, maskedState);

        SetUniformBool("u_solidAlpha", false);
    }

    void HairCompositeRE() {
        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer& gBuffer = GetFrameBuffer("GBufferRE");
        OpenGLFrameBuffer& hairFbo = GetFrameBuffer("HairRE");

        BindShader("HairCompositeRE");

        BindImageTexture(0, gBuffer.GetColorAttachmentHandleByName("Lighting"), GL_READ_WRITE, GL_RGBA16F);
        BindImageTexture(1, gBuffer.GetColorAttachmentHandleByName("Emissive"), GL_WRITE_ONLY, GL_RGBA8);
        BindTextureUnit(2, hairFbo.GetColorAttachmentHandleByName("Lighting"));

        glDispatchCompute(gBuffer.GetWidth() / TILE_SIZE, gBuffer.GetHeight() / TILE_SIZE, 1);
    }
}