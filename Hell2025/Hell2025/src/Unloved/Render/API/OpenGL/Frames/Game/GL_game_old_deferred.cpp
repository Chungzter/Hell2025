#include "Unloved/Render/API/OpenGL/GL_renderer.h"

#include "Hell/Audio.h"
#include "Hell/Input.h"
#include "Legacy/Timer.hpp"
#include "Unloved/Render/Renderer.h"
#include "World/LegacyWorld.h"

namespace Audio = Hell::Audio;
namespace Input = Hell::Input;

namespace OpenGLRenderer {

    int g_fftDisplayMode = 0;
    int g_fftEditBand = 0;

    void ClearRenderTargets();

    int GetFftDisplayMode() {
        return g_fftDisplayMode;
    }

    void RenderGame() {
        ProfilerOpenGLFrame();

		if (Unloved::Renderer::GetRendererMode() == RendererMode::RE_STYLE) {
			RenderGameREStyle();
			return;
		}


        ComputeOceanFFTPass();
        OceanHeightReadback();

        OpenGLFrameBuffer& hairFrameBuffer = OpenGL::ResourceManager::GetFrameBuffer("Hair");
        Unloved::DDGIVolume& ddgiVolume = Unloved::LegacyWorld::GetTestDDGIVolume();

        glDisable(GL_DITHER);

        if (Input::KeyPressed(HELL_KEY_N)) {
            Audio::PlayAudio(AUDIO_SELECT, 1.00f);
            FlipNormalMapY();
        }

        //BlitRoads();

        ComputeSkinningPass();
        ClearRenderTargets();

        UpdateSSBOS();
        RenderShadowMaps();
        SkyBoxPass();
        HeightMapPass();


        DecalPaintingPass();
        HouseGeometryPass();
        GeometryPass();
        GrassPass();
        MirrorGeometryPass();
        WeatherBoardsPass();
        VatBloodPass();

        ComputeTileWorldBounds();
        ChristmasLightCullingPass();
        LightCullingPass();

        BloodDecalsPass();
        ComputeViewspaceDepth();

        // GI
        UpdateGlobalIllumintation();

        OpenGL::BindSSBO(0, "Samplers");
        OpenGL::BindSSBO(1, "RendererData");
        OpenGL::BindSSBO(2, "ViewportData");
        OpenGL::BindSSBO(3, "InstanceData");
        OpenGL::BindSSBO(4, "Lights");
        OpenGL::BindSSBO(5, "TileLights");
        OpenGL::BindSSBO(6, "TileWorldBounds");

        OpenGL::BindSSBO(10, "ProbeSHColor");
        OpenGL::BindSSBO(11, "ProbeStates");

        LightingPass();

        //FurPass();
        OceanGeometryPass();
        OceanUnderWaterFlags();
        OceanSurfaceCompositePass();

        GlassPass();
        EmissivePass();
        ScreenspaceReflectionsPass();
        HairPass();
        //DepthPeeledTransparencyPass();
        PlasticPass();
        RayMarchFog();
        GaussianBlur();
        OceanUnderwaterCompositePass();
        StainedGlassPass();
        WinstonPass();
        SpriteSheetPass(); // Muzzle flash, etc
        InventoryGaussianPass();

        // Disabling lighting actually just clears it, that way you don't have fog and shit everywhere
        if (!Unloved::Renderer::GetCurrentRendererSettings().enableLighting) {
            OpenGLFrameBuffer& gBuffer = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
            gBuffer.Bind();
            gBuffer.ClearAttachment("Lighting", 0, 0, 0, 0);
        }

        if (Unloved::Renderer::GetCurrentRendererSettings().debugDrawPointCloud)       DrawPointCloud(ddgiVolume);
        if (Unloved::Renderer::GetCurrentRendererSettings().debugDrawPointCloudGrid)   DrawPointCloudGrid(ddgiVolume);
        if (Unloved::Renderer::GetCurrentRendererSettings().debugDrawIrradianceProbes) DrawProbes(ddgiVolume);

        PostProcessingPass();

        DebugViewPass();
        DebugPass();

        ExamineItemPass();
        EditorPass();
        OutlinePass();

        //DownSampleFinalImage();

        //if (Input::KeyDown(HELL_KEY_U)) {
        //    OpenGLFrameBuffer* bloodFluidFbo = OpenGL::ResourceManager::GetFrameBufferPtr("BloodFluid");
        //    OpenGL::BlitFrameBuffer(bloodFluidFbo, &finalImageBuffer, "Depth", "Color", GL_COLOR_BUFFER_BIT, GL_LINEAR);
        //}
        //if (Input::KeyDown(HELL_KEY_Y)) {
        //    OpenGLFrameBuffer* bloodFluidFbo = OpenGL::ResourceManager::GetFrameBufferPtr("BloodFluid");
        //    OpenGL::BlitFrameBuffer(bloodFluidFbo, &finalImageBuffer, "Thickness", "Color", GL_COLOR_BUFFER_BIT, GL_LINEAR);
        //}
        //if (Input::KeyDown(HELL_KEY_T)) {
        //    OpenGLFrameBuffer* bloodFluidFbo = OpenGL::ResourceManager::GetFrameBufferPtr("BloodFluid");
        //    OpenGL::BlitFrameBuffer(bloodFluidFbo, &finalImageBuffer, "BlurIntermediate", "Color", GL_COLOR_BUFFER_BIT, GL_LINEAR);
        //}

        //BlitFog();

        OpenGLFrameBuffer& finalImageFbo = OpenGL::ResourceManager::GetFrameBuffer("FinalImage");
        OpenGLFrameBuffer& gBufferRE = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
        OpenGLFrameBuffer& presentFbo = OpenGL::ResourceManager::GetFrameBuffer("Present");

        // Downscale with linear filtering
        OpenGL::BlitFrameBuffer(&gBufferRE, &finalImageFbo, "Lighting", "Color", GL_COLOR_BUFFER_BIT, GL_LINEAR);

        // Upscale with nearest filtering
        OpenGL::BlitFrameBuffer(&finalImageFbo, &presentFbo, "Color", "Color", GL_COLOR_BUFFER_BIT, GL_NEAREST);

        UIPass();

        // Blit to swap chain
        OpenGL::BlitToDefaultFrameBuffer(&presentFbo, "Color", GL_COLOR_BUFFER_BIT, GL_NEAREST);

        ImGuiPass();
        BlitDebugTextures();

        // DEBUG RENDER FFT TEXTURES TO THE SCREEN
        if (Input::KeyPressed(HELL_KEY_5)) {
            g_fftDisplayMode = 1;
            g_fftEditBand = 0;
        }
        if (Input::KeyPressed(HELL_KEY_6)) {
            g_fftDisplayMode = 2;
            g_fftEditBand = 1;
        }
        if (Input::KeyPressed(HELL_KEY_7)) {
            g_fftDisplayMode = 0;
        }
    }

    void ClearRenderTargets() {
        glDepthMask(GL_TRUE);

        // Water
        OpenGLFrameBuffer& waterFrameBuffer = OpenGL::ResourceManager::GetFrameBuffer("Water");
        waterFrameBuffer.Bind();
        waterFrameBuffer.ClearAttachment("Lighting", 0, 0, 0, 0);
        waterFrameBuffer.ClearAttachmentUI("OceanFlags", 0, 0, 0, 0);
        waterFrameBuffer.ClearAttachmentUI("OceanMask", 0, 0, 0, 0);

        // GBuffer
        OpenGLFrameBuffer& gBuffer = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
        gBuffer.Bind();
        gBuffer.ClearAttachment("Lighting", 0, 0, 0, 1);
        gBuffer.ClearAttachment("BaseColorMetallic", 0, 0, 0, 1);
        gBuffer.ClearAttachment("NormalXYRoughnessMisc", 0, 0, 0, 1);
        gBuffer.ClearAttachment("VelocityXYOcclusionSubSurface", 0, 0, 0, 1);
        gBuffer.ClearAttachment("Emissive", 0.0f, 0.0f, 0.0f, 0.0f);
        gBuffer.ClearAttachmentUI("Visibility", 0, 0, 0, 0);
        gBuffer.ClearDepthAttachment(0.0f);
        gBuffer.ClearStencilBits(0);

        gBuffer.ClearAttachment("Glass", 0, 0, 0, 0); // TODO: remove me when/if u can

        // Decal mask
        OpenGLFrameBuffer& miscFullSizeFBO = OpenGL::ResourceManager::GetFrameBuffer("MiscFullSize");
        miscFullSizeFBO.Bind();
        miscFullSizeFBO.ClearTexImage("BloodScreenSpaceDecalMask", 0, 0, 0, 0);
    }
}
