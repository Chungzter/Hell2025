#include "GL_renderer.h"
#include "API/OpenGL/GL_backEnd.h"
#include "AssetManagement/AssetManager.h"
#include "Renderer/RenderDataManager.h"
#include "Renderer/Renderer.h"
#include "World/World.h"
#include "Viewport/ViewportManager.h"

#include "Hell/RendereringConstants.h"

namespace OpenGLRenderer {
    struct RESettings {
        glm::ivec2 gBufferResolution = glm::ivec2(1920, 1080);
        glm::ivec2 finalImageResolution = glm::ivec2(1920, 1080) / 2;
    } g_settings;

    void BindShadowMapsRE();
    void BlendedLighting();
    void ClearRenderTargetsRE();
    void CreateFramebuffersRE();
    void HairDepthPrep();
    void HairDepthPrePassRE();
    void HairPassRE();
    void HairCompositeRE();
    void LightingPassRE();
    void LoadShadersRE();

    void RenderFullscreenTriangle();

    void EmissiveForwardPass();

    void PlebRenderProceduralGeometryIntoGBuffer();

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

        static bool visibilityPath = true;
        if (Input::KeyPressed(HELL_KEY_NUMPAD_2)) {
            visibilityPath = !visibilityPath;
        }

        VisibilityPass();
        VisibilitySkinnedPass();
        VisibilityAlphaDiscardPass();

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

        HairDepthPrep();
        HairDepthPrePassRE();
        HairPassRE();
        HairCompositeRE();

        // DDGI Debug
        DDGIVolume& ddgiVolume = World::GetTestDDGIVolume();
        if (Renderer::GetCurrentRendererSettings().debugDrawPointCloud)       DrawPointCloud(ddgiVolume);
        if (Renderer::GetCurrentRendererSettings().debugDrawPointCloudGrid)   DrawPointCloudGrid(ddgiVolume);
        if (Renderer::GetCurrentRendererSettings().debugDrawIrradianceProbes) DrawProbes(ddgiVolume);

        EmissivePass();
        SpriteSheetPass(); // Muzzle flash, etc

        PostProcessingPassRE();
        DebugViewPass();

        // Downscale and blit to swapchain
        OpenGLFrameBuffer& finalImageFbo = GetFrameBuffer("FinalImage");
        OpenGLFrameBuffer& gBufferRE = GetFrameBuffer("GBufferRE");
        OpenGLRenderer::BlitFrameBuffer(&gBufferRE, &finalImageFbo, "Lighting", "Color", GL_COLOR_BUFFER_BIT, GL_LINEAR);
        OpenGLRenderer::BlitToDefaultFrameBuffer(&finalImageFbo, "Color", GL_COLOR_BUFFER_BIT, GL_NEAREST);

        UIPass();
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

    void CreateFramebuffersRE() {
        OpenGLFrameBuffer& gBufferRE = CreateFrameBuffer("GBufferRE", g_settings.gBufferResolution);
        gBufferRE.CreateAttachment("Lighting", GL_RGBA16F);
        gBufferRE.CreateAttachment("BaseColorMetallic", GL_RGBA8);
        gBufferRE.CreateAttachment("NormalXYRoughnessMisc", GL_RGB10_A2);
        gBufferRE.CreateAttachment("VelocityXYOcclusionSubSurface", GL_RGBA16F);
        gBufferRE.CreateAttachment("Visibility", GL_RG32UI);
        gBufferRE.CreateAttachment("Emissive", GL_RGBA8);
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

        LoadShader("RE", "HairLightingForward", { "GL_hair_lighting_forward.vert", "GL_hair_lighting_forward.frag" });
        // LoadShader("RE", "HairLightingDeferred", { "GL_fullscreen_triangle.vert", "GL_hair_lighting_deferred.frag" });
        LoadShader("RE", "HairCompositeRE", { "GL_hair_composite_re.comp" });
        LoadShader("RE", "HairDepthPrep", { "GL_fullscreen_triangle.vert", "GL_hair_depth_prep.frag" });

        LoadShader("RE", "Visibility", { "GL_visibility.vert", "GL_visibility.frag" });
        LoadShader("RE", "VisibilityAlphaDiscard", { "GL_visibility.vert", "GL_visibility_alpha_discard.frag" });
        LoadShader("RE", "MaterialResolve", { "GL_material_resolve.vert", "GL_material_resolve.frag" });
        LoadShader("RE", "MaterialResolveSkinning", { "GL_material_resolve.vert", "GL_material_resolve.frag" }, { "SKINNED" });

        // TODO: using GL_gbuffer_re.vert is confusing, you probably have a lot of indentical shaders that peform
        //       what this does, check that, and think of a more unified name.
        LoadShader("RE", "EmissiveForward", { "GL_gbuffer_re.vert", "GL_emissive_forward.frag" });
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
        ProfilerOpenGLZoneFunction();

		OpenGLFrameBuffer& gBuffer = GetFrameBuffer("GBufferRE");
		OpenGLFrameBuffer& indirectDiffuseFbo = GetFrameBuffer("IndirectDiffuse");

		BindShader("LightingRE");
		BindShadowMapsRE();

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

		//glBindVertexArray(OpenGLBackEnd::GetWeightedVertexDataVAO());
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

	void HairPassRE() {
		ProfilerOpenGLZoneFunction();
		const DrawCommandsSet& drawInfoSet = RenderDataManager::GetDrawInfoSet();

		OpenGLFrameBuffer& fbo = GetFrameBuffer("HairRE");
		fbo.Bind();
		fbo.DrawBuffers({ "Lighting" });

		OpenGLShader& shader = GetShader("HairLightingForward");
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
		MultiDrawPerViewport(&fbo, &shader, drawInfoSet.skinnedNonDeformingHair, maskedState);

		glBindVertexArray(OpenGLBackEnd::GetSkinnedVertexDataVAO());
		MultiDrawPerViewport(&fbo, &shader, drawInfoSet.skinnedHair, maskedState);
	}

    void HairCompositeRE() {
        ProfilerOpenGLZoneFunction();

		OpenGLFrameBuffer& gBuffer = GetFrameBuffer("GBufferRE");
		OpenGLFrameBuffer& hairFbo = GetFrameBuffer("HairRE");

		BindShader("HairCompositeRE");

		BindImageTexture(0, gBuffer.GetColorAttachmentHandleByName("Lighting"), GL_READ_WRITE, GL_RGBA16F);
		BindTextureUnit(1, hairFbo.GetColorAttachmentHandleByName("Lighting"));

		glDispatchCompute(gBuffer.GetWidth() / TILE_SIZE, gBuffer.GetHeight() / TILE_SIZE, 1);
	}

    void RenderFullscreenTriangle() {
        glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }
}