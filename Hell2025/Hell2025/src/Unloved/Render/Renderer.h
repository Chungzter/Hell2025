#pragma once
#include "Hell/Math/AABB.h"
#include "Hell/Math/OBB.h"

#include "Unloved/Render/RendererEnums.h"
#include "Unloved/Render/RendererSettings.h"
#include "Unloved/Maps/MapData.h"

#include "Unloved/Common/Types.h"

namespace Unloved::Renderer {
    void Init();
    void InitMain();
    void CleanUp();
    void InitWoundMaskArray();
    void RenderLoadingScreen();
    void PreGameLogicComputePasses();
    void RenderGame();
    void HotloadShaders();

    // Override states
    void SetRendererOverrideState(RendererOverrideState state);
    void NextRendererOverrideState();
    bool OverrideStateUsesDebugViewPass();

    void SetProbeDebugState(ProbeDebugState state);
	void NextProbeDebugState();

    // Debug toggles
    void ToggleDebugDraw();
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
    void ReadBackHeightMapData(Unloved::MapData* mapData);

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
