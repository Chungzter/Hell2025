#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Unloved/Debug/Scratch.h"
#include "Hell/Render/API/OpenGL/GL_back_end.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Viewport/ViewportManager.h"
#include "World/LegacyWorld.h"

#include "Hell/ResourceManagement/ResourceManager.h"


namespace OpenGL::Renderer {

    void GlassMode0();
    void GlassMode1();
    void GlassMode2();
    void GlassMode3();

    int32_t GetGlassMode() {
        return Debug::Scratch::GetInt("Glass Mode");
    }

    void SetGlassMode(int32_t mode) {
        if (mode < 0 || mode > 3) return;
        Debug::Scratch::SetInt("Glass Mode", mode);
    }

    void GlassPass() {
        ProfilerOpenGLZoneFunctionRed();

        const int32_t glassMode = Debug::Scratch::GetInt("Glass Mode");
        if (glassMode == 0) GlassMode0();
        if (glassMode == 1) GlassMode1();
        if (glassMode == 2) GlassMode2();
        if (glassMode == 3) GlassMode3();
    }

    void GlassMode0() {
        OpenGL::RasterizerStateManager::ForceRasterizerState("GlassPass");

        const DrawCommandsSet& drawInfoSet = Unloved::RenderDataManager::GetDrawInfoSet();

        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("Glass");
        OpenGLShader* compositeShader = OpenGL::ResourceManager::GetShaderPtr("GlassComposite");
        OpenGLFrameBuffer* gBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");
        OpenGLShadowMap* flashLightShadowMapsFBO = OpenGL::ResourceManager::GetShadowMapPtr("FlashlightShadowMaps");

        if (!shader) return;
        if (!compositeShader) return;
        if (!gBuffer) return;
        if (!flashLightShadowMapsFBO) return;

        OpenGL::BindShader("Glass");
        OpenGL::BindSSBO(SSBO_IDX_SAMPLERS, "Samplers");
        OpenGL::BindSSBO(SSBO_IDX_MATERIALS, "Materials");
        OpenGL::BindSSBO(SSBO_IDX_RENDERER_DATA, "RendererData");
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
        OpenGL::BindSSBO(SSBO_IDX_LIGHTS, "Lights");
        OpenGL::BindSSBO(SSBO_IDX_GLASS_INSTANCE_DATA, "GlassInstanceData");
        OpenGL::BindSSBO(SSBO_IDX_GLASS_LIGHT_RANGES, "GlassLightRanges");
        OpenGL::BindSSBO(SSBO_IDX_GLASS_LIGHT_INDICES, "GlassLightIndices");
        OpenGL::SetUniformBool("u_flipNormalMapY", ShouldFlipNormalMapY());

        gBuffer->Bind();
        gBuffer->DrawBuffer("Glass");

        glBindVertexArray(OpenGL::ResourceManager::GetMeshBuffer("AssetGeometry").GetVAO());
        glBindTextureUnit(TEX_IDX_SHADOW_MAP_FLASHLIGHT, flashLightShadowMapsFBO->GetDepthTextureHandle());

        // Forward render each glass render item into each viewport
        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGL::Renderer::SetViewport(gBuffer, viewport);
            OpenGL::SetUniformInt("u_viewportIndex", i);

            MultiDrawIndirect(drawInfoSet.glassDrawCommands[i]);
        }

        // Composite that render back into the lighting texture
        gBuffer->SetViewport();
        OpenGL::BindShader("GlassComposite");
        glBindImageTexture(0, gBuffer->GetColorAttachmentHandleByName("Lighting"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
        glBindImageTexture(1, gBuffer->GetColorAttachmentHandleByName("Glass"), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);
        OpenGL::DispatchCompute(gBuffer->GetWidth() / 16, gBuffer->GetHeight() / 4, 1);

        glDepthMask(GL_TRUE);
    }

    void GlassMode1() {
        OpenGL::RasterizerStateManager::ForceRasterizerState("GlassPass");

        const DrawCommandsSet& drawInfoSet = Unloved::RenderDataManager::GetDrawInfoSet();

        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("Glass");
        OpenGLShader* compositeShader = OpenGL::ResourceManager::GetShaderPtr("GlassComposite");
        OpenGLFrameBuffer* gBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");
        OpenGLShadowMap* flashLightShadowMapsFBO = OpenGL::ResourceManager::GetShadowMapPtr("FlashlightShadowMaps");

        if (!shader) return;
        if (!compositeShader) return;
        if (!gBuffer) return;
        if (!flashLightShadowMapsFBO) return;

        OpenGL::BindShader("Glass");
        OpenGL::BindSSBO(SSBO_IDX_SAMPLERS, "Samplers");
        OpenGL::BindSSBO(SSBO_IDX_MATERIALS, "Materials");
        OpenGL::BindSSBO(SSBO_IDX_RENDERER_DATA, "RendererData");
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
        OpenGL::BindSSBO(SSBO_IDX_LIGHTS, "Lights");
        OpenGL::BindSSBO(SSBO_IDX_GLASS_INSTANCE_DATA, "GlassInstanceData");
        OpenGL::BindSSBO(SSBO_IDX_GLASS_LIGHT_RANGES, "GlassLightRanges");
        OpenGL::BindSSBO(SSBO_IDX_GLASS_LIGHT_INDICES, "GlassLightIndices");
        OpenGL::SetUniformBool("u_flipNormalMapY", ShouldFlipNormalMapY());

        gBuffer->Bind();
        gBuffer->DrawBuffer("Lighting");

        OpenGLRasterizerState state;
        state.depthTestEnabled = true;
        state.blendEnable = true;
        state.cullfaceEnable = false;
        state.depthMask = false;
        state.colorMask = true;
        state.depthFunc = GL_GREATER;
        state.blendFuncSrcfactor = GL_ONE;
        state.blendFuncDstfactor = GL_ONE;
        OpenGL::RasterizerStateManager::SetRasterizerState(state);

        glBindVertexArray(OpenGL::ResourceManager::GetMeshBuffer("AssetGeometry").GetVAO());
        glBindTextureUnit(TEX_IDX_SHADOW_MAP_FLASHLIGHT, flashLightShadowMapsFBO->GetDepthTextureHandle());

        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGL::Renderer::SetViewport(gBuffer, viewport);
            OpenGL::SetUniformInt("u_viewportIndex", i);

            MultiDrawIndirect(drawInfoSet.glassDrawCommands[i]);
        }
    }

    void GlassMode2() {
        OpenGL::RasterizerStateManager::ForceRasterizerState("GlassPass");

        const DrawCommandsSet& drawInfoSet = Unloved::RenderDataManager::GetDrawInfoSet();

        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("GlassNEW");
        OpenGLShader* compositeShader = OpenGL::ResourceManager::GetShaderPtr("GlassComposite");
        OpenGLFrameBuffer* gBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");
        OpenGLShadowMap* flashLightShadowMapsFBO = OpenGL::ResourceManager::GetShadowMapPtr("FlashlightShadowMaps");

        if (!shader) return;
        if (!compositeShader) return;
        if (!gBuffer) return;
        if (!flashLightShadowMapsFBO) return;

        OpenGL::BindShader("GlassNEW");
        OpenGL::BindSSBO(SSBO_IDX_SAMPLERS, "Samplers");
        OpenGL::BindSSBO(SSBO_IDX_MATERIALS, "Materials");
        OpenGL::BindSSBO(SSBO_IDX_RENDERER_DATA, "RendererData");
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
        OpenGL::BindSSBO(SSBO_IDX_LIGHTS, "Lights");
        OpenGL::BindSSBO(SSBO_IDX_GLASS_INSTANCE_DATA, "GlassInstanceData");
        OpenGL::BindSSBO(SSBO_IDX_GLASS_LIGHT_RANGES, "GlassLightRanges");
        OpenGL::BindSSBO(SSBO_IDX_GLASS_LIGHT_INDICES, "GlassLightIndices");
        OpenGL::SetUniformBool("u_flipNormalMapY", ShouldFlipNormalMapY());

        gBuffer->Bind();
        //gBuffer->DrawBuffer("Lighting");
        gBuffer->DrawBuffer(GL_NONE);

        OpenGLRasterizerState state;
        state.depthTestEnabled = true;
        state.blendEnable = false;
        state.cullfaceEnable = true;
        state.depthMask = false;
        state.colorMask = false;
        state.depthFunc = GL_GREATER;
        OpenGL::RasterizerStateManager::SetRasterizerState(state);

        glBindVertexArray(OpenGL::ResourceManager::GetMeshBuffer("AssetGeometry").GetVAO());
        glBindTextureUnit(TEX_IDX_SHADOW_MAP_FLASHLIGHT, flashLightShadowMapsFBO->GetDepthTextureHandle());

        BindImageTexture(5, gBuffer->GetColorAttachmentHandleByName("Lighting"), GL_READ_WRITE, GL_RGBA16F);

        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGL::Renderer::SetViewport(gBuffer, viewport);
            OpenGL::SetUniformInt("u_viewportIndex", i);

            MultiDrawIndirect(drawInfoSet.glassDrawCommands[i]);
        }
    }

    void GlassMode3() {
    }
}
