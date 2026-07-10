#include "Unloved/Render/API/OpenGL/GL_renderer.h"

#include "Hell/Render/API/OpenGL/GL_back_end.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/Input.h"
#include "Hell/Time.h"
#include "Legacy/Timer.hpp"

#include "Unloved/Editor/Editor.h"
#include "Unloved/Render/RendererConstants.h"
#include "Unloved/Render/RendererTypes.h"
#include "Unloved/Systems/Ocean/Ocean.h"
#include "Unloved/Systems/Particles/ParticleManager.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Viewport/ViewportManager.h"

#include "World/LegacyWorld.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Render/Renderer.h"

#include "res/shaders/common/OpenGL/binding_indices.glsl"

namespace Input = Hell::Input;

namespace OpenGL::Renderer {
    struct RESettings {
        glm::ivec2 gBufferResolution = glm::ivec2(1920, 1080);
        glm::ivec2 finalImageResolution = glm::ivec2(1920, 1080) / 2;
    } g_settings;

    void BlendedLighting();
    void ClearRenderTargetsRE();
    void CreateFramebuffersRE();

    void LightingPassRE();
    void SkyboxPassRE();
    void GlassPassRE();
    void OceanRE();
    void EmissiveForwardPass();
    void BubblesPass2();
    void BubblesPass3();

    void RenderFullscreenTriangle();

    void RenderGameREStyle() {
        ComputeOceanFFTPass();

        OpenGLRasterizerState state;
        state.depthMask = true;
        state.colorMask = true;
        OpenGL::RasterizerStateManager::ForceRasterizerState(state);

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
        OpenGL::BindSSBO(SSBO_IDX_SAMPLERS, "Samplers");
        OpenGL::BindSSBO(SSBO_IDX_MATERIALS, "Materials");
        OpenGL::BindSSBO(SSBO_IDX_RENDERER_DATA, "RendererData");
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
        OpenGL::BindSSBO(SSBO_IDX_INSTANCE_DATA, "InstanceData");
        OpenGL::BindSSBO(SSBO_IDX_LIGHTS, "Lights");
        OpenGL::BindSSBO(6, "TileLights");
        // TODO: OpenGL::BindSSBO(9, "TileChristmasLights");
        // TODO: OpenGL::BindSSBO(10, "ChristmasLightInstances");
        // TODO: OpenGL::BindSSBO(11, "ChristmasLightIndices");

        LightingPassRE();
        BlendedLighting();

        SkyboxPassRE();
        HairPassRE();
        OceanRE();


        OceanUnderWaterFlags();
        OceanSurfaceCompositePass();

        GlassPassRE();
        EmissivePass();
        RayMarchFog();
        GaussianBlur();

        OceanUnderwaterCompositePass();

        StainedGlassPass();
        BubblesPass2();
        BubblesPass3();

        ParticlePass();

        // DDGI Debug
        Unloved::DDGIVolume& ddgiVolume = Unloved::LegacyWorld::GetTestDDGIVolume();
        if (Unloved::Renderer::GetCurrentRendererSettings().debugDrawPointCloud)       DrawPointCloud(ddgiVolume);
        if (Unloved::Renderer::GetCurrentRendererSettings().debugDrawPointCloudGrid)   DrawPointCloudGrid(ddgiVolume);
        if (Unloved::Renderer::GetCurrentRendererSettings().debugDrawIrradianceProbes) DrawProbes(ddgiVolume);

        SpriteSheetPass(); // Muzzle flash, etc

        PostProcessingPassRE();
        DebugViewPass();
        DebugPass();

        EditorPass();
        OutlinePass();

        OpenGLFrameBuffer& finalImageFbo = OpenGL::ResourceManager::GetFrameBuffer("FinalImage");
        OpenGLFrameBuffer& gBuffer = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
        OpenGLFrameBuffer& presentFbo = OpenGL::ResourceManager::GetFrameBuffer("Present");

        // Downscale with linear filtering
        OpenGL::BlitFrameBuffer(&gBuffer, &finalImageFbo, "Lighting", "Color", GL_COLOR_BUFFER_BIT, GL_LINEAR);

        // Upscale with nearest filtering
        OpenGL::BlitFrameBuffer(&finalImageFbo, &presentFbo, "Color", "Color", GL_COLOR_BUFFER_BIT, GL_NEAREST);

        UIPass();

        PresentFinalImage(presentFbo);

        ImGuiPass();
    }


	void ClearRenderTargetsRE() {
		OpenGLFrameBuffer& gBuffer = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
        gBuffer.ClearAttachment("Lighting", 0, 0, 0, 1);
        gBuffer.ClearAttachment("BaseColorMetallic", 0, 0, 0, 1);
        gBuffer.ClearAttachment("NormalXYRoughnessMisc", 0, 0, 0, 1);
        gBuffer.ClearAttachment("VelocityXYOcclusionSubSurface", 0, 0, 0, 1);
        gBuffer.ClearAttachment("Emissive", 0.0f, 0.0f, 0.0f, 0.0f);
        gBuffer.ClearAttachmentUI("Visibility", 0, 0, 0, 0);
        gBuffer.ClearDepthAttachment(0.0f);
        gBuffer.ClearStencilBits(0);

		OpenGLFrameBuffer& hairFboRE = OpenGL::ResourceManager::GetFrameBuffer("HairRE");
		hairFboRE.ClearAttachment("Lighting", 0, 0, 0, 0);
		hairFboRE.ClearDepthAttachment(0.0f);

        OpenGLFrameBuffer& waterFrameBuffer = OpenGL::ResourceManager::GetFrameBuffer("Water");
        waterFrameBuffer.Bind();
        waterFrameBuffer.ClearAttachment("Lighting", 0, 0, 0, 0);
        waterFrameBuffer.ClearAttachmentUI("OceanFlags", 0, 0, 0, 0);
        waterFrameBuffer.ClearAttachmentUI("OceanMask", 0, 0, 0, 0);
	}

	void BindShadowMapsRE() {
		OpenGLShadowMap& flashLightShadowMaps = OpenGL::ResourceManager::GetShadowMap("FlashlightShadowMaps");
		OpenGLShadowCubeMapArray& hiResShadowMaps = OpenGL::ResourceManager::GetShadowCubeMapArray("HiRes");
		OpenGLShadowCubeMapArray& lowResShadowMaps = OpenGL::ResourceManager::GetShadowCubeMapArray("LowRes");
		OpenGLShadowMapArray& moonShadowCascades = OpenGL::ResourceManager::GetShadowMapArray("MoonlightCSM");

		OpenGL::BindTextureUnit(9, GetTextureHandleByName("Flashlight2"));
		OpenGL::BindTextureUnit(TEX_IDX_SHADOW_MAP_FLASHLIGHT, flashLightShadowMaps.GetDepthTextureHandle());
		OpenGL::BindTextureUnit(TEX_IDX_SHADOW_MAP_HI_RES, hiResShadowMaps.GetDepthTexture());
		OpenGL::BindTextureUnit(TEX_IDX_SHADOW_MAP_LOW_RES, lowResShadowMaps.GetDepthTexture());
		OpenGL::BindTextureUnit(TEX_IDX_SHADOW_MAP_CSM, moonShadowCascades.GetDepthTexture());
	}

    void BubblesPass2() {
        //ProfilerOpenGLZoneFunction();

        const std::vector<ViewportData>& viewportData = Unloved::RenderDataManager::GetViewportData();

        OpenGLCubemapView& skyboxCubemapView = OpenGL::ResourceManager::GetCubemapView("SkyboxNightSky");
        OpenGLFrameBuffer& fbo = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");

        fbo.Bind();
        fbo.SetViewport();
        fbo.DrawBuffers({ "Lighting" });

        OpenGLRasterizerState state;
        state.depthTestEnabled = true;
        state.cullfaceEnable = false;
        state.depthMask = false;
        state.colorMask = true;
        state.depthFunc = GL_GREATER;

        state.blendEnable = true;
        state.blendFuncSrcfactor = GL_SRC_ALPHA;
        state.blendFuncDstfactor = GL_ONE_MINUS_SRC_ALPHA;

        OpenGL::RasterizerStateManager::SetRasterizerState(state);

        OpenGL::BindShader("Bubbles2");
        OpenGL::SetUniformFloat("u_time", Unloved::Session::GetSessionTime());
        OpenGL::BindTextureUnit(0, skyboxCubemapView.GetHandle());
        OpenGL::BindTextureUnit(1, GetTextureHandleByName("Bubbles_10x10"));

        BindEmptyVAO();

        ParticleManager::Update(Hell::Time::DeltaTime());

        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGL::Renderer::SetViewport(&fbo, viewport);
            OpenGL::SetUniformInt("u_viewportIndex", i);
            OpenGL::SetUniformMat4("u_projectionView", viewportData[i].projectionViewReverseZ);
            OpenGL::SetUniformMat4("u_view", viewportData[i].view);
            OpenGL::SetUniformVec3("u_viewPos", viewportData[i].viewPos);

            for (Particle& particle : ParticleManager::GetParticles()) {
                OpenGL::SetUniformVec3("u_particlePosition", particle.position);
                OpenGL::SetUniformFloat("u_particleRotation", particle.rotation);
                OpenGL::SetUniformFloat("u_particleScale", particle.scale);
                OpenGL::SetUniformFloat("u_particleAlphaFade", particle.alphaFade);
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
        }
    }

    void BubblesPass3() {
        //ProfilerOpenGLZoneFunction();

        const std::vector<ViewportData>& viewportData = Unloved::RenderDataManager::GetViewportData();

        OpenGLCubemapView& skyboxCubemapView = OpenGL::ResourceManager::GetCubemapView("SkyboxNightSky");
        OpenGLFrameBuffer& fbo = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");

        fbo.Bind();
        fbo.SetViewport();
        fbo.DrawBuffers({ "Lighting" });

        OpenGLRasterizerState state;
        state.depthTestEnabled = true;
        state.cullfaceEnable = false;
        state.depthMask = false;
        state.colorMask = true;
        state.depthFunc = GL_GREATER;

        state.blendEnable = true;
        state.blendFuncSrcfactor = GL_SRC_ALPHA;
        state.blendFuncDstfactor = GL_ONE_MINUS_SRC_ALPHA;

        OpenGL::RasterizerStateManager::SetRasterizerState(state);

        OpenGL::BindShader("Bubbles3");
        OpenGL::SetUniformFloat("u_time", Unloved::Session::GetSessionTime());
        OpenGL::BindTextureUnit(0, GetTextureHandleByName("UnderwaterBulletBubble"));

        BindEmptyVAO();

        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGL::Renderer::SetViewport(&fbo, viewport);
            OpenGL::SetUniformInt("u_viewportIndex", i);
            OpenGL::SetUniformMat4("u_projectionView", viewportData[i].projectionViewReverseZ);
            OpenGL::SetUniformMat4("u_view", viewportData[i].view);
            OpenGL::SetUniformVec3("u_viewPos", viewportData[i].viewPos);

            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        //glm::vec3 pos = glm::vec3(36.0, 32.5, 37.0);
        //DrawPoint(pos, RED);
    }

    void LightingPassRE() {
        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer& gBuffer = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
        OpenGLFrameBuffer& indirectDiffuseFbo = OpenGL::ResourceManager::GetFrameBuffer("IndirectDiffuse");

        OpenGL::BindShader("LightingDeferred");
        BindShadowMapsRE();

        std::vector<float>& cascadeLevels = GetShadowCascadeLevels();
        OpenGL::SetUniformFloat("u_cascadeFarPlane", 256.0f); // ???
        OpenGL::SetUniformFloat("u_cascadePlaneDistances[0]", cascadeLevels[0]);
        OpenGL::SetUniformFloat("u_cascadePlaneDistances[1]", cascadeLevels[1]);
        OpenGL::SetUniformFloat("u_cascadePlaneDistances[2]", cascadeLevels[2]);
        OpenGL::SetUniformFloat("u_cascadePlaneDistances[3]", cascadeLevels[3]);

        OpenGL::BindTextureUnit(4, gBuffer.GetColorAttachmentHandleByName("BaseColorMetallic"));
        OpenGL::BindTextureUnit(5, gBuffer.GetColorAttachmentHandleByName("NormalXYRoughnessMisc"));
        OpenGL::BindTextureUnit(6, gBuffer.GetColorAttachmentHandleByName("VelocityXYOcclusionSubSurface"));
        OpenGL::BindTextureUnit(7, gBuffer.GetDepthAttachmentHandle());
        OpenGL::BindTextureUnit(8, indirectDiffuseFbo.GetColorAttachmentHandleByName("Color"));

        OpenGLFrameBuffer& fbo = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
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
        state.stencilWriteMask = 0x00;
        state.stencilFailOp = GL_KEEP;
        state.stencilDepthFailOp = GL_KEEP;
        state.stencilPassOp = GL_KEEP;

        // Only allow these bits through
        state.stencilFunc = GL_NOTEQUAL;
        state.stencilRef = 0;
        state.stencilReadMask = STENCIL_BIT_ASSET | STENCIL_BIT_SKINNED | STENCIL_BIT_PROCEDUAL;

        OpenGL::RasterizerStateManager::SetRasterizerState(state);

        RenderFullscreenTriangle();
	}

	void BlendedLighting() {
		ProfilerOpenGLZoneFunction();

		const DrawCommandsSet& drawInfoSet = Unloved::RenderDataManager::GetDrawInfoSet();

		OpenGLFrameBuffer& fbo = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
        OpenGLFrameBuffer& indirectDiffuseFbo = OpenGL::ResourceManager::GetFrameBuffer("IndirectDiffuse");
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
		OpenGLShader& opaqueShader = OpenGL::ResourceManager::GetShader("LightingForward");
		OpenGL::BindShader("LightingForward");
        OpenGL::BindTextureUnit(5, indirectDiffuseFbo.GetColorAttachmentHandleByName("Color"));

        OpenGLMeshBuffer& meshBuffer = OpenGL::ResourceManager::GetMeshBuffer("AssetGeometry");
		glBindVertexArray(meshBuffer.GetVAO());
		MultiDrawPerViewport(fbo, opaqueShader, drawInfoSet.blended, state);

		//glBindVertexArray(OpenGL::BackEnd::GetWeightedVertexDataVAO());
		MultiDrawPerViewport(fbo, opaqueShader, drawInfoSet.skinnedNonDeformingBlended, state);

		glBindVertexArray(OpenGL::BackEnd::GetSkinnedVertexDataVAO());
		MultiDrawPerViewport(fbo, opaqueShader, drawInfoSet.skinnedBlended, state);
	}

    void SkyboxPassRE() {
        if (Unloved::Editor::IsOpen()) return;

        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer& gBuffer = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
        OpenGLCubemapView* skyboxCubemapView = OpenGL::ResourceManager::GetCubemapViewPtr("SkyboxNightSky");

        gBuffer.Bind();
        gBuffer.SetViewport();
        gBuffer.DrawBuffers({ "Lighting" });

        OpenGL::BindShader("SkyboxRE");

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

        OpenGL::RasterizerStateManager::SetRasterizerState(state);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxCubemapView->GetHandle());
        glBindVertexArray(OpenGL::ResourceManager::GetMeshBuffer("AssetGeometry").GetVAO());

        RenderFullscreenTriangle();
    }

    void OceanRE() {
        ProfilerOpenGLZoneFunction();
        if (!Unloved::LegacyWorld::HasOcean()) return;

        const std::vector<ViewportData>& viewportData = Unloved::RenderDataManager::GetViewportData();

        OpenGLCubemapView& skyboxCubemapView = OpenGL::ResourceManager::GetCubemapView("SkyboxNightSky");
        OpenGLFrameBuffer& fftBand0Fbo = OpenGL::ResourceManager::GetFrameBuffer("FFT_band0");
        OpenGLFrameBuffer& fftBand1Fbo = OpenGL::ResourceManager::GetFrameBuffer("FFT_band1");
        OpenGLFrameBuffer& gBuffer = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
        OpenGLFrameBuffer& waterFbo = OpenGL::ResourceManager::GetFrameBuffer("Water");

        const int gridSize = 128;
        const int lodLevels = 6;
        const int vertexCount = gridSize * gridSize * 6 * lodLevels;

        OpenGL::BindShader("OceanLighting");
        OpenGL::SetUniformInt("u_gridWidth", gridSize);
        OpenGL::SetUniformFloat("u_oceanOriginY", Ocean::GetOceanOriginY());
        OpenGL::SetUniformFloat("u_time", Unloved::Session::GetSessionTime());

        OpenGL::BindTextureUnit(0, fftBand0Fbo.GetColorAttachmentHandleByName("Displacement"));
        OpenGL::BindTextureUnit(1, fftBand0Fbo.GetColorAttachmentHandleByName("Normals"));
        OpenGL::BindTextureUnit(2, fftBand1Fbo.GetColorAttachmentHandleByName("Displacement"));
        OpenGL::BindTextureUnit(3, fftBand1Fbo.GetColorAttachmentHandleByName("Normals"));
        OpenGL::BindTextureUnit(4, skyboxCubemapView.GetHandle());
        OpenGL::BindTextureUnit(5, GetTextureHandleByName("WaterNormals"));

        OpenGLRasterizerState state;
        state.depthTestEnabled = true;
        state.blendEnable = false;
        state.cullfaceEnable = false;
        state.depthMask = true;
        state.colorMask = true;
        state.depthFunc = GL_GREATER;

        OpenGL::RasterizerStateManager::SetRasterizerState(state);
        BindEmptyVAO();

        static bool lines = false;
        if (Input::KeyPressed(HELL_KEY_L)) {
            lines = !lines;
        }

        OpenGL::BlitFrameBufferDepth(&gBuffer, &waterFbo);

        waterFbo.Bind();
        waterFbo.DrawBuffers({ "Lighting", "OceanMask" });

        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGL::Renderer::SetViewport(&waterFbo, viewport);
            OpenGL::SetUniformInt("u_viewportIndex", i);

            if (lines) glDrawArrays(GL_LINES, 0, vertexCount);
            else       glDrawArrays(GL_TRIANGLES, 0, vertexCount);
        }

        glBindVertexArray(0);
    }

    void GlassPassRE() {
        ProfilerOpenGLZoneFunction();

        OpenGL::RasterizerStateManager::ForceRasterizerState("GlassPass");

        const DrawCommandsSet& drawInfoSet = Unloved::RenderDataManager::GetDrawInfoSet();
        const std::vector<ViewportData>& viewportData = Unloved::RenderDataManager::GetViewportData();

        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("Glass");
        OpenGLShader* compositeShader = OpenGL::ResourceManager::GetShaderPtr("GlassComposite");
        OpenGLFrameBuffer* gBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");
        OpenGLShadowMap* flashLightShadowMapsFBO = OpenGL::ResourceManager::GetShadowMapPtr("FlashlightShadowMaps");

        if (!shader) return;
        if (!compositeShader) return;
        if (!gBuffer) return;
        if (!flashLightShadowMapsFBO) return;

        // TODO: explicitly bind all other ssbos used by this render pass
        OpenGL::BindSSBO(SSBO_IDX_MATERIALS, "Materials");

        OpenGL::BindShader("Glass");
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
        glBindTextureUnit(7, GetTextureHandleByName("Flashlight2"));
        glBindTextureUnit(TEX_IDX_SHADOW_MAP_FLASHLIGHT, flashLightShadowMapsFBO->GetDepthTextureHandle());

        // Forward render each glass render item into each viewport
        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGL::Renderer::SetViewport(gBuffer, viewport);
            OpenGL::SetUniformInt("u_viewportIndex", i);

            for (const RenderItem& renderItem : drawInfoSet.glass[i]) {
                OpenGL::SetUniformMat4("u_modelMatrix", renderItem.modelMatrix);

                Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(renderItem.meshId);
                if (!mesh) continue;

                Material* material = Hell::ResourceManager::GetMaterialByIndex(renderItem.materialIndex);
                glActiveTexture(GL_TEXTURE4);
                glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(material->m_basecolor)->GetGLTexture().GetHandle());
                glActiveTexture(GL_TEXTURE5);
                glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(material->m_normal)->GetGLTexture().GetHandle());
                glActiveTexture(GL_TEXTURE6);
                glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(material->m_rma)->GetGLTexture().GetHandle());

                glDrawElementsBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), mesh->baseVertex);
            }
        }

        // Composite that render back into the lighting texture
        //gBuffer->SetViewport();
        //glBindImageTexture(0, gBuffer->GetColorAttachmentHandleByName("Lighting"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
        //glBindImageTexture(1, gBuffer->GetColorAttachmentHandleByName("Glass"), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);
        //OpenGL::DispatchCompute(gBuffer->GetWidth() / 16, gBuffer->GetHeight() / 4, 1);
        //
        //glDepthMask(GL_TRUE);
    }

    void EmissiveForwardPass() {
        ProfilerOpenGLZoneFunction();
        const DrawCommandsSet& drawInfoSet = Unloved::RenderDataManager::GetDrawInfoSet();

        OpenGLFrameBuffer& fbo = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
        fbo.Bind();
        fbo.DrawBuffers({ "Emissive" });

        OpenGL::BindShader("EmissiveForward");

        OpenGL::BindSSBO(SSBO_IDX_SAMPLERS, "Samplers");
        OpenGL::BindSSBO(SSBO_IDX_MATERIALS, "Materials");
        OpenGL::BindSSBO(SSBO_IDX_RENDERER_DATA, "RendererData");
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
        OpenGL::BindSSBO(SSBO_IDX_INSTANCE_DATA, "InstanceData");

        OpenGLRasterizerState state;
        state.depthTestEnabled = true;
        state.blendEnable = false;
        state.cullfaceEnable = false;
        state.depthMask = false;
        state.colorMask = true;
        state.depthFunc = GL_EQUAL;

        glBindVertexArray(OpenGL::ResourceManager::GetMeshBuffer("AssetGeometry").GetVAO());

        MultiDrawPerViewportRE(fbo, drawInfoSet.emissive, state);
    }

    void RenderFullscreenTriangle() {
        glBindVertexArray(OpenGL::ResourceManager::GetMeshBuffer("AssetGeometry").GetVAO());
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }
}
