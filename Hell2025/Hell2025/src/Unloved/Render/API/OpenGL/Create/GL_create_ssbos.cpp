#include "Hell/Common/Constants.h"

#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Unloved/Render/Renderer.h"
#include "Unloved/Render/RendererConstants.h"
#include "Unloved/Render/RendererTypes.h"
#include "Unloved/Systems/Ocean/Ocean.h"

namespace OpenGL::Renderer{

    void CreateSSBOs() {
        GLbitfield staticFlags = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
        GLbitfield dynamicFlags = GL_DYNAMIC_STORAGE_BIT | GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;

        // Ocean
        const glm::uvec2 oceanSize = Ocean::GetBaseFFTResolution(); // WARNING!!! This size must bit your largest FFT dimensions
        OpenGL::ResourceManager::CreateSSBO("ffth0Band0").Create(Ocean::GetFFTResolution(0).x * Ocean::GetFFTResolution(0).y * sizeof(std::complex<float>), staticFlags);
        OpenGL::ResourceManager::CreateSSBO("ffth0Band1").Create(Ocean::GetFFTResolution(1).x * Ocean::GetFFTResolution(1).y * sizeof(std::complex<float>), staticFlags);
        OpenGL::ResourceManager::CreateSSBO("fftSpectrumInSSBO").Create(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);
        OpenGL::ResourceManager::CreateSSBO("fftSpectrumOutSSBO").Create(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);
        OpenGL::ResourceManager::CreateSSBO("fftDispInXSSBO").Create(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);
        OpenGL::ResourceManager::CreateSSBO("fftDispZInSSBO").Create(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);
        OpenGL::ResourceManager::CreateSSBO("fftGradXInSSBO").Create(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);
        OpenGL::ResourceManager::CreateSSBO("fftGradZInSSBO").Create(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);
        OpenGL::ResourceManager::CreateSSBO("fftDispXOutSSBO").Create(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);
        OpenGL::ResourceManager::CreateSSBO("fftDispZOutSSBO").Create(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);
        OpenGL::ResourceManager::CreateSSBO("fftGradXOutSSBO").Create(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);
        OpenGL::ResourceManager::CreateSSBO("fftGradZOutSSBO").Create(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);

        int dummySize = 64;

        // Core
        OpenGL::ResourceManager::CreateSSBO("Samplers").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("ViewportData").Create(sizeof(ViewportData) * 4, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("RendererData").Create(sizeof(RendererData), GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("InstanceData").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("Lights").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);

        // Skinning
        OpenGL::ResourceManager::CreateSSBO("SkinningDispatchGroups").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("SkinningJobs").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("SkinningTransforms").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);

        OpenGL::ResourceManager::CreateSSBO("Materials").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);

        OpenGL::ResourceManager::CreateSSBO("RenderItemsUI").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);

        // Vertices
        OpenGL::ResourceManager::CreateSSBO("Indices2");
        OpenGL::ResourceManager::CreateSSBO("Vertices2");

        // Raytracing
        OpenGL::ResourceManager::CreateSSBO("TriangleData").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("SceneBvh").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("MeshesBvh").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("EntityInstances").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("PointGridBuffer").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("PointIndicesBuffer").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);

        // DDGI
        OpenGL::ResourceManager::CreateSSBO("DDGIVolume").Create(sizeof(DDGIVolumeGPU), GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("DirtyDoorAABBs").Create(sizeof(GPUAABB), GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("PointCloudGridCounts").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("PointCloudDirtyFlags").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("PointCloudGridOffsets").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("PointCloudTextureInfo").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("ProbeDistanceCounter").Create(sizeof(uint32_t), GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("ProbeDistanceDispatchArgs").Create(sizeof(DispatchIndirectCommand), GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("ProbeDistanceIndices").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("ProbeIndexCounter").Create(sizeof(uint32_t), GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("ProbeIrradianceCounter").Create(sizeof(uint32_t), GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("ProbeIrradianceDispatchArgs").Create(sizeof(DispatchIndirectCommand), GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("ProbeIrradianceIndices").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("ProbePointIndices").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("ProbePointOffsets").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("ProbePointCounts").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("ProbeSHColor").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("ProbeStates").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);

        OpenGL::ResourceManager::CreateSSBO("LightAABBs").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);

        // Tile data
        OpenGL::ResourceManager::CreateSSBO("TileChristmasLights").Create(GetTileCount() * sizeof(TileInstanceData), HELL_NONE_BIT);
        OpenGL::ResourceManager::CreateSSBO("TileBloodDecals").Create(GetTileCount() * sizeof(TileInstanceData), HELL_NONE_BIT);
        OpenGL::ResourceManager::CreateSSBO("TileLights").Create(GetTileCount() * sizeof(TileLights), HELL_NONE_BIT);
        OpenGL::ResourceManager::CreateSSBO("TileWorldBounds").Create(GetTileCount() * sizeof(TileWorldBounds), HELL_NONE_BIT);

        // Instance data
        OpenGL::ResourceManager::CreateSSBO("BloodDecalCounter").Create(sizeof(uint32_t), GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("BloodDecalIndices").Create(sizeof(uint32_t) * GetTileCount() * 256, HELL_NONE_BIT);
        OpenGL::ResourceManager::CreateSSBO("BloodDecalInstances").Create(sizeof(BloodDecalInstanceData) * MAX_SCREEN_SPACE_BLOOD_DECAL_COUNT, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("ChristmasLightCounter").Create(sizeof(uint32_t), GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("ChristmasLightIndices").Create(sizeof(uint32_t) * GetTileCount() * 256, HELL_NONE_BIT);
        OpenGL::ResourceManager::CreateSSBO("ChristmasLightInstances").Create(MAX_CHRISTMAS_LIGHTS * sizeof(GPUChristmasLight), GL_DYNAMIC_STORAGE_BIT);

        // Remove me at some point
        OpenGL::ResourceManager::CreateSSBO("MetaBalls").Create(sizeof(glm::vec4) * 1000, GL_DYNAMIC_STORAGE_BIT);

        int MAX_OCEAN_PATCHES = 500;
        OpenGL::ResourceManager::CreateSSBO("OceanPatchTransforms").Create(sizeof(glm::mat4) * MAX_OCEAN_PATCHES, GL_DYNAMIC_STORAGE_BIT);

        // Preallocate the indirect command buffer
        IndirectBuffer& indirectBuffer = GetIndirectBuffer();
        indirectBuffer.PreAllocate(sizeof(DrawIndexedIndirectCommand) * MAX_INDIRECT_DRAW_COMMAND_COUNT);
    }
}
