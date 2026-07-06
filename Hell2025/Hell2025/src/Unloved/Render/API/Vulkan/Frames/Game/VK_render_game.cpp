#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"

namespace VulkanRenderer {

    void RenderGame() {
        SwapchainFrame frame;
        if (!BeginSwapchainFrame(frame)) return;
        
        UpdateBuffers();
        UpdateBuffersUI();

        ComputeSkinningPass(frame.commandBuffer);

        VisibilityPass(frame.commandBuffer);
        MaterialResolvePass(frame.commandBuffer);
        UpdateRayQueryAccelerationStructures(frame.commandBuffer);
        LightingPass(frame.commandBuffer);

        BlitImage(frame.commandBuffer, "Lighting", "FinalImage", VK_FILTER_LINEAR);
        BlitImage(frame.commandBuffer, "FinalImage", "Present", VK_FILTER_NEAREST);

        RenderUIPass(frame.commandBuffer);

        PresentPass(frame.commandBuffer, frame.swapchainImageView);
        EndSwapchainFrame(frame);
    }
}
