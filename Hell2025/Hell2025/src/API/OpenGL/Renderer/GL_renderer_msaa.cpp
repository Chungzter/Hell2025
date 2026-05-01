#include "GL_renderer.h"
#include "API/OpenGL/GL_backEnd.h"
#include "Config/Config.h"
#include "Renderer/Renderer.h"
#include "Renderer/RenderDataManager.h"
#include "Viewport/ViewportManager.h"

// remove me
#include "World/World.h"
#include "AssetManagement/AssetManager.h"

namespace OpenGLRenderer {

    void ClearRenderTargetsMSAA();
	void CompositeLighting();
    void DepthPrePass();
    void MSAAResolve();
    void ShadedGeometry();
    void PostProcessingPassMSAA();

    void MultiDrawScene();                    // would these be good?
    void MultiDrawSkinnedScene();             // would these be good?
    void MultiDrawSkinnedNonDeformingScene(); // would these be good?

    void MultiDrawPerViewport(OpenGLFrameBuffer * fbo, OpenGLShader* shader, const std::vector<DrawIndexedIndirectCommand> drawCommands[4], OpenGLRasterizerState& rasterizerState);

    void InitMSAA() {
        const Resolutions& resolutions = Config::GetResolutions();

		OpenGLFrameBuffer& mssaFbo = CreateMultisampledFrameBuffer("MSAA", resolutions.gBuffer, 4);
		mssaFbo.CreateAttachment("Lighting", GL_RGBA16F);
		mssaFbo.CreateAttachment("Normal", GL_RGB10_A2);
		mssaFbo.CreateAttachment("Material", GL_RGBA8);
        mssaFbo.CreateDepthAttachment(GL_DEPTH_COMPONENT32F);

        OpenGLFrameBuffer& resolveFbo = CreateFrameBuffer("Resolve", mssaFbo.GetWidth(), mssaFbo.GetHeight());
		resolveFbo.CreateAttachment("Color", GL_RGBA16F);
		resolveFbo.CreateAttachment("Normal", GL_RGB10_A2);
		resolveFbo.CreateAttachment("Material", GL_RGBA8);
		resolveFbo.CreateDepthAttachment(GL_DEPTH_COMPONENT32F);

		LoadShader("MSAA", "DepthPrePass", { "GL_depth_prepass.vert", "GL_depth_prepass.frag" });
		LoadShader("MSAA", "DepthPrePassAlphaDiscard", { "GL_depth_prepass_alpha_discard.vert", "GL_depth_prepass_alpha_discard.frag" });
		LoadShader("MSAA", "ShadedHardSurface", { "GL_shaded.vert", "GL_shaded.frag" });
		LoadShader("MSAA", "ShadedHair", { "GL_shaded.vert", "GL_shaded_hair.frag" });
    }

    void RenderGameMSAA() {
        ProfilerOpenGLFrame();
		glDisable(GL_DITHER);
		glDisable(GL_SAMPLE_SHADING);
        glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE); // TODO: not do this every frame

        OpenGLFrameBuffer* finalImageFbo = GetFrameBuffer("FinalImage");
        OpenGLFrameBuffer* resolveFbo = GetFrameBuffer("Resolve");
        OpenGLFrameBuffer* msaaFbo = GetFrameBuffer("MSAA");

		ComputeSkinningPass();
		UpdateSSBOS();
		RenderShadowMaps();
        ClearRenderTargetsMSAA();

		glBindVertexArray(OpenGLBackEnd::GetWeightedVertexDataVAO());
		glBindBuffer(GL_ARRAY_BUFFER, OpenGLBackEnd::GetWeightedVertexDataVBO());
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, OpenGLBackEnd::GetWeightedVertexDataEBO());

		glBindVertexArray(OpenGLBackEnd::GetSkinnedVertexDataVAO());
		glBindBuffer(GL_ARRAY_BUFFER, OpenGLBackEnd::GetSkinnedVertexDataVBO());
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, OpenGLBackEnd::GetWeightedVertexDataEBO());

		glBindVertexArray(0);

        DepthPrePass();


        ShadedGeometry();
        MSAAResolve();

        UpdateGlobalIllumintation();

        BindSSBO("Samplers", 0);
        BindSSBO("RendererData", 1);
        BindSSBO("ViewportData", 2);
        BindSSBO("InstanceData", 3);
        BindSSBO("Lights", 4);

        CompositeLighting();

		// DDGI Debug
		DDGIVolume& ddgiVolume = World::GetTestDDGIVolume();
		if (Renderer::GetCurrentRendererSettings().debugDrawPointCloud)       DrawPointCloud(ddgiVolume);
		if (Renderer::GetCurrentRendererSettings().debugDrawPointCloudGrid)   DrawPointCloudGrid(ddgiVolume);
		if (Renderer::GetCurrentRendererSettings().debugDrawIrradianceProbes) DrawProbes(ddgiVolume);

        // Resolve lighting
        OpenGLRenderer::BlitFrameBuffer(msaaFbo, resolveFbo, "Lighting", "Color", GL_COLOR_BUFFER_BIT, GL_LINEAR);

		DebugViewPass();
		PostProcessingPassMSAA();

        // Downscale blit
        OpenGLRenderer::BlitFrameBuffer(resolveFbo, finalImageFbo, "Color", "Color", GL_COLOR_BUFFER_BIT, GL_LINEAR);

        // Blit to swapchain
        OpenGLRenderer::BlitToDefaultFrameBuffer(finalImageFbo, "Color", GL_COLOR_BUFFER_BIT, GL_NEAREST);

        glDisable(GL_CULL_FACE); // Must be disabled before UI pass

        UIPass();
        ImGuiPass();
    }


    void ClearRenderTargetsMSAA() {
		OpenGLRasterizerState state;
		state.depthMask = true;
		state.colorMask = true;
		ForceRasterizerState(state);

        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glDepthMask(GL_TRUE);

        OpenGLFrameBuffer* msaaFbo = GetFrameBuffer("MSAA");
		msaaFbo->Bind();
		msaaFbo->ClearAttachment("Lighting", 0.0f, 0.0f, 0.0f, 1.0f);
		msaaFbo->ClearAttachment("Normal", 0.0f, 0.0f, 0.0f, 1.0f);
		msaaFbo->ClearAttachment("Material", 0.0f, 0.0f, 0.0f, 0.0f);
        msaaFbo->ClearDepthAttachment(0.0f);
    }

	void DepthPrePass() {
		ProfilerOpenGLZoneFunction();

		const DrawCommandsSet& drawInfoSet = RenderDataManager::GetDrawInfoSet();

		OpenGLFrameBuffer* msaaFbo = GetFrameBuffer("MSAA");
		msaaFbo->Bind();
		msaaFbo->DrawBuffer(GL_NONE);

		OpenGLRasterizerState opaqueDepthState;
		opaqueDepthState.depthTestEnabled = true;
		opaqueDepthState.blendEnable = false;
		opaqueDepthState.cullfaceEnable = true;
		opaqueDepthState.depthMask = true;
		opaqueDepthState.colorMask = true;
		opaqueDepthState.depthFunc = GL_GREATER;

		OpenGLRasterizerState maskedDepthState = opaqueDepthState;
		maskedDepthState.cullfaceEnable = false;

		// Opaque pass
		OpenGLShader* opaqueShader = GetShader("DepthPrePass");
		opaqueShader->Bind();

		glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
		MultiDrawPerViewport(msaaFbo, opaqueShader, drawInfoSet.standard, opaqueDepthState);

		glBindVertexArray(OpenGLBackEnd::GetWeightedVertexDataVAO());
		MultiDrawPerViewport(msaaFbo, opaqueShader, drawInfoSet.skinnedNonDeformingStandard, opaqueDepthState);

		glBindVertexArray(OpenGLBackEnd::GetSkinnedVertexDataVAO());
		MultiDrawPerViewport(msaaFbo, opaqueShader, drawInfoSet.skinnedStandard, opaqueDepthState);

		glBindVertexArray(World::GetHouseMeshBuffer().GetGLMeshBuffer().GetVAO());
		MultiDrawPerViewport(msaaFbo, opaqueShader, drawInfoSet.house, opaqueDepthState);

		// Masked pass
		//glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);

		OpenGLShader* maskedShader = GetShader("DepthPrePassAlphaDiscard");
		maskedShader->Bind();

		glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
		MultiDrawPerViewport(msaaFbo, maskedShader, drawInfoSet.alphaDiscard, maskedDepthState);
		MultiDrawPerViewport(msaaFbo, maskedShader, drawInfoSet.hair, maskedDepthState);

		glBindVertexArray(OpenGLBackEnd::GetWeightedVertexDataVAO());
		MultiDrawPerViewport(msaaFbo, maskedShader, drawInfoSet.skinnedNonDeformingAlphaDiscard, maskedDepthState);
		MultiDrawPerViewport(msaaFbo, maskedShader, drawInfoSet.skinnedNonDeformingHair, maskedDepthState);

		glBindVertexArray(OpenGLBackEnd::GetSkinnedVertexDataVAO());
		MultiDrawPerViewport(msaaFbo, maskedShader, drawInfoSet.skinnedAlphaDiscard, maskedDepthState);
		MultiDrawPerViewport(msaaFbo, maskedShader, drawInfoSet.skinnedHair, maskedDepthState);

		//glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
		glBindVertexArray(0);
	}

    void ShadedGeometry() {
		ProfilerOpenGLZoneFunction();

		const DrawCommandsSet& drawInfoSet = RenderDataManager::GetDrawInfoSet();

		OpenGLFrameBuffer* msaaFbo = GetFrameBuffer("MSAA");
		msaaFbo->Bind();
		msaaFbo->DrawBuffers({ "Lighting", "Normal", "Material" });

		OpenGLRasterizerState opaqueState;
		opaqueState.blendEnable = false;
		opaqueState.cullfaceEnable = true;
		opaqueState.colorMask = true;
		opaqueState.depthFunc = GL_EQUAL;
		opaqueState.depthMask = false;
		opaqueState.depthTestEnabled = true;

		OpenGLRasterizerState maskedState = opaqueState;
		maskedState.cullfaceEnable = false;

		OpenGLRasterizerState blendedState = opaqueState;
		blendedState.blendEnable = true;
		blendedState.blendFuncSrcfactor = GL_SRC_ALPHA;
		blendedState.blendFuncDstfactor = GL_ONE_MINUS_SRC_ALPHA;
		blendedState.cullfaceEnable = false;
		blendedState.depthFunc = GL_GEQUAL;
		blendedState.depthMask = false;
		blendedState.depthTestEnabled = true;
		
		OpenGLShader* hairShader = GetShader("ShadedHair");
		OpenGLShader* hardSurfaeShader = GetShader("ShadedHardSurface");

		OpenGLShadowMap* flashLightShadowMapsFBO = GetShadowMap("FlashlightShadowMaps");
		OpenGLShadowCubeMapArray* hiResShadowMaps = GetShadowCubeMapArray("HiRes");
		OpenGLShadowMapArray* shadowMapArray = GetShadowMapArray("MoonlightCSM");

		glBindTextureUnit(7, AssetManager::GetTextureByName("Flashlight2")->GetGLTexture().GetHandle());
		glBindTextureUnit(8, flashLightShadowMapsFBO->GetDepthTextureHandle());

		glActiveTexture(GL_TEXTURE9);
		glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, hiResShadowMaps->GetDepthTexture());

		glActiveTexture(GL_TEXTURE10);
		glBindTexture(GL_TEXTURE_2D_ARRAY, shadowMapArray->GetDepthTexture());

		glBindTextureUnit(11, AssetManager::GetTextureByName("RatKingHair_HAIR_FLOW")->GetGLTexture().GetHandle());
		glBindTextureUnit(12, AssetManager::GetTextureByName("RatKingHair_HAIR_ID")->GetGLTexture().GetHandle());
		glBindTextureUnit(13, AssetManager::GetTextureByName("RatKingHair_HAIR_ROOT")->GetGLTexture().GetHandle());

		// Opaque and masked passes
		hardSurfaeShader->Bind();

		glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
		MultiDrawPerViewport(msaaFbo, hardSurfaeShader, drawInfoSet.standard, opaqueState);
		MultiDrawPerViewport(msaaFbo, hardSurfaeShader, drawInfoSet.alphaDiscard, maskedState);
		MultiDrawPerViewport(msaaFbo, hardSurfaeShader, drawInfoSet.hair, maskedState);

		glBindVertexArray(OpenGLBackEnd::GetWeightedVertexDataVAO());
		MultiDrawPerViewport(msaaFbo, hardSurfaeShader, drawInfoSet.skinnedNonDeformingStandard, opaqueState);
		MultiDrawPerViewport(msaaFbo, hardSurfaeShader, drawInfoSet.skinnedNonDeformingAlphaDiscard, maskedState);
		MultiDrawPerViewport(msaaFbo, hardSurfaeShader, drawInfoSet.skinnedNonDeformingHair, maskedState);

		glBindVertexArray(OpenGLBackEnd::GetSkinnedVertexDataVAO());
		MultiDrawPerViewport(msaaFbo, hardSurfaeShader, drawInfoSet.skinnedStandard, opaqueState);
		MultiDrawPerViewport(msaaFbo, hardSurfaeShader, drawInfoSet.skinnedAlphaDiscard, maskedState);
		MultiDrawPerViewport(msaaFbo, hardSurfaeShader, drawInfoSet.skinnedHair, maskedState);

		glBindVertexArray(World::GetHouseMeshBuffer().GetGLMeshBuffer().GetVAO());
		MultiDrawPerViewport(msaaFbo, hardSurfaeShader, drawInfoSet.house, opaqueState);

		// Hair passes
		hairShader->Bind();

		glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
		MultiDrawPerViewport(msaaFbo, hairShader, drawInfoSet.hair, maskedState);

		glBindVertexArray(OpenGLBackEnd::GetWeightedVertexDataVAO());
		MultiDrawPerViewport(msaaFbo, hairShader, drawInfoSet.skinnedNonDeformingHair, maskedState);

		glBindVertexArray(OpenGLBackEnd::GetSkinnedVertexDataVAO());
		MultiDrawPerViewport(msaaFbo, hairShader, drawInfoSet.skinnedHair, maskedState);

		// Blended passes
		hardSurfaeShader->Bind();

		glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
		MultiDrawPerViewport(msaaFbo, hardSurfaeShader, drawInfoSet.blended, blendedState);

		glBindVertexArray(OpenGLBackEnd::GetWeightedVertexDataVAO());
		MultiDrawPerViewport(msaaFbo, hardSurfaeShader, drawInfoSet.skinnedNonDeformingBlended, blendedState);

		glBindVertexArray(OpenGLBackEnd::GetSkinnedVertexDataVAO());
		MultiDrawPerViewport(msaaFbo, hardSurfaeShader, drawInfoSet.skinnedBlended, blendedState);

		glBindVertexArray(0);
    }

    void MSAAResolve() {
        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer* msaaFbo = GetFrameBuffer("MSAA");
        OpenGLFrameBuffer* resolveFbo = GetFrameBuffer("Resolve");

        if (!msaaFbo) return;
        if (!resolveFbo) return;

		OpenGLRenderer::BlitFrameBuffer(msaaFbo, resolveFbo, "Normal", "Normal", GL_COLOR_BUFFER_BIT, GL_LINEAR);
	}

	void CompositeLighting() {
		// TODO: Add indirect lighting here
		// TODO: Add emissive lighting here
	}

	void PostProcessingPassMSAA() {
		ProfilerOpenGLZoneFunction();

		RendererSettings& rendererSettings = Renderer::GetCurrentRendererSettings();

		// Only post process the following modes
		if (rendererSettings.rendererOverrideState == RendererOverrideState::NONE || // This means the final lit image
			rendererSettings.rendererOverrideState == RendererOverrideState::CAMERA_NDOTL ||
			rendererSettings.rendererOverrideState == RendererOverrideState::INDIRECT_DIFFUSE) {

			OpenGLFrameBuffer* fbo = GetFrameBuffer("Resolve");
			OpenGLFrameBuffer* gBuffer = GetFrameBuffer("GBuffer");
			OpenGLFrameBuffer* msaaFbo = GetFrameBuffer("MSAA");
			OpenGLShader* shader = GetShader("PostProcessing");

			if (!fbo) return;
			if (!shader) return;

			shader->Bind();
			shader->SetBool("u_msaaRenderer", true);

			glBindImageTexture(0, fbo->GetColorAttachmentHandleByName("Color"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);

			BindTextureUnit(3, gBuffer->GetColorAttachmentHandleByName("WorldPosition"));
			BindTextureUnit(4, gBuffer->GetDepthAttachmentHandle());
			BindTextureUnit(5, msaaFbo->GetDepthAttachmentHandle());
			BindTextureUnit(6, msaaFbo->GetColorAttachmentHandleByName("Normal"));
			BindTextureUnit(7, msaaFbo->GetColorAttachmentHandleByName("Material"));

			glDispatchCompute((fbo->GetWidth() + 7) / 8, (fbo->GetHeight() + 7) / 8, 1);
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		}
	}

    void MultiDrawPerViewport(OpenGLFrameBuffer* fbo, OpenGLShader* shader, const std::vector<DrawIndexedIndirectCommand> drawCommands[4], OpenGLRasterizerState& rasterizerState) {
		SetRasterizerState(rasterizerState);

        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (viewport->IsVisible()) {
                OpenGLRenderer::SetViewport(fbo, viewport);
                if (BackEnd::RenderDocFound()) {
                    SplitMultiDrawIndirect(shader, drawCommands[i], true, false);
                }
                else {
                    MultiDrawIndirect(drawCommands[i]);
                }
            }
        }
    }
}