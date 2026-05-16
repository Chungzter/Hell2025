#include "../GL_renderer.h"
#include "Renderer/Renderer.h"
#include "World/World.h"

namespace OpenGLRenderer {
    std::vector<GPUChristmasLight> g_gpuLights;

    void ComputeTileWorldBounds() {
        ProfilerOpenGLZoneFunction();

        uint32_t depthHandle = 0;

		switch (Renderer::GetRendererMode()) {
		    case RendererMode::OLD_DEFERRED: depthHandle = GetFrameBuffer("GBuffer").GetDepthAttachmentHandle();   break;
		    case RendererMode::MSAA:         depthHandle = GetFrameBuffer("Resolve").GetDepthAttachmentHandle();   break;
            case RendererMode::RE_STYLE:     depthHandle = GetFrameBuffer("GBufferRE").GetDepthAttachmentHandle(); break;
		}

        BindShader("TileWorldBounds");
        SetUniformInt("u_tileXCount", GetTileCountX());
		SetUniformInt("u_tileYCount", GetTileCountY());

        BindSSBO("RendererData", 1);
        BindSSBO("ViewportData", 2);
        BindSSBO("TileWorldBounds", 6);

        BindTextureUnit(0, depthHandle);

        glDispatchCompute(GetTileCountX(), GetTileCountY(), 1);
    }

    void LightCullingPass() {
        ProfilerOpenGLZoneFunction();

        OpenGLShader* shader = GetShaderOLD("LightCulling");

        if (!shader) return;

        shader->Bind();
        shader->SetInt("u_lightCount", World::GetLights().size());
        shader->SetInt("u_tileXCount", GetTileCountX());
        shader->SetInt("u_tileYCount", GetTileCountY());

        BindSSBO("RendererData", 1);
        BindSSBO("ViewportData", 2);
        BindSSBO("Lights", 4);
        BindSSBO("TileLights", 5);
        BindSSBO("TileWorldBounds", 6);

        glDispatchCompute(GetTileCountX(), GetTileCountY(), 1);
    }

    void ChristmasLightCullingPass() {
        ProfilerOpenGLZoneFunction();

        // Clear the lights from last frame, coz they change
        g_gpuLights.clear();

        // Gather all the Christmas lights from ALL the ChristmasLightSets
        Hell::SlotMap<ChristmasLightSet>& christmasLightSets = World::GetChristmasLightSets();

        for (ChristmasLightSet& christmasLightSet : christmasLightSets) {
            const std::vector<GPUChristmasLight>& gpuLights = christmasLightSet.GetGPUChristmasLights();
            g_gpuLights.insert(g_gpuLights.end(), gpuLights.begin(), gpuLights.end());
        }

        UpdateSSBO("ChristmasLightInstances", g_gpuLights.size() * sizeof(GPUChristmasLight), (void*)&g_gpuLights[0]);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        // Debug draw the lights as points
        //for (const GPUChristmasLight& light : g_gpuLights) {
        //    Renderer::DrawPoint(light.position, light.color);
        //}

        OpenGLShader* shader = GetShaderOLD("ChristmasLightCulling");
        if (!shader) return;

        shader->Bind();
        shader->SetInt("u_christmasLightCount", g_gpuLights.size());
        shader->SetInt("u_tileXCount", GetTileCountX());
        shader->SetInt("u_tileYCount", GetTileCountY());

        BindSSBO("TileWorldBounds", 6);
        BindSSBO("TileChristmasLights", 7);
        BindSSBO("ChristmasLightInstances", 8);
        BindSSBO("ChristmasLightIndices", 9);
        BindSSBO("ChristmasLightCounter", 10);

        glDispatchCompute(GetTileCountX(), GetTileCountY(), 1);
    }

    void BloodDecalTileCulling() {
        ProfilerOpenGLZoneFunction();

        OpenGLShader* shader = GetShaderOLD("BloodDecalsCulling");
        if (!shader) return;

        shader->Bind();
        shader->SetInt("u_decalCount", World::GetScreenSpaceBloodDecals().size());
        shader->SetInt("u_tileXCount", GetTileCountX());
        shader->SetInt("u_tileYCount", GetTileCountY());

        BindSSBO("TileWorldBounds", 6);
        BindSSBO("TileBloodDecals", 7);
        BindSSBO("BloodDecalInstances", 8);
        BindSSBO("BloodDecalIndices", 9);
        BindSSBO("BloodDecalCounter", 10);

        glDispatchCompute(GetTileCountX(), GetTileCountY(), 1);

        //if (Input::KeyPressed(HELL_KEY_SPACE)) {
        //    std::cout << "Blood count: " << World::GetScreenSpaceBloodDecals().size() << "\n";
        //}
    }
}