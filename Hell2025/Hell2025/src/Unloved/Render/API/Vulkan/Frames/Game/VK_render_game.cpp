#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"

#include "Unloved/Render/Renderer.h"
#include "Unloved/Systems/DDGI/DDGIManager.h"

namespace VulkanRenderer {
    namespace {
        void SubmitDDGIGridDebugDraw() {
            const RendererSettings& rendererSettings = Unloved::Renderer::GetCurrentRendererSettings();
            if (!rendererSettings.debugDrawPointCloudGrid) return;

            Hell::SlotMap<Unloved::DDGIVolume>& ddgiVolumes = Unloved::DDGIManager::GetVolumes();
            for (Unloved::DDGIVolume& ddgiVolume : ddgiVolumes) {
                ddgiVolume.GetPointClound().DebugDrawGrid();
            }
        }
    }

    void RenderGame() {
        SwapchainFrame frame;
        if (!BeginSwapchainFrame(frame)) return;

        SubmitDDGIGridDebugDraw();
        UpdateBuffers();
        UpdateBuffersUI();

        ComputeSkinningPass(frame.commandBuffer);
        UpdateRayTracing(frame.commandBuffer);
        DDGIPointCloudPass(frame.commandBuffer);

        VisibilityPass(frame.commandBuffer);
        MaterialResolvePass(frame.commandBuffer);
        DDGIProbeUpdatePass(frame.commandBuffer);
        DDGIIrradianceTexturePass(frame.commandBuffer);

        ComputeTileWorldBounds(frame.commandBuffer);
        LightCullingPass(frame.commandBuffer);

        ReflectedRadiancePass(frame.commandBuffer);

        LightingPass(frame.commandBuffer);

        LightingForwardBlendedPass(frame.commandBuffer);
        SkyboxPass(frame.commandBuffer);
        // HairPass(frame.commandBuffer);
        SpriteSheetPass(frame.commandBuffer); // Muzzle flash, etc

        PostProcessingPass(frame.commandBuffer);

        DebugViewPass(frame.commandBuffer);
        DebugTileViewPass(frame.commandBuffer);
        DDGIRaytraceScenePass(frame.commandBuffer);
        DDGIPointCloudDebugPass(frame.commandBuffer);
        DDGIProbeDebugPass(frame.commandBuffer);
        DebugPass(frame.commandBuffer);

        BlitImage(frame.commandBuffer, "Lighting", "FinalImage", VK_FILTER_LINEAR);
        BlitImage(frame.commandBuffer, "FinalImage", "Present", VK_FILTER_NEAREST);

        RenderUIPass(frame.commandBuffer);

        PresentPass(frame.commandBuffer, frame.swapchainImageView);
        EndSwapchainFrame(frame);
    }
}
