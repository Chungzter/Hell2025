#include "../GL_renderer.h"
#include "Renderer/Renderer.h"
#include "World/LegacyWorld.h"
namespace OpenGLRenderer {
    std::vector<GPUChristmasLight> g_gpuLights;

    void ComputeTileWorldBounds() {
        ProfilerOpenGLZoneFunction();

        uint32_t depthHandle = 0;

		switch (Renderer::GetRendererMode()) {
		    case RendererMode::OLD_DEFERRED: depthHandle = GetFrameBuffer("GBuffer").GetDepthAttachmentHandle();   break;
            case RendererMode::RE_STYLE:     depthHandle = GetFrameBuffer("GBufferRE").GetDepthAttachmentHandle(); break;
		}

        BindShader("TileWorldBounds");
        SetUniformInt("u_tileXCount", GetTileCountX());
		SetUniformInt("u_tileYCount", GetTileCountY());

        BindSSBO(1, "RendererData");
        BindSSBO(2, "ViewportData");
        BindSSBO(6, "TileWorldBounds");

        BindTextureUnit(0, depthHandle);

        glDispatchCompute(GetTileCountX(), GetTileCountY(), 1);
    }

    void LightCullingPass() {
        ProfilerOpenGLZoneFunction();

        OpenGLShader* shader = GetShaderOLD("LightCulling");

        if (!shader) return;

        shader->Bind();
        shader->SetInt("u_lightCount", LegacyWorld::GetLights().size());
        shader->SetInt("u_tileXCount", GetTileCountX());
        shader->SetInt("u_tileYCount", GetTileCountY());

        BindSSBO(1, "RendererData");
        BindSSBO(2, "ViewportData");
        BindSSBO(4, "Lights");
        BindSSBO(5, "TileLights");
        BindSSBO(6, "TileWorldBounds");

        glDispatchCompute(GetTileCountX(), GetTileCountY(), 1);
    }

    void ChristmasLightCullingPass() {
        ProfilerOpenGLZoneFunction();

        // Clear the lights from last frame, coz they change
        g_gpuLights.clear();

        // Gather all the Christmas lights from ALL the ChristmasLightSets
        Hell::SlotMap<ChristmasLightSet>& christmasLightSets = LegacyWorld::GetChristmasLightSets();

        for (ChristmasLightSet& christmasLightSet : christmasLightSets) {
            const std::vector<GPUChristmasLight>& gpuLights = christmasLightSet.GetGPUChristmasLights();
            g_gpuLights.insert(g_gpuLights.end(), gpuLights.begin(), gpuLights.end());
        }

        UpdateSSBO("ChristmasLightInstances", g_gpuLights.size() * sizeof(GPUChristmasLight), (void*)&g_gpuLights[0]);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        // Debug draw the lights as points
        //for (const GPUChristmasLight& light : g_gpuLights) {
        //    DebugDraw::DrawPoint(light.position, light.color);
        //}

        OpenGLShader* shader = GetShaderOLD("ChristmasLightCulling");
        if (!shader) return;

        shader->Bind();
        shader->SetInt("u_christmasLightCount", g_gpuLights.size());
        shader->SetInt("u_tileXCount", GetTileCountX());
        shader->SetInt("u_tileYCount", GetTileCountY());

        BindSSBO(6, "TileWorldBounds");
        BindSSBO(7, "TileChristmasLights");
        BindSSBO(8, "ChristmasLightInstances");
        BindSSBO(9, "ChristmasLightIndices");
        BindSSBO(10, "ChristmasLightCounter");

        glDispatchCompute(GetTileCountX(), GetTileCountY(), 1);
    }

    void BloodDecalTileCulling() {
        ProfilerOpenGLZoneFunction();

        OpenGLShader* shader = GetShaderOLD("BloodDecalsCulling");
        if (!shader) return;

        shader->Bind();
        shader->SetInt("u_decalCount", LegacyWorld::GetScreenSpaceBloodDecals().size());
        shader->SetInt("u_tileXCount", GetTileCountX());
        shader->SetInt("u_tileYCount", GetTileCountY());

        BindSSBO(6, "TileWorldBounds");
        BindSSBO(7, "TileBloodDecals");
        BindSSBO(8, "BloodDecalInstances");
        BindSSBO(9, "BloodDecalIndices");
        BindSSBO(10, "BloodDecalCounter");

        glDispatchCompute(GetTileCountX(), GetTileCountY(), 1);

        //if (Input::KeyPressed(HELL_KEY_SPACE)) {
        //    std::cout << "Blood count: " << LegacyWorld::GetScreenSpaceBloodDecals().size() << "\n";
        //}
    }
}
