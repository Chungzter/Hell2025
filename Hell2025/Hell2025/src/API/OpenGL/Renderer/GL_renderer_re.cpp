#include "GL_renderer.h"
#include "API/OpenGL/GL_backEnd.h"
#include "AssetManagement/AssetManager.h"
#include "Renderer/RenderDataManager.h"
#include "Renderer/Renderer.h"
#include "World/World.h"
#include "Viewport/ViewportManager.h"

namespace OpenGLRenderer {
	struct RESettings {
		glm::ivec2 gBufferResolution = glm::ivec2(1920, 1080);
		glm::ivec2 finalImageResolution = glm::ivec2(1920, 1080) / 2;
	} g_settings;

	void BindShadowMapsRE();
	void BlendedLighting();
	void ClearRenderTargetsRE();
	void CreateFramebuffersRE();
	void DepthPrePassRE();
	void GBufferPass();
	void HairDepthPrep();
	void HairDepthPrePassRE();
	void HairPassRE();
	void HairCompositeRE();
	void LightingPassRE();
	void LoadShadersRE();
	void PostProcessingPassRE();

    void VisibilityPass();
    void MaterialResolvePass();
	void VisibilityDebugPass();

	void InitREStyle() {
		CreateFramebuffersRE();
		LoadShadersRE();
	}

	void RenderGameREStyle() {
		ProfilerOpenGLFrame();

		OpenGLRasterizerState state;
		state.depthMask = true;
		state.colorMask = true;
		ForceRasterizerState(state);

		ComputeSkinningPass();
		UpdateSSBOS();
		RenderShadowMaps();
		ClearRenderTargetsRE();

        VisibilityPass();
        MaterialResolvePass();

        static bool test = false;

        if (!test) {
            VisibilityDebugPass();
		}

		if (Input::KeyPressed(HELL_KEY_NUMPAD_2)) {
			test = !test;
		}

		if (test) {
			//DepthPrePassRE();
			//GBufferPass();

			ComputeTileWorldBounds();
			ChristmasLightCullingPass();
			LightCullingPass();

			UpdateGlobalIllumintation();
		}

		// TODO: Don't let this renderer just assume these SSBOs are always bound here.
		// It's a nightmare. Explicitly rebind them for every render pass that needs them.
		BindSSBO("Samplers", 0);
		BindSSBO("RendererData", 1);
		BindSSBO("ViewportData", 2);
		BindSSBO("InstanceData", 3);
		BindSSBO("Lights", 4);

		if (test) {
			LightingPassRE();
			BlendedLighting();

			HairDepthPrep();
			HairDepthPrePassRE();
			HairPassRE();
			HairCompositeRE();

			PostProcessingPassRE();
		}
		 
		DebugViewPass();
		 
		// DDGI Debug
        DDGIVolume& ddgiVolume = World::GetTestDDGIVolume();
        if (Renderer::GetCurrentRendererSettings().debugDrawPointCloud)       DrawPointCloud(ddgiVolume);
        if (Renderer::GetCurrentRendererSettings().debugDrawPointCloudGrid)   DrawPointCloudGrid(ddgiVolume);
        if (Renderer::GetCurrentRendererSettings().debugDrawIrradianceProbes) DrawProbes(ddgiVolume);

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
		gBufferRE.CreateDepthAttachment(GL_DEPTH32F_STENCIL8);

		OpenGLFrameBuffer& hairFboRE = CreateMultisampledFrameBuffer("HairRE", g_settings.gBufferResolution, 4);
		hairFboRE.CreateAttachment("Lighting", GL_RGBA16F);
		hairFboRE.CreateDepthAttachment(GL_DEPTH32F_STENCIL8);
	}

	void LoadShadersRE() {
		LoadShader("RE", "DepthPrePassRE", { "GL_depth_prepass.vert", "GL_depth_prepass.frag" });
		LoadShader("RE", "DepthPrePassAlphaDiscardRE", { "GL_depth_prepass_alpha_discard.vert", "GL_depth_prepass_alpha_discard.frag" });
		LoadShader("RE", "GBufferRE", { "GL_gbuffer_re.vert", "GL_gbuffer_re.frag" });
		LoadShader("RE", "LightingRE", { "GL_lighting_re.comp" });
		LoadShader("RE", "HairLightingRE", { "GL_hair_lighting_re.vert", "GL_hair_lighting_re.frag" });
		LoadShader("RE", "HairCompositeRE", { "GL_hair_composite_re.comp" });
		LoadShader("RE", "HairDepthPrep", { "GL_fullscreen_triangle.vert", "GL_hair_depth_prep.frag" });

        LoadShader("RE", "Mesh", { "GL_mesh_shader.mesh", "GL_mesh_shader.frag" });
        LoadShader("RE", "Visibility", { "GL_visibility.vert", "GL_visibility.frag" });
        LoadShader("RE", "VisibilityDebug", { "GL_visibility_debug.comp" });
		LoadShader("RE", "MaterialResolve", { "GL_material_resolve.vert", "GL_material_resolve.frag" });
	}


	void VisibilityPass() {
        ProfilerOpenGLZoneFunction();

        const DrawCommandsSet& drawInfoSet = RenderDataManager::GetDrawInfoSet();

        OpenGLFrameBuffer& fbo = GetFrameBuffer("GBufferRE");
        fbo.Bind();
        fbo.DrawBuffers({ "Visibility" });

        OpenGLRasterizerState visibilityState;
        visibilityState.depthTestEnabled = true;
        visibilityState.blendEnable = false;
        visibilityState.cullfaceEnable = true;
        visibilityState.depthMask = true;
        visibilityState.colorMask = true;
        visibilityState.depthFunc = GL_GREATER;

		BindShader("Visibility");

        glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
        MultiDrawPerViewportRE(fbo, drawInfoSet.standard, visibilityState);
	}
	
    void MaterialResolvePass2() {
        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer& gbufferFbo = GetFrameBuffer("GBufferRE");
        gbufferFbo.Bind();
        gbufferFbo.DrawBuffers({ "BaseColorMetallic", "NormalXYRoughnessMisc", "VelocityXYOcclusionSubSurface" });

        BindShader("MaterialResolve");

        BindImageTexture(0, gbufferFbo.GetColorAttachmentHandleByName("Visibility"), GL_READ_ONLY, GL_RG32UI);
        BindImageTexture(1, gbufferFbo.GetColorAttachmentHandleByName("BaseColorMetallic"), GL_WRITE_ONLY, GL_RGBA8);
        BindImageTexture(2, gbufferFbo.GetColorAttachmentHandleByName("NormalXYRoughnessMisc"), GL_WRITE_ONLY, GL_RGB10_A2);
        BindImageTexture(3, gbufferFbo.GetColorAttachmentHandleByName("VelocityXYOcclusionSubSurface"), GL_WRITE_ONLY, GL_RGBA16F);
        BindTextureUnit(4, gbufferFbo.GetDepthAttachmentHandle());

        BindSSBO(OpenGLBackEnd::GetVertexDataVBO(), 0);
        BindSSBO(OpenGLBackEnd::GetVertexDataEBO(), 1);
        BindSSBO("ViewportData", 2);
        BindSSBO("InstanceData", 3);
        BindSSBO("Samplers", 4);
        BindSSBO("RendererData", 5);

        unsigned int width = gbufferFbo.GetWidth();
        unsigned int height = gbufferFbo.GetHeight();
        unsigned int groupX = (width + 15) / 16;
        unsigned int groupY = (height + 15) / 16;

        glDispatchCompute(groupX, groupY, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }

    void MaterialResolvePass() {
        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer& gbufferFbo = GetFrameBuffer("GBufferRE");
        gbufferFbo.Bind();
        gbufferFbo.DrawBuffers({ "BaseColorMetallic", "NormalXYRoughnessMisc", "VelocityXYOcclusionSubSurface" });

        BindShader("MaterialResolve");

        BindImageTexture(0, gbufferFbo.GetColorAttachmentHandleByName("Visibility"), GL_READ_ONLY, GL_RG32UI);
        BindTextureUnit(1, gbufferFbo.GetDepthAttachmentHandle());

        BindSSBO(OpenGLBackEnd::GetVertexDataVBO(), 0);
        BindSSBO(OpenGLBackEnd::GetVertexDataEBO(), 1);
        BindSSBO("ViewportData", 2);
        BindSSBO("InstanceData", 3);
        BindSSBO("Samplers", 4);
        BindSSBO("RendererData", 5);

        OpenGLRasterizerState resolveState;
        resolveState.depthTestEnabled = false;
        resolveState.blendEnable = false;
        resolveState.cullfaceEnable = false;
        resolveState.depthMask = false;
        resolveState.colorMask = true;
        SetRasterizerState(resolveState);

        // Procedural fullscreen draw
        glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

	void VisibilityDebugPass() {
        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer& gbufferFbo = GetFrameBuffer("GBufferRE");

        BindShader("VisibilityDebug");
        BindImageTexture(0, gbufferFbo.GetColorAttachmentHandleByName("Visibility"), GL_READ_ONLY, GL_RG32UI);
        BindImageTexture(1, gbufferFbo.GetColorAttachmentHandleByName("Lighting"), GL_WRITE_ONLY, GL_RGBA16F);

        unsigned int width = gbufferFbo.GetWidth();
        unsigned int height = gbufferFbo.GetHeight();
        unsigned int groupX = (width + 15) / 16;
        unsigned int groupY = (height + 15) / 16;

        glDispatchCompute(groupX, groupY, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	}


	void ClearRenderTargetsRE() {
		OpenGLFrameBuffer& gBufferRE = GetFrameBuffer("GBufferRE");
		gBufferRE.ClearAttachment("Lighting", 0, 0, 0, 1);
		gBufferRE.ClearAttachment("BaseColorMetallic", 0, 0, 0, 1);
		gBufferRE.ClearAttachment("NormalXYRoughnessMisc", 0, 0, 0, 1);
        gBufferRE.ClearAttachment("VelocityXYOcclusionSubSurface", 0, 0, 0, 1);
		gBufferRE.ClearAttachmentUI("Visibility", 0, 0, 0, 0);
		gBufferRE.ClearDepthAttachment(0.0f);

		OpenGLFrameBuffer& hairFboRE = GetFrameBuffer("HairRE");
		hairFboRE.ClearAttachment("Lighting", 0, 0, 0, 0);
		hairFboRE.ClearDepthAttachment(0.0f);
	}

	void DepthPrePassRE() {
		ProfilerOpenGLZoneFunction();

		const DrawCommandsSet& drawInfoSet = RenderDataManager::GetDrawInfoSet();

		OpenGLFrameBuffer& fbo = GetFrameBuffer("GBufferRE");
		fbo.Bind();
		fbo.DrawBuffer(GL_NONE);

		OpenGLRasterizerState opaqueDepthState;
		opaqueDepthState.depthTestEnabled = true;
		opaqueDepthState.blendEnable = false;
		opaqueDepthState.cullfaceEnable = true;
		opaqueDepthState.depthMask = true;
		opaqueDepthState.colorMask = false;
		opaqueDepthState.depthFunc = GL_GREATER;

		// Opaque
		OpenGLShader& opaqueShader = GetShader("DepthPrePassRE");
		opaqueShader.Bind();

		glBindVertexArray(World::GetHouseMeshBuffer().GetGLMeshBuffer().GetVAO());
		MultiDrawPerViewportRE(fbo, drawInfoSet.house, opaqueDepthState);

		glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
		MultiDrawPerViewportRE(fbo, drawInfoSet.standard, opaqueDepthState);

		glBindVertexArray(OpenGLBackEnd::GetWeightedVertexDataVAO());
		MultiDrawPerViewportRE(fbo, drawInfoSet.skinnedNonDeformingStandard, opaqueDepthState);

		glBindVertexArray(OpenGLBackEnd::GetSkinnedVertexDataVAO());
		MultiDrawPerViewportRE(fbo, drawInfoSet.skinnedStandard, opaqueDepthState);
	}

	void GBufferPass() {
		ProfilerOpenGLZoneFunction();

		const DrawCommandsSet& drawInfoSet = RenderDataManager::GetDrawInfoSet();

		OpenGLFrameBuffer& fbo = GetFrameBuffer("GBufferRE");
		fbo.Bind();
		fbo.DrawBuffers({ "BaseColorMetallic", "NormalXYRoughnessMisc", "VelocityXYOcclusionSubSurface" });

		OpenGLRasterizerState opaqueDepthState;
		opaqueDepthState.depthTestEnabled = true;
		opaqueDepthState.blendEnable = false;
		opaqueDepthState.cullfaceEnable = true;
		opaqueDepthState.depthMask = false;
		opaqueDepthState.colorMask = true;
		opaqueDepthState.depthFunc = GL_EQUAL;

		// Opaque
		OpenGLShader& opaqueShader = GetShader("GBufferRE");
		opaqueShader.Bind();

		glBindVertexArray(World::GetHouseMeshBuffer().GetGLMeshBuffer().GetVAO());
		MultiDrawPerViewport(fbo, opaqueShader, drawInfoSet.house, opaqueDepthState);

		glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
		MultiDrawPerViewport(fbo, opaqueShader, drawInfoSet.standard, opaqueDepthState);

		glBindVertexArray(OpenGLBackEnd::GetWeightedVertexDataVAO());
		MultiDrawPerViewport(fbo, opaqueShader, drawInfoSet.skinnedNonDeformingStandard, opaqueDepthState);

		glBindVertexArray(OpenGLBackEnd::GetSkinnedVertexDataVAO());
		MultiDrawPerViewport(fbo, opaqueShader, drawInfoSet.skinnedStandard, opaqueDepthState);
	}

	void BindShadowMapsRE() {
		OpenGLShadowMap& flashLightShadowMaps = GetShadowMap("FlashlightShadowMaps");
		OpenGLShadowCubeMapArray& hiResShadowMaps = GetShadowCubeMapArray("HiRes");
		OpenGLShadowMapArray& moonShadowCascades = GetShadowMapArray("MoonlightCSM");

		BindTextureUnit(7, AssetManager::GetTextureByName("Flashlight2")->GetGLTexture().GetHandle());
		BindTextureUnit(8, flashLightShadowMaps.GetDepthTextureHandle());
		BindTextureUnit(9, hiResShadowMaps.GetDepthTexture());
		BindTextureUnit(10, moonShadowCascades.GetDepthTexture());
	}

	void LightingPassRE() {
		OpenGLFrameBuffer& gBuffer = GetFrameBuffer("GBufferRE");
		OpenGLFrameBuffer& indirectDiffuseFbo = GetFrameBuffer("IndirectDiffuse");

		BindShader("LightingRE");
		BindShadowMapsRE();

		//BindSSBO("TileChristmasLights", 7);
		//BindSSBO("ChristmasLightInstances", 8);
		//BindSSBO("ChristmasLightIndices", 9);

		BindImageTexture(0, gBuffer.GetColorAttachmentHandleByName("Lighting"), GL_READ_WRITE, GL_RGBA16F);
		BindTextureUnit(1, gBuffer.GetColorAttachmentHandleByName("BaseColorMetallic"));
		BindTextureUnit(2, gBuffer.GetColorAttachmentHandleByName("NormalXYRoughnessMisc"));
		BindTextureUnit(3, gBuffer.GetColorAttachmentHandleByName("VelocityXYOcclusionSubSurface"));
		BindTextureUnit(4, gBuffer.GetDepthAttachmentHandle());
		BindTextureUnit(5, indirectDiffuseFbo.GetColorAttachmentHandleByName("Color"));

		glDispatchCompute(gBuffer.GetWidth() / TILE_SIZE, gBuffer.GetHeight() / TILE_SIZE, 1);
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
		OpenGLShader& opaqueShader = GetShader("ShadedHardSurface");
		opaqueShader.Bind();

		glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
		MultiDrawPerViewport(fbo, opaqueShader, drawInfoSet.blended, state);

		glBindVertexArray(OpenGLBackEnd::GetWeightedVertexDataVAO());
		MultiDrawPerViewport(fbo, opaqueShader, drawInfoSet.skinnedNonDeformingBlended, state);

		glBindVertexArray(OpenGLBackEnd::GetSkinnedVertexDataVAO());
		MultiDrawPerViewport(fbo, opaqueShader, drawInfoSet.skinnedBlended, state);
	}

	void HairDepthPrep() {
		ProfilerOpenGLZoneFunction();

		static uint32_t dummyVao = 0;
		if (dummyVao == 0) {
			glGenVertexArrays(1, &dummyVao);
		}

		OpenGLFrameBuffer& gBuffer = GetFrameBuffer("GBufferRE");
		OpenGLFrameBuffer& hairfbo = GetFrameBuffer("HairRE");

		hairfbo.Bind();
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
		SetRasterizerState(state);

		glDrawArrays(GL_TRIANGLES, 0, 3);
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
		OpenGLShader& shader = GetShader("DepthPrePassAlphaDiscard");
		shader.Bind();

		glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
		MultiDrawPerViewport(&fbo, &shader, drawInfoSet.alphaDiscard, state);
		MultiDrawPerViewport(&fbo, &shader, drawInfoSet.hair, state);

		glBindVertexArray(OpenGLBackEnd::GetWeightedVertexDataVAO());
		MultiDrawPerViewport(&fbo, &shader, drawInfoSet.skinnedNonDeformingHair, state);

		glBindVertexArray(OpenGLBackEnd::GetSkinnedVertexDataVAO());
		MultiDrawPerViewport(&fbo, &shader, drawInfoSet.skinnedHair, state);

		glBindVertexArray(0);
	}

	void HairPassRE() {
		ProfilerOpenGLZoneFunction();

		const DrawCommandsSet& drawInfoSet = RenderDataManager::GetDrawInfoSet();

		OpenGLFrameBuffer& fbo = GetFrameBuffer("HairRE");
		fbo.Bind();
		fbo.DrawBuffers({ "Lighting" });

		OpenGLShader& shader = GetShader("ShadedHair");
		shader.Bind();
		shader.SetFloat("u_renderResolutionScale", 1.0f);

		BindShadowMapsRE();

		OpenGLRasterizerState maskedState;
		maskedState.blendEnable = false;
		maskedState.cullfaceEnable = false;
		maskedState.colorMask = true;
		maskedState.depthFunc = GL_EQUAL;
		maskedState.depthMask = false;
		maskedState.depthTestEnabled = true;

		glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
		MultiDrawPerViewport(&fbo, &shader, drawInfoSet.hair, maskedState);

		glBindVertexArray(OpenGLBackEnd::GetWeightedVertexDataVAO());
		MultiDrawPerViewport(&fbo, &shader, drawInfoSet.skinnedNonDeformingHair, maskedState);

		glBindVertexArray(OpenGLBackEnd::GetSkinnedVertexDataVAO());
		MultiDrawPerViewport(&fbo, &shader, drawInfoSet.skinnedHair, maskedState);
	}

	void HairCompositeRE() {
		OpenGLFrameBuffer& gBuffer = GetFrameBuffer("GBufferRE");
		OpenGLFrameBuffer& hairFbo = GetFrameBuffer("HairRE");

		BindShader("HairCompositeRE");

		BindImageTexture(0, gBuffer.GetColorAttachmentHandleByName("Lighting"), GL_READ_WRITE, GL_RGBA16F);
		BindTextureUnit(1, hairFbo.GetColorAttachmentHandleByName("Lighting"));

		glDispatchCompute(gBuffer.GetWidth() / TILE_SIZE, gBuffer.GetHeight() / TILE_SIZE, 1);
	}

	void PostProcessingPassRE() {
		ProfilerOpenGLZoneFunction();

		RendererSettings& rendererSettings = Renderer::GetCurrentRendererSettings();

		// Only post process the following modes
		if (rendererSettings.rendererOverrideState == RendererOverrideState::NONE || // This means the final lit image
			rendererSettings.rendererOverrideState == RendererOverrideState::CAMERA_NDOTL ||
			rendererSettings.rendererOverrideState == RendererOverrideState::INDIRECT_DIFFUSE) {

			OpenGLFrameBuffer& gBufferRE = GetFrameBuffer("GBufferRE");

			BindShader("PostProcessing");
			BindImageTexture(0, gBufferRE.GetColorAttachmentHandleByName("Lighting"), GL_READ_WRITE, GL_RGBA16F);

			glDispatchCompute(gBufferRE.GetWidth() / 8, gBufferRE.GetHeight() / 8, 1);
		}
	}
}