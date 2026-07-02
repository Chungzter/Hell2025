#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Hell/Render/API/OpenGL/GL_back_end.h"
#include "Unloved/Render/RendererConstants.h"
#include "Unloved/Render/Renderer.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Hell/Audio.h"
namespace Audio = Hell::Audio;
#include "Unloved/Debug/Debug.h"

#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/Input.h"
namespace Input = Hell::Input;


namespace OpenGLRenderer {

    //void HairLightingSkinnedResolvePass();
    void HairDepthPrep();
    void HairDepthPrePassRE();
    void HairForwardLightingPassRE();
    void HairCompositeRE();

    void HairPassRE() {
        HairDepthPrep();
        HairDepthPrePassRE();
        HairForwardLightingPassRE();
        HairCompositeRE();
    }

    void HairDepthPrep() {
        ProfilerOpenGLZoneFunction();

        static uint32_t dummyVao = 0;
        if (dummyVao == 0) {
            glGenVertexArrays(1, &dummyVao);
        }

        OpenGLFrameBuffer& gBuffer = OpenGL::ResourceManager::GetFrameBuffer("GBufferRE");
        OpenGLFrameBuffer& hairfbo = OpenGL::ResourceManager::GetFrameBuffer("HairRE");

        hairfbo.Bind();
        hairfbo.SetViewport();
        hairfbo.DrawBuffer("Lighting");

        OpenGL::BindShader("HairDepthPrep");
        OpenGL::BindTextureUnit(0, gBuffer.GetDepthAttachmentHandle());

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

        OpenGLRasterizerStateManager::SetRasterizerState(state);
        RenderFullscreenTriangle();
    }

    void HairDepthPrePassRE() {
        ProfilerOpenGLZoneFunction();

        const DrawCommandsSet& drawInfoSet = Unloved::RenderDataManager::GetDrawInfoSet();

        OpenGLFrameBuffer& fbo = OpenGL::ResourceManager::GetFrameBuffer("HairRE");
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
        OpenGLShader& shader = OpenGL::ResourceManager::GetShader("DepthPrePassAlphaDiscardRE");
        OpenGL::BindShader("DepthPrePassAlphaDiscardRE");

        Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("AssetGeometry");
        glBindVertexArray(meshBuffer.GetVAO());
        MultiDrawPerViewport(&fbo, &shader, drawInfoSet.hair, state);
        MultiDrawPerViewport(&fbo, &shader, drawInfoSet.skinnedNonDeformingHair, state);

        glBindVertexArray(OpenGL::BackEnd::GetSkinnedVertexDataVAO());
        MultiDrawPerViewport(&fbo, &shader, drawInfoSet.skinnedHair, state);

        glBindVertexArray(0);
    }

    void HairForwardLightingPassRE() {
        ProfilerOpenGLZoneFunction();
        const DrawCommandsSet& drawInfoSet = Unloved::RenderDataManager::GetDrawInfoSet();

        OpenGLFrameBuffer& fbo = OpenGL::ResourceManager::GetFrameBuffer("HairRE");
        OpenGLFrameBuffer& indirectDiffuseFbo = OpenGL::ResourceManager::GetFrameBuffer("IndirectDiffuse");

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
            OpenGL::BindShader("HairLightingForwardOLD");
        }
        else {
            OpenGL::BindShader("HairLightingForward");
        }

        OpenGL::SetUniformInt("u_renderResolutionScale", 1.0f);
        OpenGL::SetUniformInt("u_hairTextureIndex", Hell::ResourceManager::GetTextureBindlessIndexByName("RatKingHair_FLOW_ID_ROOT", true));
        OpenGL::SetUniformInt("u_hairBlendMapTextureIndex", Hell::ResourceManager::GetTextureBindlessIndexByName("Gold_ALB", true));             // YO!

        BindShadowMapsRE();
        OpenGL::BindTextureUnit(5, indirectDiffuseFbo.GetColorAttachmentHandleByName("Color"));

        OpenGLRasterizerState maskedState;
        maskedState.blendEnable = false;
        maskedState.cullfaceEnable = false;
        maskedState.colorMask = true;
        maskedState.depthFunc = GL_EQUAL;
        maskedState.depthMask = false;
        maskedState.depthTestEnabled = true;

        glBindVertexArray(OpenGL::BackEnd::GetSkinnedVertexDataVAO());
        MultiDrawPerViewportRE(fbo, drawInfoSet.skinnedHair, maskedState);

        OpenGL::BindShader("LightingForward");
        OpenGL::SetUniformBool("u_solidAlpha", true);

        Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("AssetGeometry");
        glBindVertexArray(meshBuffer.GetVAO());
        MultiDrawPerViewportRE(fbo, drawInfoSet.hair, maskedState);
        MultiDrawPerViewportRE(fbo, drawInfoSet.skinnedNonDeformingHair, maskedState);

        OpenGL::SetUniformBool("u_solidAlpha", false);
    }

    void HairCompositeRE() {
        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer& gBuffer = OpenGL::ResourceManager::GetFrameBuffer("GBufferRE");
        OpenGLFrameBuffer& hairFbo = OpenGL::ResourceManager::GetFrameBuffer("HairRE");

        OpenGL::BindShader("HairCompositeRE");

        OpenGL::BindImageTexture(0, gBuffer.GetColorAttachmentHandleByName("Lighting"), GL_READ_WRITE, GL_RGBA16F);
        OpenGL::BindImageTexture(1, gBuffer.GetColorAttachmentHandleByName("Emissive"), GL_WRITE_ONLY, GL_RGBA8);
        OpenGL::BindTextureUnit(2, hairFbo.GetColorAttachmentHandleByName("Lighting"));

        OpenGL::DispatchCompute(gBuffer.GetWidth() / TILE_SIZE, gBuffer.GetHeight() / TILE_SIZE, 1);
    }
}
