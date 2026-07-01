#include "../GL_renderer.h"
#include "Renderer/Renderer.h"
#include "Unloved/Systems/Blood/BloodSystem.h"
#include "World/LegacyWorld.h"
#include "Unloved/World/World.h"
namespace OpenGLRenderer {
    using namespace Unloved;

    std::vector<GPUChristmasLight> g_gpuLights;

    void ComputeTileWorldBounds() {
        ProfilerOpenGLZoneFunction();

        uint32_t depthHandle = 0;

		switch (Renderer::GetRendererMode()) {
		    case RendererMode::OLD_DEFERRED: depthHandle = OpenGL::ResourceManager::GetFrameBuffer("GBuffer").GetDepthAttachmentHandle();   break;
            case RendererMode::RE_STYLE:     depthHandle = OpenGL::ResourceManager::GetFrameBuffer("GBufferRE").GetDepthAttachmentHandle(); break;
		}

        OpenGL::BindShader("TileWorldBounds");
        OpenGL::SetUniformInt("u_tileXCount", GetTileCountX());
		OpenGL::SetUniformInt("u_tileYCount", GetTileCountY());

        OpenGL::BindSSBO(1, "RendererData");
        OpenGL::BindSSBO(2, "ViewportData");
        OpenGL::BindSSBO(6, "TileWorldBounds");

        OpenGL::BindTextureUnit(0, depthHandle);

        OpenGL::DispatchCompute(GetTileCountX(), GetTileCountY(), 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    void LightCullingPass() {
        ProfilerOpenGLZoneFunction();

        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("LightCulling");

        if (!shader) return;

        OpenGL::BindShader("LightCulling");
        OpenGL::SetUniformInt("u_lightCount", Unloved::World::GetLights().size());
        OpenGL::SetUniformInt("u_tileXCount", GetTileCountX());
        OpenGL::SetUniformInt("u_tileYCount", GetTileCountY());

        OpenGL::BindSSBO(1, "RendererData");
        OpenGL::BindSSBO(2, "ViewportData");
        OpenGL::BindSSBO(4, "Lights");
        OpenGL::BindSSBO(5, "TileLights");
        OpenGL::BindSSBO(6, "TileWorldBounds");

        OpenGL::DispatchCompute(GetTileCountX(), GetTileCountY(), 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    void ChristmasLightCullingPass() {
        ProfilerOpenGLZoneFunction();

        // Clear the lights from last frame, coz they change
        g_gpuLights.clear();

        // Gather all the Christmas lights from ALL the ChristmasLightSets
        Hell::SlotMap<ChristmasLightSet>& christmasLightSets = Unloved::World::GetChristmasLightSets();

        for (ChristmasLightSet& christmasLightSet : christmasLightSets) {
            const std::vector<GPUChristmasLight>& gpuLights = christmasLightSet.GetGPUChristmasLights();
            g_gpuLights.insert(g_gpuLights.end(), gpuLights.begin(), gpuLights.end());
        }

        OpenGL::UpdateSSBO("ChristmasLightInstances", g_gpuLights.size() * sizeof(GPUChristmasLight), (void*)&g_gpuLights[0]);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        // Debug draw the lights as points
        //for (const GPUChristmasLight& light : g_gpuLights) {
        //    DebugDraw::DrawPoint(light.position, light.color);
        //}

        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("ChristmasLightCulling");
        if (!shader) return;

        OpenGL::BindShader("ChristmasLightCulling");
        OpenGL::SetUniformInt("u_christmasLightCount", g_gpuLights.size());
        OpenGL::SetUniformInt("u_tileXCount", GetTileCountX());
        OpenGL::SetUniformInt("u_tileYCount", GetTileCountY());

        OpenGL::BindSSBO(6, "TileWorldBounds");
        OpenGL::BindSSBO(7, "TileChristmasLights");
        OpenGL::BindSSBO(8, "ChristmasLightInstances");
        OpenGL::BindSSBO(9, "ChristmasLightIndices");
        OpenGL::BindSSBO(10, "ChristmasLightCounter");

        OpenGL::DispatchCompute(GetTileCountX(), GetTileCountY(), 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    void BloodDecalTileCulling() {
        ProfilerOpenGLZoneFunction();

        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("BloodDecalsCulling");
        if (!shader) return;

        OpenGL::BindShader("BloodDecalsCulling");
        OpenGL::SetUniformInt("u_decalCount", static_cast<int>(Unloved::BloodSystem::GetBloodScreenSpaceDecals().size()));
        OpenGL::SetUniformInt("u_tileXCount", GetTileCountX());
        OpenGL::SetUniformInt("u_tileYCount", GetTileCountY());

        OpenGL::BindSSBO(6, "TileWorldBounds");
        OpenGL::BindSSBO(7, "TileBloodDecals");
        OpenGL::BindSSBO(8, "BloodDecalInstances");
        OpenGL::BindSSBO(9, "BloodDecalIndices");
        OpenGL::BindSSBO(10, "BloodDecalCounter");

        OpenGL::DispatchCompute(GetTileCountX(), GetTileCountY(), 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        //if (Input::KeyPressed(HELL_KEY_SPACE)) {
        //    std::cout << "Blood count: " << Unloved::BloodSystem::GetBloodScreenSpaceDecals().size() << "\n";
        //}
    }
}
