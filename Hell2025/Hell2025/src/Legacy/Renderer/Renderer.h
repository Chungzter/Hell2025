#pragma once
#include "Hell/Math/AABB.h"
#include "Hell/Math/OBB.h"
#include "Types/Map/Map.h"

#include <Game/Types.h>

namespace Renderer {
    void InitMain();
    void InitWoundMaskArray();
    void RenderLoadingScreen();
    void PreGameLogicComputePasses();
    void RenderGame();
    void HotloadShaders();
    void UploadVertexData();

    // Override states
    void SetRendererOverrideState(RendererOverrideState state);
    void NextRendererOverrideState();

    void SetProbeDebugState(ProbeDebugState state);
	void NextProbeDebugState();

    // Debug toggles
    void ToggleLighting();
    void ToggleOverrideState(RendererOverrideState state);
    void ToggleIrradianceProbeSampling();
    void TogglePointCloud();
    void TogglePointCloudGrid();
    void ToggleRagdollRendering();
    void ToggleScreenSpaceReflections();
    void ToggleSphericalHarmonics();

    void NextRendererMode();
	void SetRendererMode(RendererMode rendererMode);
	RendererMode GetRendererMode();

    int32_t GetNextFreeWoundMaskIndexAndMarkItTaken();
    void MarkWoundMaskIndexAsAvailable(int32_t index);

    void RecalculateAllHeightMapData(bool blitWorldMap);
    void ReadBackHeightMapData(Map* map);

	uint32_t GetTileCount();
	uint32_t GetTileCountX();
	uint32_t GetTileCountY();

    RendererSettings& GetCurrentRendererSettings();

    const std::string& GetZoneNames();
    const std::string& GetZoneGPUTimings();
    const std::string& GetZoneCPUTimings();
    const std::string& GetTotalGPUTime();
    const std::string& GetTotalCPUTime();

    bool GameIsRendering();
}