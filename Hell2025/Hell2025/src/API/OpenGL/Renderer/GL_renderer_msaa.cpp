#include "GL_renderer.h"
#include "API/OpenGL/GL_backEnd.h"
#include "Config/Config.h"
#include "Renderer/Renderer.h"
#include "Renderer/RenderDataManager.h"
#include "Viewport/ViewportManager.h"

// remove me
#include "World/World.h"
#include "AssetManagement/AssetManager.h"
#include "Core/Game.h"

namespace OpenGLRenderer {

    struct MSAASettings {
        glm::ivec2 msaaTargetResolution = glm::ivec2(1920, 1080);
        glm::ivec2 finalImageResolution = glm::ivec2(1920, 1080) / 2;
        uint32_t g_mssaSampleCount = 4;
    } g_settings;


    bool hairOG = true;
    uint32_t g_mlabFrameIndex = 0;

    void BindShadowMaps();
    void ClearRenderTargetsMSAA();
    void CreateFramebuffersMSAA();
    void CompositeLighting();
    void DepthPrePass();
    void HairHalfResDepthPrePass();
    void HairHalfResShading();
    void LoadShadersMSAA();
    void HairMLAB();
    void ResolveDepthNormalsMaterials();
    void ResolveLighting();
    void ShadingOpaque();
    void ShadingAlphaDiscard();
    void ShadingHair();
    void ShadingBlended();
    void PostProcessingPassMSAA();

    void HairMLABSolid();
    void HairMLAB();
    void ResolveHairMLAB();

    void MultiDrawPerViewport(OpenGLFrameBuffer * fbo, OpenGLShader* shader, const std::vector<DrawIndexedIndirectCommand> drawCommands[4], OpenGLRasterizerState& rasterizerState);

    void InitMSAA() {
        CreateFramebuffersMSAA();
        LoadShadersMSAA();
    }

    void CreateFramebuffersMSAA() {
        OpenGLFrameBuffer& mssaFbo = CreateMultisampledFrameBuffer("MSAA", g_settings.msaaTargetResolution, g_settings.g_mssaSampleCount);
        mssaFbo.CreateAttachment("Lighting", GL_RGBA16F);
        mssaFbo.CreateAttachment("BaseColor", GL_RGBA8);
        mssaFbo.CreateAttachment("Normal", GL_RGB10_A2);
        mssaFbo.CreateAttachment("RENormal", GL_RGB10_A2);
        mssaFbo.CreateAttachment("Material", GL_RGBA8);
        mssaFbo.CreateDepthAttachment(GL_DEPTH_COMPONENT32F);

        OpenGLFrameBuffer& resolveFbo = CreateFrameBuffer("Resolve", g_settings.msaaTargetResolution);
        resolveFbo.CreateAttachment("Lighting", GL_RGBA16F);
        resolveFbo.CreateAttachment("BaseColor", GL_RGBA8);
        resolveFbo.CreateAttachment("Normal", GL_RGB10_A2);
        resolveFbo.CreateAttachment("Material", GL_RGBA8);
        resolveFbo.CreateAttachment("HairMLAB", GL_RGBA16F);
        resolveFbo.CreateDepthAttachment(GL_DEPTH_COMPONENT32F);

        OpenGLFrameBuffer& halfResFbo = CreateFrameBuffer("HalfRes", g_settings.msaaTargetResolution / 2);
        halfResFbo.CreateAttachment("HairMLAB", GL_RGBA16F);
        halfResFbo.CreateAttachment("HairTest", GL_RGBA16F);
        halfResFbo.CreateDepthAttachment(GL_DEPTH_COMPONENT32F);

        OpenGLFrameBuffer& hairFbo = CreateMultisampledFrameBuffer("HairMSAA", g_settings.msaaTargetResolution / 2, 4);
        hairFbo.CreateAttachment("Lighting", GL_RGBA16F);
        hairFbo.CreateAttachment("BaseColor", GL_RGBA8);
        hairFbo.CreateAttachment("Normal", GL_RGB10_A2);
        hairFbo.CreateAttachment("Material", GL_RGBA8);
        hairFbo.CreateDepthAttachment(GL_DEPTH_COMPONENT32F);

        struct MlabNode {
            uint32_t color;
            float depth;
        };

        int bucketCount = 4;
        int size = sizeof(MlabNode) * resolveFbo.GetWidth() * resolveFbo.GetHeight() * bucketCount * 2;

        //CreateSSBO("HairMLABNodes", size, GL_DYNAMIC_STORAGE_BIT);
    }

    void LoadShadersMSAA() {
        LoadShader("MSAA", "DepthPrePass", { "GL_depth_prepass.vert", "GL_depth_prepass.frag" });
        LoadShader("MSAA", "DepthPrePassAlphaDiscard", { "GL_depth_prepass_alpha_discard.vert", "GL_depth_prepass_alpha_discard.frag" });
        LoadShader("MSAA", "LightingComposite", { "GL_lighting_composite.comp" });
        LoadShader("MSAA", "ShadedHardSurface", { "GL_shaded.vert", "GL_shaded.frag" });
        LoadShader("MSAA", "ShadedHair", { "GL_shaded.vert", "GL_shaded_hair.frag" });
        //LoadShader("MSAA", "HairMLAB", { "GL_hair_mlab.vert", "GL_hair_mlab.frag" });
        LoadShader("MSAA", "HairMLABResolve", { "GL_hair_mlab_resolve.vert", "GL_hair_mlab_resolve.frag" });
    }

    void RenderGameMSAA() {
        ProfilerOpenGLFrame();

        OpenGLRasterizerState defaultState;
        defaultState.colorMask = true;
        defaultState.depthMask = true;
        SetRasterizerState(defaultState);

        if (g_mlabFrameIndex == 0) {
            ClearSSBO("HairMLABNodes");
        }
        g_mlabFrameIndex++;

        glDisable(GL_DITHER);
        glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
        glDisable(GL_SAMPLE_ALPHA_TO_ONE);
        glDisable(GL_SAMPLE_COVERAGE);
        glDisable(GL_SAMPLE_SHADING);
        glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE); // TODO: not do this every frame

        if (Input::KeyPressed(HELL_KEY_NUMPAD_6)) {
            hairOG = !hairOG;
            Audio::PlayAudio(AUDIO_SELECT, 1.00f);
        }

        if (Input::KeyPressed(HELL_KEY_NUMPAD_8)) {
            Player* player = Game::GetLocalPlayerByIndex(0);
            player->SetFootPosition(glm::vec3(36.53f, 31.0f, 36.36f));
            player->GetCamera().SetEulerRotation(glm::vec3(-0.06f, 5.17f, 0.0f));
            Audio::PlayAudio(AUDIO_SELECT, 1.00f);
        }

        if (Input::KeyPressed(HELL_KEY_NUMPAD_1)) {
            g_settings.msaaTargetResolution = glm::ivec2(1920, 1080);
            g_settings.g_mssaSampleCount = 4;
            CreateFramebuffersMSAA();
            Audio::PlayAudio(AUDIO_SELECT, 1.00f);
        }
        if (Input::KeyPressed(HELL_KEY_NUMPAD_2)) {
            g_settings.msaaTargetResolution = glm::ivec2(1920, 1080) / 2;
            g_settings.g_mssaSampleCount = 4;
            CreateFramebuffersMSAA();
            Audio::PlayAudio(AUDIO_SELECT, 1.00f);
        }
        if (Input::KeyPressed(HELL_KEY_NUMPAD_3)) {
            g_settings.msaaTargetResolution = glm::ivec2(1920, 1080) / 2;
            g_settings.g_mssaSampleCount = 8;
            CreateFramebuffersMSAA();
            Audio::PlayAudio(AUDIO_SELECT, 1.00f);
        }
        if (Input::KeyPressed(HELL_KEY_NUMPAD_4)) {
            g_settings.msaaTargetResolution = glm::ivec2(1920, 1080) / 2;
            g_settings.g_mssaSampleCount = 16;
            CreateFramebuffersMSAA();
            Audio::PlayAudio(AUDIO_SELECT, 1.00f);
        }

        OpenGLFrameBuffer* finalImageFbo = GetFrameBufferOLD("FinalImage");
        OpenGLFrameBuffer* resolveFbo = GetFrameBufferOLD("Resolve");
        OpenGLFrameBuffer* msaaFbo = GetFrameBufferOLD("MSAA");

        ComputeSkinningPass();
        UpdateSSBOS();
        RenderShadowMaps();
        ClearRenderTargetsMSAA();

        //glBindVertexArray(OpenGLBackEnd::GetWeightedVertexDataVAO());
        //glBindBuffer(GL_ARRAY_BUFFER, OpenGLBackEnd::GetWeightedVertexDataVBO());
        //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, OpenGLBackEnd::GetWeightedVertexDataEBO());

        glBindVertexArray(OpenGLBackEnd::GetSkinnedVertexDataVAO());
        glBindBuffer(GL_ARRAY_BUFFER, OpenGLBackEnd::GetSkinnedVertexDataVBO());
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, OpenGLBackEnd::GetVertexDataEBO());

        //glBindVertexArray(OpenGLBackEnd::GetWeightedVertexDataVAO());
        //glBindBuffer(GL_ARRAY_BUFFER, OpenGLBackEnd::GetWeightedVertexDataVBO());
        //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, OpenGLBackEnd::GetWeightedVertexDataEBO());
        //
        //glBindVertexArray(OpenGLBackEnd::GetSkinnedVertexDataVAO());
        //glBindBuffer(GL_ARRAY_BUFFER, OpenGLBackEnd::GetSkinnedVertexDataVBO());
        //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, OpenGLBackEnd::GetWeightedVertexDataEBO());

        glBindVertexArray(0);

        DepthPrePass();
        ShadingOpaque();
        ShadingAlphaDiscard();
        if (hairOG) ShadingHair();
        ShadingBlended();

		ResolveDepthNormalsMaterials();


        SetRasterizerState(defaultState);

        //UpdateGlobalIllumintation();

        BindSSBO(0, "Samplers");
        BindSSBO(1, "RendererData");
        BindSSBO(2, "ViewportData");
        BindSSBO(3, "InstanceData");
        BindSSBO(4, "Lights");

		ResolveLighting();

        //if (!hairOG) HairMLABSolid();
        //if (!hairOG) HairMLAB();
        //if (!hairOG) ResolveHairMLAB();

        if (!hairOG) HairHalfResDepthPrePass();
        if (!hairOG) HairHalfResShading();

        CompositeLighting();

        // DDGI Debug
        DDGIVolume& ddgiVolume = World::GetTestDDGIVolume();
        if (Renderer::GetCurrentRendererSettings().debugDrawPointCloud)       DrawPointCloud(ddgiVolume);
        if (Renderer::GetCurrentRendererSettings().debugDrawPointCloudGrid)   DrawPointCloudGrid(ddgiVolume);
        if (Renderer::GetCurrentRendererSettings().debugDrawIrradianceProbes) DrawProbes(ddgiVolume);

        PostProcessingPassMSAA();
		DebugViewPass();

        // Downscale blit
        OpenGLRenderer::BlitFrameBuffer(resolveFbo, finalImageFbo, "Lighting", "Color", GL_COLOR_BUFFER_BIT, GL_LINEAR);

        // Blit to swapchain
        OpenGLRenderer::BlitToDefaultFrameBuffer(finalImageFbo, "Color", GL_COLOR_BUFFER_BIT, GL_NEAREST);

        UIPass();
    }

    void ClearRenderTargetsMSAA() {
		OpenGLRasterizerState state;
		state.depthMask = true;
		state.colorMask = true;
		ForceRasterizerState(state);

        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glDepthMask(GL_TRUE);

        OpenGLFrameBuffer& msaaFbo = GetFrameBuffer("MSAA");
		msaaFbo.Bind();
        msaaFbo.ClearAttachment("BaseColor", 0.0f, 0.0f, 0.0f, 0.0f);
        msaaFbo.ClearAttachment("Normal", 0.0f, 0.0f, 0.0f, 0.0f);
        msaaFbo.ClearAttachment("RENormal", 0.0f, 0.0f, 0.0f, 0.0f);
        msaaFbo.ClearAttachment("Material", 0.0f, 0.0f, 0.0f, 0.0f);
        msaaFbo.ClearAttachment("Lighting", 0.0f, 0.0f, 0.0f, 0.0f);
        msaaFbo.ClearDepthAttachment(0.0f);

        OpenGLFrameBuffer& hairFbo = GetFrameBuffer("HairMSAA");
        hairFbo.Bind();
        hairFbo.ClearAttachment("BaseColor", 0.0f, 0.0f, 0.0f, 0.0f);
        hairFbo.ClearAttachment("Normal", 0.0f, 0.0f, 0.0f, 0.0f);
        hairFbo.ClearAttachment("Material", 0.0f, 0.0f, 0.0f, 0.0f);
        hairFbo.ClearAttachment("Lighting", 0.0f, 0.0f, 0.0f, 0.0f);
        hairFbo.ClearDepthAttachment(0.0f);

        OpenGLFrameBuffer& resolveFbo = GetFrameBuffer("Resolve");
        resolveFbo.Bind();
        resolveFbo.ClearAttachment("HairMLAB", 0.0f, 0.0f, 0.0f, 0.0f);
    }

	void DepthPrePass() {
		ProfilerOpenGLZoneFunction();

		const DrawCommandsSet& drawInfoSet = RenderDataManager::GetDrawInfoSet();

		OpenGLFrameBuffer* msaaFbo = GetFrameBufferOLD("MSAA");
		msaaFbo->Bind();
		msaaFbo->DrawBuffer(GL_NONE);

		OpenGLRasterizerState opaqueDepthState;
		opaqueDepthState.depthTestEnabled = true;
		opaqueDepthState.blendEnable = false;
		opaqueDepthState.cullfaceEnable = true;
		opaqueDepthState.depthMask = true;
		opaqueDepthState.colorMask = false;
		opaqueDepthState.depthFunc = GL_GREATER;

		OpenGLRasterizerState maskedDepthState = opaqueDepthState;
		maskedDepthState.cullfaceEnable = false;

		// Opaque
		OpenGLShader* opaqueShader = GetShaderOLD("DepthPrePass");
		opaqueShader->Bind();

        glBindVertexArray(World::GetHouseMeshBuffer().GetGLMeshBuffer().GetVAO());
        MultiDrawPerViewport(msaaFbo, opaqueShader, drawInfoSet.house, opaqueDepthState);

		glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
		MultiDrawPerViewport(msaaFbo, opaqueShader, drawInfoSet.standard, opaqueDepthState);

		//glBindVertexArray(OpenGLBackEnd::GetWeightedVertexDataVAO());
		MultiDrawPerViewport(msaaFbo, opaqueShader, drawInfoSet.skinnedNonDeformingStandard, opaqueDepthState);

		glBindVertexArray(OpenGLBackEnd::GetSkinnedVertexDataVAO());
		MultiDrawPerViewport(msaaFbo, opaqueShader, drawInfoSet.skinnedStandard, opaqueDepthState);

        if (!hairOG) return;

		// Masked
		OpenGLShader* maskedShader = GetShaderOLD("DepthPrePassAlphaDiscard");
		maskedShader->Bind();

		glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
		MultiDrawPerViewport(msaaFbo, maskedShader, drawInfoSet.alphaDiscard, maskedDepthState);
		MultiDrawPerViewport(msaaFbo, maskedShader, drawInfoSet.hair, maskedDepthState);

		//glBindVertexArray(OpenGLBackEnd::GetWeightedVertexDataVAO());
		MultiDrawPerViewport(msaaFbo, maskedShader, drawInfoSet.skinnedNonDeformingAlphaDiscard, maskedDepthState);
        MultiDrawPerViewport(msaaFbo, maskedShader, drawInfoSet.skinnedNonDeformingHair, maskedDepthState);

		glBindVertexArray(OpenGLBackEnd::GetSkinnedVertexDataVAO());
		MultiDrawPerViewport(msaaFbo, maskedShader, drawInfoSet.skinnedAlphaDiscard, maskedDepthState);
        MultiDrawPerViewport(msaaFbo, maskedShader, drawInfoSet.skinnedHair, maskedDepthState);

        glBindVertexArray(0);
	}

    void BindShadowMaps() {
        OpenGLShadowMap* flashLightShadowMapsFBO = GetShadowMapOLD("FlashlightShadowMaps");
        OpenGLShadowCubeMapArray* hiResShadowMaps = GetShadowCubeMapArrayOLD("HiRes");
        OpenGLShadowMapArray* shadowMapArray = GetShadowMapArrayOLD("MoonlightCSM");

        glBindTextureUnit(7, AssetManager::GetTextureByName("Flashlight2")->GetGLTexture().GetHandle());
        glBindTextureUnit(8, flashLightShadowMapsFBO->GetDepthTextureHandle());

        glActiveTexture(GL_TEXTURE9);
        glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, hiResShadowMaps->GetDepthTexture());

        glActiveTexture(GL_TEXTURE10);
        glBindTexture(GL_TEXTURE_2D_ARRAY, shadowMapArray->GetDepthTexture());
    }

    void ShadingOpaque() {
		ProfilerOpenGLZoneFunction();

		const DrawCommandsSet& drawInfoSet = RenderDataManager::GetDrawInfoSet();

		OpenGLFrameBuffer& msaaFbo = GetFrameBuffer("MSAA");
		msaaFbo.Bind();
        msaaFbo.DrawBuffers({ "Lighting", "BaseColor", "Normal", "Material", "RENormal" });

        OpenGLShader& shader = GetShader("ShadedHardSurface");
        shader.Bind();

        BindShadowMaps();

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

		glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
		MultiDrawPerViewport(&msaaFbo, &shader, drawInfoSet.standard, opaqueState);
		MultiDrawPerViewport(&msaaFbo, &shader, drawInfoSet.alphaDiscard, maskedState);

		//glBindVertexArray(OpenGLBackEnd::GetWeightedVertexDataVAO());
		MultiDrawPerViewport(&msaaFbo, &shader, drawInfoSet.skinnedNonDeformingStandard, opaqueState);
        MultiDrawPerViewport(&msaaFbo, &shader, drawInfoSet.skinnedNonDeformingAlphaDiscard, maskedState);

		glBindVertexArray(OpenGLBackEnd::GetSkinnedVertexDataVAO());
		MultiDrawPerViewport(&msaaFbo, &shader, drawInfoSet.skinnedStandard, opaqueState);
        MultiDrawPerViewport(&msaaFbo, &shader, drawInfoSet.skinnedAlphaDiscard, maskedState);

		glBindVertexArray(World::GetHouseMeshBuffer().GetGLMeshBuffer().GetVAO());
		MultiDrawPerViewport(&msaaFbo, &shader, drawInfoSet.house, opaqueState);

		glBindVertexArray(0);
    }

    void ShadingAlphaDiscard() {
        ProfilerOpenGLZoneFunction();

        const DrawCommandsSet& drawInfoSet = RenderDataManager::GetDrawInfoSet();

        OpenGLFrameBuffer& msaaFbo = GetFrameBuffer("MSAA");
        msaaFbo.Bind();
        msaaFbo.DrawBuffers({ "Lighting", "BaseColor", "Normal", "Material" });

        OpenGLShader& shader = GetShader("ShadedHardSurface");
        shader.Bind();

        BindShadowMaps();

        OpenGLRasterizerState maskedState;
        maskedState.blendEnable = false;
        maskedState.cullfaceEnable = false;
        maskedState.colorMask = true;
        maskedState.depthFunc = GL_EQUAL;
        maskedState.depthMask = false;
        maskedState.depthTestEnabled = true;

        glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
        MultiDrawPerViewport(&msaaFbo, &shader, drawInfoSet.alphaDiscard, maskedState);

        //glBindVertexArray(OpenGLBackEnd::GetWeightedVertexDataVAO());
        MultiDrawPerViewport(&msaaFbo, &shader, drawInfoSet.skinnedNonDeformingAlphaDiscard, maskedState);

        glBindVertexArray(OpenGLBackEnd::GetSkinnedVertexDataVAO());
        MultiDrawPerViewport(&msaaFbo, &shader, drawInfoSet.skinnedAlphaDiscard, maskedState);

        glBindVertexArray(0);
    }

    void HairHalfResDepthPrePass() {
        ProfilerOpenGLZoneFunction();

        const DrawCommandsSet& drawInfoSet = RenderDataManager::GetDrawInfoSet();

        OpenGLFrameBuffer* msaaFbo = GetFrameBufferOLD("HairMSAA");
        msaaFbo->Bind();
        msaaFbo->DrawBuffer(GL_NONE);
        //msaaFbo->ClearDepthAttachment(0.0f);

        OpenGLRasterizerState opaqueDepthState;
        opaqueDepthState.depthTestEnabled = true;
        opaqueDepthState.blendEnable = false;
        opaqueDepthState.cullfaceEnable = true;
        opaqueDepthState.depthMask = true;
        opaqueDepthState.colorMask = false;
        opaqueDepthState.depthFunc = GL_GREATER;

        OpenGLRasterizerState maskedDepthState = opaqueDepthState;
        maskedDepthState.cullfaceEnable = false;

        // Opaque
        //OpenGLShader* opaqueShader = GetShaderOLD("DepthPrePass");
        //opaqueShader->Bind();
        //
        //glBindVertexArray(World::GetHouseMeshBuffer().GetGLMeshBuffer().GetVAO());
        //MultiDrawPerViewport(msaaFbo, opaqueShader, drawInfoSet.house, opaqueDepthState);
        //
        //glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
        //MultiDrawPerViewport(msaaFbo, opaqueShader, drawInfoSet.standard, opaqueDepthState);
        //
        //glBindVertexArray(OpenGLBackEnd::GetWeightedVertexDataVAO());
        //MultiDrawPerViewport(msaaFbo, opaqueShader, drawInfoSet.skinnedNonDeformingStandard, opaqueDepthState);
        //
        //glBindVertexArray(OpenGLBackEnd::GetSkinnedVertexDataVAO());
        //MultiDrawPerViewport(msaaFbo, opaqueShader, drawInfoSet.skinnedStandard, opaqueDepthState);

        // Masked
        OpenGLShader* maskedShader = GetShaderOLD("DepthPrePassAlphaDiscard");
        maskedShader->Bind();

        glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
        //MultiDrawPerViewport(msaaFbo, maskedShader, drawInfoSet.alphaDiscard, maskedDepthState);
        MultiDrawPerViewport(msaaFbo, maskedShader, drawInfoSet.hair, maskedDepthState);

        //glBindVertexArray(OpenGLBackEnd::GetWeightedVertexDataVAO());
        //MultiDrawPerViewport(msaaFbo, maskedShader, drawInfoSet.skinnedNonDeformingAlphaDiscard, maskedDepthState);
        MultiDrawPerViewport(msaaFbo, maskedShader, drawInfoSet.skinnedNonDeformingHair, maskedDepthState);

        glBindVertexArray(OpenGLBackEnd::GetSkinnedVertexDataVAO());
        //MultiDrawPerViewport(msaaFbo, maskedShader, drawInfoSet.skinnedAlphaDiscard, maskedDepthState);
        MultiDrawPerViewport(msaaFbo, maskedShader, drawInfoSet.skinnedHair, maskedDepthState);

        glBindVertexArray(0);
    }

    void HairHalfResShading() {
        ProfilerOpenGLZoneFunction();

        const DrawCommandsSet& drawInfoSet = RenderDataManager::GetDrawInfoSet();

        OpenGLFrameBuffer& msaaFbo = GetFrameBuffer("HairMSAA");
        msaaFbo.Bind();
        msaaFbo.DrawBuffers({ "Lighting", "BaseColor", "Normal", "Material" });

        OpenGLShader& shader = GetShader("ShadedHair");
        shader.Bind();
        shader.SetFloat("u_renderResolutionScale", 0.5f);

        BindShadowMaps();

        OpenGLRasterizerState maskedState;
        maskedState.blendEnable = false;
        maskedState.cullfaceEnable = false;
        maskedState.colorMask = true;
        maskedState.depthFunc = GL_EQUAL;
        maskedState.depthMask = false;
        maskedState.depthTestEnabled = true;

        glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
        MultiDrawPerViewport(&msaaFbo, &shader, drawInfoSet.hair, maskedState);

        //glBindVertexArray(OpenGLBackEnd::GetWeightedVertexDataVAO());
        MultiDrawPerViewport(&msaaFbo, &shader, drawInfoSet.skinnedNonDeformingHair, maskedState);

        glBindVertexArray(OpenGLBackEnd::GetSkinnedVertexDataVAO());
        MultiDrawPerViewport(&msaaFbo, &shader, drawInfoSet.skinnedHair, maskedState);
    }

    void ShadingHair() {
        ProfilerOpenGLZoneFunction();

        const DrawCommandsSet& drawInfoSet = RenderDataManager::GetDrawInfoSet();

        OpenGLFrameBuffer& msaaFbo = GetFrameBuffer("MSAA");
        msaaFbo.Bind();
        msaaFbo.DrawBuffers({ "Lighting", "BaseColor", "Normal", "Material" });
        //msaaFbo.DrawBuffers({ "Lighting" });

        OpenGLShader& shader = GetShader("ShadedHair");
        shader.Bind();
        shader.SetFloat("u_renderResolutionScale", 1.0f);

        BindShadowMaps();

        OpenGLRasterizerState maskedState;
        maskedState.blendEnable = false;
        maskedState.cullfaceEnable = false;
        maskedState.colorMask = true;
        maskedState.depthFunc = GL_EQUAL;
        maskedState.depthMask = false;
        maskedState.depthTestEnabled = true;

        glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
        MultiDrawPerViewport(&msaaFbo, &shader, drawInfoSet.hair, maskedState);

        //glBindVertexArray(OpenGLBackEnd::GetWeightedVertexDataVAO());
        MultiDrawPerViewport(&msaaFbo, &shader, drawInfoSet.skinnedNonDeformingHair, maskedState);

        glBindVertexArray(OpenGLBackEnd::GetSkinnedVertexDataVAO());
        MultiDrawPerViewport(&msaaFbo, &shader, drawInfoSet.skinnedHair, maskedState);
    }


    void HairMLABSolid() {
        ProfilerOpenGLZoneFunction();

        const DrawCommandsSet& drawInfoSet = RenderDataManager::GetDrawInfoSet();

        OpenGLFrameBuffer& msaaFbo = GetFrameBuffer("Resolve");
        msaaFbo.Bind();
        msaaFbo.DrawBuffers({ "Lighting", "BaseColor", "Normal", "Material" });
        //msaaFbo.DrawBuffers({ "Lighting" });

        OpenGLShader& shader = GetShader("ShadedHair");
        shader.Bind();
        shader.SetFloat("u_renderResolutionScale", 1.0f);

        BindShadowMaps();

        OpenGLRasterizerState maskedState;
        maskedState.blendEnable = false;
        maskedState.cullfaceEnable = false;
        maskedState.colorMask = true;
        maskedState.depthFunc = GL_GREATER;
        maskedState.depthMask = true;
        maskedState.depthTestEnabled = true;

        glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
        MultiDrawPerViewport(&msaaFbo, &shader, drawInfoSet.hair, maskedState);

        //glBindVertexArray(OpenGLBackEnd::GetWeightedVertexDataVAO());
        MultiDrawPerViewport(&msaaFbo, &shader, drawInfoSet.skinnedNonDeformingHair, maskedState);

        glBindVertexArray(OpenGLBackEnd::GetSkinnedVertexDataVAO());
        MultiDrawPerViewport(&msaaFbo, &shader, drawInfoSet.skinnedHair, maskedState);
    }

    void HairMLAB() {
        ProfilerOpenGLZoneFunction();

        const DrawCommandsSet& drawInfoSet = RenderDataManager::GetDrawInfoSet();

        OpenGLFrameBuffer& fbo = GetFrameBuffer("Resolve");
        //OpenGLFrameBuffer& fbo = GetFrameBuffer("HalfRes");
        fbo.Bind();
        fbo.DrawBuffer(GL_NONE);

        OpenGLShader& shader = GetShader("HairMLAB");
        shader.Bind();
        shader.SetInt("u_renderTargetWidth", fbo.GetWidth());
        shader.SetUInt("u_mlabFrameIndex", g_mlabFrameIndex);

        BindSSBO(6, "HairMLABNodes");

        BindShadowMaps();

        OpenGLRasterizerState state;
        state.blendEnable = false;
        state.cullfaceEnable = false;
        state.colorMask = false;
        state.depthFunc = GL_GREATER;
        state.depthMask = false;
        state.depthTestEnabled = true;

        glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
        MultiDrawPerViewport(&fbo, &shader, drawInfoSet.hair, state);

        //glBindVertexArray(OpenGLBackEnd::GetWeightedVertexDataVAO());
        MultiDrawPerViewport(&fbo, &shader, drawInfoSet.skinnedNonDeformingHair, state);

        glBindVertexArray(OpenGLBackEnd::GetSkinnedVertexDataVAO());
        MultiDrawPerViewport(&fbo, &shader, drawInfoSet.skinnedHair, state);
    }



    void ResolveHairMLAB() {
        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer& fbo = GetFrameBuffer("Resolve");
        //OpenGLFrameBuffer& fbo = GetFrameBuffer("HalfRes");
        fbo.Bind();
        fbo.DrawBuffer({ "HairMLAB" });

        OpenGLShader& shader = GetShader("HairMLABResolve");
        shader.Bind();
        shader.SetInt("u_renderTargetWidth", fbo.GetWidth());
        shader.SetFloat("u_renderResolutionScale", 0.5f);
        shader.SetUInt("u_mlabFrameIndex", g_mlabFrameIndex);

        BindSSBO(6, "HairMLABNodes");
        BindTextureUnit(0, fbo.GetDepthAttachmentHandle());

        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        OpenGLRasterizerState state;
        state.blendEnable = false;
        state.cullfaceEnable = false;
        state.colorMask = true;
        state.depthTestEnabled = false;
        state.depthMask = false;
        SetRasterizerState(state);

        static GLuint emptyVAO = 0;
        if (emptyVAO == 0) {
            glCreateVertexArrays(1, &emptyVAO);
        }

        glBindVertexArray(emptyVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);
    }


    void ShadingBlended() {
        ProfilerOpenGLZoneFunction();

        const DrawCommandsSet& drawInfoSet = RenderDataManager::GetDrawInfoSet();

        OpenGLFrameBuffer& msaaFbo = GetFrameBuffer("MSAA");
        msaaFbo.Bind();
        msaaFbo.DrawBuffers({ "Lighting", "BaseColor", "Normal", "Material" });

        OpenGLShader& shader = GetShader("ShadedHardSurface");
        shader.Bind();

        BindShadowMaps();

        OpenGLRasterizerState blendedState;
        blendedState.blendEnable = true;
        blendedState.blendFuncSrcfactor = GL_SRC_ALPHA;
        blendedState.blendFuncDstfactor = GL_ONE_MINUS_SRC_ALPHA;
        blendedState.cullfaceEnable = false; // test this
        blendedState.depthFunc = GL_GEQUAL;
        blendedState.depthMask = false;
        blendedState.depthTestEnabled = true;

        glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
        MultiDrawPerViewport(&msaaFbo, &shader, drawInfoSet.blended, blendedState);

        //glBindVertexArray(OpenGLBackEnd::GetWeightedVertexDataVAO());
        MultiDrawPerViewport(&msaaFbo, &shader, drawInfoSet.skinnedNonDeformingBlended, blendedState);

        glBindVertexArray(OpenGLBackEnd::GetSkinnedVertexDataVAO());
        MultiDrawPerViewport(&msaaFbo, &shader, drawInfoSet.skinnedBlended, blendedState);

        glBindVertexArray(0);
    }

    void ResolveDepthNormalsMaterials() {
        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer& msaaFbo = GetFrameBuffer("MSAA");
        OpenGLFrameBuffer& resolveFbo = GetFrameBuffer("Resolve");
        OpenGLFrameBuffer& hairFbo = GetFrameBuffer("HairMSAA");
        OpenGLFrameBuffer& halfResFbo = GetFrameBuffer("HalfRes");

        OpenGLRenderer::BlitFrameBufferDepth(&msaaFbo, &resolveFbo);
        OpenGLRenderer::BlitFrameBufferDepth(&resolveFbo, &halfResFbo);
        OpenGLRenderer::BlitFrameBufferDepth(&halfResFbo, &hairFbo);

        OpenGLRenderer::BlitFrameBuffer(&msaaFbo, &resolveFbo, "Normal", "Normal", GL_COLOR_BUFFER_BIT, GL_LINEAR);
        OpenGLRenderer::BlitFrameBuffer(&msaaFbo, &resolveFbo, "Material", "Material", GL_COLOR_BUFFER_BIT, GL_LINEAR);
        OpenGLRenderer::BlitFrameBuffer(&msaaFbo, &resolveFbo, "Lighting", "Lighting", GL_COLOR_BUFFER_BIT, GL_LINEAR);
	}

    void ResolveLighting() {
        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer& msaaFbo = GetFrameBuffer("MSAA");
        OpenGLFrameBuffer& resolveFbo = GetFrameBuffer("Resolve");

        OpenGLRenderer::BlitFrameBuffer(&msaaFbo, &resolveFbo, "Lighting", "Lighting", GL_COLOR_BUFFER_BIT, GL_NEAREST);
	}

    void CompositeLighting() {
        if (hairOG)
            return;

        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer& indirectDiffuseFbo = GetFrameBuffer("IndirectDiffuse");
        OpenGLFrameBuffer& halfRes = GetFrameBuffer("HalfRes");
        OpenGLFrameBuffer& hairFbo = GetFrameBuffer("HairMSAA");
        OpenGLFrameBuffer& msaaFbo = GetFrameBuffer("MSAA");
        OpenGLFrameBuffer& resolveFbo = GetFrameBuffer("Resolve");

        BindShader("LightingComposite");

        BindImageTexture(0, resolveFbo.GetColorAttachmentHandleByName("Lighting"), GL_READ_WRITE, GL_RGBA16F);
        BindTextureUnit(1, resolveFbo.GetColorAttachmentHandleByName("Material"));
        BindTextureUnit(2, hairFbo.GetColorAttachmentHandleByName("Lighting"));
        BindTextureUnit(3, indirectDiffuseFbo.GetColorAttachmentHandleByName("Color"));

        OpenGLRenderer::BlitFrameBuffer(&hairFbo, &halfRes, "Lighting", "HairTest", GL_COLOR_BUFFER_BIT, GL_LINEAR);
        BindTextureUnit(4, halfRes.GetColorAttachmentHandleByName("HairTest"));

        BindTextureUnit(6, msaaFbo.GetColorAttachmentHandleByName("Normal"));
        BindTextureUnit(7, msaaFbo.GetColorAttachmentHandleByName("RENormal"));

        //BindTextureUnit(5, resolveFbo.GetColorAttachmentHandleByName("HairMLAB"));
        //BindTextureUnit(5, halfRes.GetColorAttachmentHandleByName("HairMLAB"));

        glDispatchCompute((resolveFbo.GetWidth() + 7) / 8, (resolveFbo.GetHeight() + 7) / 8, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	}

	void PostProcessingPassMSAA() {
		ProfilerOpenGLZoneFunction();

		RendererSettings& rendererSettings = Renderer::GetCurrentRendererSettings();

		// Only post process the following modes
		if (rendererSettings.rendererOverrideState == RendererOverrideState::NONE || // This means the final lit image
			rendererSettings.rendererOverrideState == RendererOverrideState::CAMERA_NDOTL ||
			rendererSettings.rendererOverrideState == RendererOverrideState::INDIRECT_DIFFUSE) {

			OpenGLFrameBuffer& gBuffer = GetFrameBuffer("Resolve");

			BindShader("PostProcessing");
			BindImageTexture(0, gBuffer.GetColorAttachmentHandleByName("Lighting"), GL_READ_WRITE, GL_RGBA16F);

			glDispatchCompute(gBuffer.GetWidth() / 8, gBuffer.GetHeight() / 8, 1);
		}
	}
}