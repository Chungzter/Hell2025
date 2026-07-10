#include "Legacy/World/LegacyWorld.h"

#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Unloved/Render/Renderer.h"
#include "Unloved/Render/RendererTypes.h"
#include "Unloved/Systems/Blood/BloodSystem.h"
#include "Unloved/World/World.h"

namespace OpenGL::Renderer {

    namespace {
        std::vector<GPUChristmasLight> g_christmasLights;
    }

    void ComputeTileWorldBounds() {
        ProfilerOpenGLZoneFunction();

        uint32_t depthHandle = 0;

		switch (Unloved::Renderer::GetRendererMode()) {
		    case RendererMode::OLD_DEFERRED: depthHandle = OpenGL::ResourceManager::GetFrameBuffer("GBuffer").GetDepthAttachmentHandle();   break;
            case RendererMode::RE_STYLE:     depthHandle = OpenGL::ResourceManager::GetFrameBuffer("GBufferRE").GetDepthAttachmentHandle(); break;
		}

        OpenGL::BindShader("TileWorldBounds");
        OpenGL::SetUniformInt("u_tileXCount", GetTileCountX());
		OpenGL::SetUniformInt("u_tileYCount", GetTileCountY());

        OpenGL::BindSSBO(2, "RendererData");
        OpenGL::BindSSBO(3, "ViewportData");
        OpenGL::BindSSBO(7, "TileWorldBounds");

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

        OpenGL::BindSSBO(2, "RendererData");
        OpenGL::BindSSBO(3, "ViewportData");
        OpenGL::BindSSBO(5, "Lights");
        OpenGL::BindSSBO(6, "TileLights");
        OpenGL::BindSSBO(7, "TileWorldBounds");

        OpenGL::DispatchCompute(GetTileCountX(), GetTileCountY(), 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    void ChristmasLightCullingPass() {
        ProfilerOpenGLZoneFunction();

        // Clear the lights from last frame, coz they change
        g_christmasLights.clear();

        // Gather all the Christmas lights from ALL the ChristmasLightSets
        for (Unloved::ChristmasLightSet& christmasLightSet : Unloved::World::GetChristmasLightSets()) {
            const std::vector<GPUChristmasLight>& gpuLights = christmasLightSet.GetGPUChristmasLights();
            g_christmasLights.insert(g_christmasLights.end(), gpuLights.begin(), gpuLights.end());
        }

        // If no Christmas lights found, then initialize the SSBOs to zero
        if (g_christmasLights.empty()) {
            OpenGL::ClearSSBO("TileChristmasLights");
            OpenGL::ClearSSBO("ChristmasLightInstances");
            OpenGL::ClearSSBO("ChristmasLightIndices");
            OpenGL::ClearSSBO("ChristmasLightCounter");
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

            return;
        }


        OpenGL::UpdateSSBO("ChristmasLightInstances", g_christmasLights.size() * sizeof(GPUChristmasLight), g_christmasLights.data());
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        // Debug draw the lights as points
        //for (const GPUChristmasLight& light : g_gpuLights) {
        //    DebugDraw::DrawPoint(light.position, light.color);
        //}

        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("ChristmasLightCulling");
        if (!shader) return;

        OpenGL::BindShader("ChristmasLightCulling");
        OpenGL::SetUniformInt("u_christmasLightCount", g_christmasLights.size());
        OpenGL::SetUniformInt("u_tileXCount", GetTileCountX());
        OpenGL::SetUniformInt("u_tileYCount", GetTileCountY());

        OpenGL::BindSSBO(7, "TileWorldBounds");
        OpenGL::BindSSBO(9, "TileChristmasLights");
        OpenGL::BindSSBO(10, "ChristmasLightInstances");
        OpenGL::BindSSBO(11, "ChristmasLightIndices");
        OpenGL::BindSSBO(12, "ChristmasLightCounter");

        OpenGL::DispatchCompute(GetTileCountX(), GetTileCountY(), 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    void BloodDecalTileCulling() {
        ProfilerOpenGLZoneFunction();

        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("BloodDecalsCulling");
        if (!shader) return;

        OpenGL::BindShader("BloodDecalsCulling");
        OpenGL::SetUniformInt("u_decalCount", static_cast<int>(Unloved::BloodSystem::GetBloodScreenSpaceDecals().size()));
        OpenGL::SetUniformInt("u_tileXCount", static_cast<int>(GetTileCountX()));
        OpenGL::SetUniformInt("u_tileYCount", static_cast<int>(GetTileCountY()));

        OpenGL::BindSSBO(7, "TileWorldBounds");
        OpenGL::BindSSBO(8, "TileBloodDecals");
        OpenGL::BindSSBO(9, "BloodDecalInstances");
        OpenGL::BindSSBO(10, "BloodDecalIndices");
        OpenGL::BindSSBO(11, "BloodDecalCounter");

        OpenGL::DispatchCompute(GetTileCountX(), GetTileCountY(), 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        //if (Input::KeyPressed(HELL_KEY_SPACE)) {
        //    std::cout << "Blood count: " << Unloved::BloodSystem::GetBloodScreenSpaceDecals().size() << "\n";
        //}
    }
}
