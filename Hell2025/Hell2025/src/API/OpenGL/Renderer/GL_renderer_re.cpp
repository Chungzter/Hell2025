#include "GL_renderer.h"
#include "API/OpenGL/GL_backEnd.h"
#include "AssetManagement/AssetManager.h"
#include "Renderer/RenderDataManager.h"
#include "Renderer/Renderer.h"
#include "World/World.h"
#include "Viewport/ViewportManager.h"

#include "Hell/RendereringConstants.h"
#include "Ocean/Ocean.h"
#include "Core/Game.h"

namespace OpenGLRenderer {
    struct RESettings {
        glm::ivec2 gBufferResolution = glm::ivec2(1920, 1080);
        glm::ivec2 finalImageResolution = glm::ivec2(1920, 1080) / 2;
    } g_settings;

    void BlendedLighting();
    void ClearRenderTargetsRE();
    void CreateFramebuffersRE();

    void LightingPassRE();
    void LoadShadersRE();
    void SkyboxPassRE();
    void GlassPassRE();
    void OceanRE();
    void EmissiveForwardPass();

    void RenderFullscreenTriangle();

    void InitREStyle() {
        CreateFramebuffersRE();
        LoadShadersRE();
    }

    void RenderGameREStyle() {
        ComputeOceanFFTPass();

        OpenGLRasterizerState state;
        state.depthMask = true;
        state.colorMask = true;
        ForceRasterizerState(state);

        ComputeSkinningPass();
        UpdateSSBOS();
        RenderShadowMaps();
        ClearRenderTargetsRE();

        VisibilityPass();
        VisibilitySkinnedPass();
        VisibilityAlphaDiscardPass();
        VisibilitySkinnedHairPass();

        MaterialResolvePass();
        MaterialResolveSkinnedPass();
        MaterialResolveProceduralPass();

        EmissiveForwardPass();

        ComputeTileWorldBounds();
        ChristmasLightCullingPass();
        LightCullingPass();

        // TODO: BloodDecalsPass(); // this pass has a pretty different setup to this renderer, think this one through

        UpdateGlobalIllumintation();

        // TODO: Don't let this renderer just assume these SSBOs are always bound here.
        // It's a nightmare. Explicitly rebind them for every render pass that needs them.
        BindSSBO(0, "Samplers");
        BindSSBO(1, "RendererData");
        BindSSBO(2, "ViewportData");
        BindSSBO(3, "InstanceData");
        BindSSBO(4, "Lights");
        BindSSBO(5, "TileLights");
        // TODO: BindSSBO(7, "TileChristmasLights");
        // TODO: BindSSBO(8, "ChristmasLightInstances");
        // TODO: BindSSBO(9, "ChristmasLightIndices");

        LightingPassRE();
        BlendedLighting();

        SkyboxPassRE();
        HairPassRE();
        OceanRE();
        
        GaussianBlur();

        OceanUnderWaterFlags();
        OceanSurfaceCompositePass();

        GlassPassRE();
        StainedGlassPass();

        EmissivePass();

        OceanUnderwaterCompositePass();

        // DDGI Debug
        DDGIVolume& ddgiVolume = World::GetTestDDGIVolume();
        if (Renderer::GetCurrentRendererSettings().debugDrawPointCloud)       DrawPointCloud(ddgiVolume);
        if (Renderer::GetCurrentRendererSettings().debugDrawPointCloudGrid)   DrawPointCloudGrid(ddgiVolume);
        if (Renderer::GetCurrentRendererSettings().debugDrawIrradianceProbes) DrawProbes(ddgiVolume);

        SpriteSheetPass(); // Muzzle flash, etc

        PostProcessingPassRE();
        DebugViewPass();
        DebugPass();

        // Downscale and blit to swapchain
        OpenGLFrameBuffer& finalImageFbo = GetFrameBuffer("FinalImage");
        OpenGLFrameBuffer& gBufferRE = GetFrameBuffer("GBufferRE");
        OpenGLRenderer::BlitFrameBuffer(&gBufferRE, &finalImageFbo, "Lighting", "Color", GL_COLOR_BUFFER_BIT, GL_LINEAR);
        OpenGLRenderer::BlitToDefaultFrameBuffer(&finalImageFbo, "Color", GL_COLOR_BUFFER_BIT, GL_NEAREST);

        UIPass();
    }

    void CreateFramebuffersRE() {
        OpenGLFrameBuffer& gBufferRE = CreateFrameBuffer("GBufferRE", g_settings.gBufferResolution);
        gBufferRE.CreateAttachment("Lighting", GL_RGBA16F);
        gBufferRE.CreateAttachment("BaseColorMetallic", GL_RGBA8);
        gBufferRE.CreateAttachment("NormalXYRoughnessMisc", GL_RGB10_A2);
        gBufferRE.CreateAttachment("VelocityXYOcclusionSubSurface", GL_RGBA16F);
        gBufferRE.CreateAttachment("Visibility", GL_RG32UI);
        gBufferRE.CreateAttachment("Emissive", GL_RGBA8);
        gBufferRE.CreateDepthAttachment(GL_DEPTH32F_STENCIL8);

        gBufferRE.CreateAttachment("Glass", GL_RGBA16F); // Remove/rethink me

        OpenGLFrameBuffer& hairFboRE = CreateMultisampledFrameBuffer("HairRE", g_settings.gBufferResolution, 4);
        hairFboRE.CreateAttachment("Lighting", GL_RGBA16F);
        hairFboRE.CreateDepthAttachment(GL_DEPTH32F_STENCIL8);
    }

    void LoadShadersRE() {
        LoadShader("RE", "DepthPrePassRE", { "GL_depth_prepass.vert", "GL_depth_prepass.frag" });
        LoadShader("RE", "DepthPrePassAlphaDiscardRE", { "GL_depth_prepass_alpha_discard.vert", "GL_depth_prepass_alpha_discard.frag" });
        LoadShader("RE", "GBufferRE", { "GL_gbuffer_re.vert", "GL_gbuffer_re.frag" });
        LoadShader("RE", "LightingDeferred", { "GL_fullscreen_triangle.vert", "GL_lighting_deferred.frag" });

        LoadShader("RE", "HairLightingForward", { "GL_hair_lighting_forward.vert", "GL_hair_lighting_forward.frag" });
        LoadShader("RE", "HairLightingForwardOLD", { "GL_hair_lighting_forward.vert", "GL_hair_lighting_forward_old.frag" });
        LoadShader("RE", "HairCompositeRE", { "GL_hair_composite_re.comp" });
        LoadShader("RE", "HairDepthPrep", { "GL_fullscreen_triangle.vert", "GL_hair_depth_prep.frag" });

        LoadShader("RE", "Visibility", { "GL_visibility.vert", "GL_visibility.frag" });
        LoadShader("RE", "VisibilityAlphaDiscard", { "GL_visibility.vert", "GL_visibility_alpha_discard.frag" });
        LoadShader("RE", "MaterialResolve", { "GL_material_resolve.vert", "GL_material_resolve.frag" });
        LoadShader("RE", "MaterialResolveSkinning", { "GL_material_resolve.vert", "GL_material_resolve.frag" }, { "SKINNED" });

        // TODO: using GL_gbuffer_re.vert is confusing, you probably have a lot of identical shaders that perform
        // what this does, check that, and think of a more unified name.
        LoadShader("RE", "EmissiveForward", { "GL_gbuffer_re.vert", "GL_emissive_forward.frag" });

        LoadShader("RE", "LightingForward", { "GL_lighting_forward.vert", "GL_lighting_forward.frag" });
        LoadShader("RE", "SkyboxRE", { "GL_fullscreen_triangle.vert", "GL_skybox_re.frag" });

        LoadShader("RE", "OceanLighting", { "GL_ocean_lighting.vert", "GL_ocean_lighting.frag" });
    }

	void ClearRenderTargetsRE() {
		OpenGLFrameBuffer& gBufferRE = GetFrameBuffer("GBufferRE");
		gBufferRE.ClearAttachment("Lighting", 0, 0, 0, 1);
		gBufferRE.ClearAttachment("BaseColorMetallic", 0, 0, 0, 1);
		gBufferRE.ClearAttachment("NormalXYRoughnessMisc", 0, 0, 0, 1);
        gBufferRE.ClearAttachment("VelocityXYOcclusionSubSurface", 0, 0, 0, 1);
        gBufferRE.ClearAttachment("Emissive", 0.0f, 0.0f, 0.0f, 0.0f);
        gBufferRE.ClearAttachmentUI("Visibility", 0, 0, 0, 0);
        gBufferRE.ClearDepthAttachment(0.0f);
        gBufferRE.ClearStencilBits(0);

		OpenGLFrameBuffer& hairFboRE = GetFrameBuffer("HairRE");
		hairFboRE.ClearAttachment("Lighting", 0, 0, 0, 0);
		hairFboRE.ClearDepthAttachment(0.0f);

        OpenGLFrameBuffer& waterFrameBuffer = GetFrameBuffer("Water");
        waterFrameBuffer.Bind();
        waterFrameBuffer.ClearAttachment("Lighting", 0, 0, 0, 0);
        waterFrameBuffer.ClearAttachmentUI("OceanFlags", 0, 0, 0, 0);
        waterFrameBuffer.ClearAttachmentUI("OceanMask", 0, 0, 0, 0);
	}

	void BindShadowMapsRE() {
		OpenGLShadowMap& flashLightShadowMaps = GetShadowMap("FlashlightShadowMaps");
		OpenGLShadowCubeMapArray& hiResShadowMaps = GetShadowCubeMapArray("HiRes");
		OpenGLShadowMapArray& moonShadowCascades = GetShadowMapArray("MoonlightCSM");

		BindTextureUnit(7, GetTextureHandleByName("Flashlight2"));
		BindTextureUnit(8, flashLightShadowMaps.GetDepthTextureHandle());
		BindTextureUnit(9, hiResShadowMaps.GetDepthTexture());
		BindTextureUnit(10, moonShadowCascades.GetDepthTexture());
	}

    void LightingPassRE() {
        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer& gBuffer = GetFrameBuffer("GBufferRE");
        OpenGLFrameBuffer& indirectDiffuseFbo = GetFrameBuffer("IndirectDiffuse");

        BindShader("LightingDeferred");
        BindShadowMapsRE();

        std::vector<float>& cascadeLevels = GetShadowCascadeLevels();
        SetUniformFloat("u_cascadeFarPlane", 256.0f); // ???
        SetUniformFloat("u_cascadePlaneDistances[0]", cascadeLevels[0]);
        SetUniformFloat("u_cascadePlaneDistances[1]", cascadeLevels[1]);
        SetUniformFloat("u_cascadePlaneDistances[2]", cascadeLevels[2]);
        SetUniformFloat("u_cascadePlaneDistances[3]", cascadeLevels[3]);

        BindTextureUnit(1, gBuffer.GetColorAttachmentHandleByName("BaseColorMetallic"));
        BindTextureUnit(2, gBuffer.GetColorAttachmentHandleByName("NormalXYRoughnessMisc"));
        BindTextureUnit(3, gBuffer.GetColorAttachmentHandleByName("VelocityXYOcclusionSubSurface"));
        BindTextureUnit(4, gBuffer.GetDepthAttachmentHandle());
        BindTextureUnit(5, indirectDiffuseFbo.GetColorAttachmentHandleByName("Color"));

        OpenGLFrameBuffer& fbo = GetFrameBuffer("GBufferRE");
        fbo.Bind();
        fbo.SetViewport();
        fbo.DrawBuffers({ "Lighting" });

        OpenGLRasterizerState state;
        state.depthTestEnabled = false;
        state.blendEnable = false;
        state.cullfaceEnable = false;
        state.depthMask = false;
        state.colorMask = true;

        state.stencilTestEnabled = true;
        state.stencilFunc = GL_EQUAL;
        state.stencilWriteMask = 0x00;
        state.stencilFailOp = GL_KEEP;
        state.stencilDepthFailOp = GL_KEEP;
        state.stencilPassOp = GL_KEEP;
        state.stencilRef = 0;
        state.stencilReadMask = STENCIL_BIT_SKINNED_HAIR;

        SetRasterizerState(state);

        RenderFullscreenTriangle();
	}

	void BlendedLighting() {
		ProfilerOpenGLZoneFunction();

		const DrawCommandsSet& drawInfoSet = RenderDataManager::GetDrawInfoSet();

		OpenGLFrameBuffer& fbo = GetFrameBuffer("GBufferRE");
		fbo.Bind();
		fbo.DrawBuffers({ "Lighting" });

		OpenGLRasterizerState state;
		state.depthTestEnabled = true;
		state.blendEnable = true;
		state.cullfaceEnable = false;
		state.depthMask = false;
		state.colorMask = true;
		state.depthFunc = GL_GREATER;

		// Opaque
		OpenGLShader& opaqueShader = GetShader("LightingForward");
		opaqueShader.Bind();

		glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
		MultiDrawPerViewport(fbo, opaqueShader, drawInfoSet.blended, state);

		//glBindVertexArray(OpenGLBackEnd::GetWeightedVertexDataVAO());
		MultiDrawPerViewport(fbo, opaqueShader, drawInfoSet.skinnedNonDeformingBlended, state);

		glBindVertexArray(OpenGLBackEnd::GetSkinnedVertexDataVAO());
		MultiDrawPerViewport(fbo, opaqueShader, drawInfoSet.skinnedBlended, state);
	}

    void SkyboxPassRE() {
        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer& gBuffer = GetFrameBuffer("GBufferRE");
        OpenGLCubemapView* skyboxCubemapView = GetCubemapViewOLD("SkyboxNightSky");

        gBuffer.Bind();
        gBuffer.SetViewport();
        gBuffer.DrawBuffers({ "Lighting" });

        BindShader("SkyboxRE");

        OpenGLRasterizerState state;
        state.depthTestEnabled = false;
        state.blendEnable = false;
        state.cullfaceEnable = false;
        state.depthMask = false;
        state.depthFunc = GL_GREATER;
        
        state.stencilTestEnabled = true;
        state.stencilFunc = GL_EQUAL;
        state.stencilRef = 0; // This is any non-rendered pixel
        state.stencilReadMask = 0xFF;
        state.stencilWriteMask = 0x00;
        state.stencilFailOp = GL_KEEP;
        state.stencilDepthFailOp = GL_KEEP;
        state.stencilPassOp = GL_KEEP;

        SetRasterizerState(state);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxCubemapView->GetHandle());
        glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
        
        RenderFullscreenTriangle();
    }

    void OceanRE() {
        ProfilerOpenGLZoneFunction();
        if (!World::HasOcean()) return;
        
        const std::vector<ViewportData>& viewportData = RenderDataManager::GetViewportData();

        OpenGLCubemapView& skyboxCubemapView = GetCubemapView("SkyboxNightSky");
        OpenGLFrameBuffer& fftBand0Fbo = GetFrameBuffer("FFT_band0");
        OpenGLFrameBuffer& fftBand1Fbo = GetFrameBuffer("FFT_band1");
        OpenGLFrameBuffer& gBuffer = GetFrameBuffer("GBufferRE");
        OpenGLFrameBuffer& waterFbo = GetFrameBuffer("Water");

        const int gridSize = 128;
        const int lodLevels = 6;
        const int vertexCount = gridSize * gridSize * 6 * lodLevels;

        BindShader("OceanLighting");
        SetUniformInt("u_gridWidth", gridSize);
        SetUniformFloat("u_oceanOriginY", Ocean::GetOceanOriginY());
        SetUniformFloat("u_time", Game::GetTotalTime());

        BindTextureUnit(0, fftBand0Fbo.GetColorAttachmentHandleByName("Displacement"));
        BindTextureUnit(1, fftBand0Fbo.GetColorAttachmentHandleByName("Normals"));
        BindTextureUnit(2, fftBand1Fbo.GetColorAttachmentHandleByName("Displacement"));
        BindTextureUnit(3, fftBand1Fbo.GetColorAttachmentHandleByName("Normals"));
        BindTextureUnit(4, skyboxCubemapView.GetHandle());
        BindTextureUnit(5, GetTextureHandleByName("WaterNormals"));

        OpenGLRasterizerState state;
        state.depthTestEnabled = true;
        state.blendEnable = false;
        state.cullfaceEnable = false;
        state.depthMask = true;
        state.colorMask = true;
        state.depthFunc = GL_GREATER;

        SetRasterizerState(state);
        BindEmptyVAO();

        static bool lines = false;
        if (Input::KeyPressed(HELL_KEY_L)) {
            lines = !lines;
        }

        OpenGLRenderer::BlitFrameBufferDepth(&gBuffer, &waterFbo);

        waterFbo.Bind();
        waterFbo.DrawBuffers({ "Lighting", "OceanMask" });

        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGLRenderer::SetViewport(&waterFbo, viewport);
            SetUniformInt("u_viewportIndex", i);

            if (lines) glDrawArrays(GL_LINES, 0, vertexCount);
            else       glDrawArrays(GL_TRIANGLES, 0, vertexCount);
        }

        glBindVertexArray(0);
    }

    void GlassPassRE() {
        ProfilerOpenGLZoneFunction();

        ForceRasterizerState("GlassPass");

        const std::vector<ViewportData>& viewportData = RenderDataManager::GetViewportData();

        OpenGLShader* shader = GetShaderOLD("Glass");
        OpenGLShader* compositeShader = GetShaderOLD("GlassComposite");
        OpenGLFrameBuffer* gBuffer = GetFrameBufferOLD("GBufferRE");
        OpenGLShadowMap* flashLightShadowMapsFBO = GetShadowMapOLD("FlashlightShadowMaps");

        if (!shader) return;
        if (!compositeShader) return;
        if (!gBuffer) return;
        if (!flashLightShadowMapsFBO) return;

        shader->Bind();
        shader->SetBool("u_flipNormalMapY", ShouldFlipNormalMapY());

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
        SetRasterizerState(state);

        glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
        glBindTextureUnit(0, gBuffer->GetDepthAttachmentHandle());
        glBindTextureUnit(7, GetTextureHandleByName("Flashlight2"));
        glBindTextureUnit(8, flashLightShadowMapsFBO->GetDepthTextureHandle());

        // Forward render each glass render item into each viewport
        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGLRenderer::SetViewport(gBuffer, viewport);
            shader->SetInt("u_viewportIndex", i);

            for (const RenderItem& renderItem : RenderDataManager::GetRenderItemsGlass()) {
                shader->SetMat4("u_modelMatrix", renderItem.modelMatrix);

                Mesh* mesh = AssetManager::GetMeshByIndex(renderItem.meshIndex);
                if (!mesh) continue;

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByIndex(renderItem.baseColorTextureIndex)->GetGLTexture().GetHandle());
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByIndex(renderItem.normalMapTextureIndex)->GetGLTexture().GetHandle());
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByIndex(renderItem.rmaTextureIndex)->GetGLTexture().GetHandle());

                glDrawElementsBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), mesh->baseVertex);
            }
        }

        // Composite that render back into the lighting texture
        //gBuffer->SetViewport();
        //compositeShader->Bind();
        //glBindImageTexture(0, gBuffer->GetColorAttachmentHandleByName("Lighting"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
        //glBindImageTexture(1, gBuffer->GetColorAttachmentHandleByName("Glass"), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);
        //glDispatchCompute(gBuffer->GetWidth() / 16, gBuffer->GetHeight() / 4, 1);
        //
        //glDepthMask(GL_TRUE);
    }

    void EmissiveForwardPass() {
        ProfilerOpenGLZoneFunction();
        const DrawCommandsSet& drawInfoSet = RenderDataManager::GetDrawInfoSet();

        OpenGLFrameBuffer& fbo = GetFrameBuffer("GBufferRE");
        fbo.Bind();
        fbo.DrawBuffers({ "Emissive" });

        BindShader("EmissiveForward");

        BindSSBO(0, "Samplers");
        BindSSBO(1, "RendererData");
        BindSSBO(2, "ViewportData");
        BindSSBO(3, "InstanceData");

        OpenGLRasterizerState state;
        state.depthTestEnabled = true;
        state.blendEnable = false;
        state.cullfaceEnable = false;
        state.depthMask = false;
        state.colorMask = true;
        state.depthFunc = GL_EQUAL;

        glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());

        MultiDrawPerViewportRE(fbo, drawInfoSet.emissive, state);
    }

    void RenderFullscreenTriangle() {
        glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }
}