#pragma once
#include "RendererEnums.h"

struct RendererSettings {
    int depthPeelCount = 3;
    bool drawGrass = true;
    bool screenspaceReflections = true;
    bool debugDrawPointCloud = false;
    bool debugDrawPointCloudGrid = false;
    bool debugDrawIrradianceProbes = false;
    bool debugDrawNavMesh = false;
    bool debugDrawRagdolls = false;
    bool enableIrradianceProbeSampling = true;
    bool enableLighting = true;
    bool irradianceUsesSH = true;
    RendererOverrideState rendererOverrideState = RendererOverrideState::NONE;
    ProbeDebugState probeDebugState = ProbeDebugState::HIDDEN;
};