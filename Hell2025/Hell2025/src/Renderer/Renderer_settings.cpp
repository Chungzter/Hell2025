#include "Renderer.h"
#include "Audio/Audio.h"
#include "Core/Debug.h"
#include "Editor/Editor.h"
#include "Util/Util.h"

namespace Renderer {
    struct RendererSettingsSet {
        RendererSettings game;
        RendererSettings houseEditor;
        RendererSettings mapHeightEditor;
        RendererSettings mapObjectEditor;
    } g_rendererSettingsSet;

    RendererMode g_rendererMode = RendererMode::OLD_DEFERRED;

    RendererSettings& GetCurrentRendererSettings() {
        if (Editor::IsOpen()) {
            switch (Editor::GetEditorMode()) {
            case EditorMode::HOUSE_EDITOR:      return g_rendererSettingsSet.houseEditor;
            case EditorMode::MAP_HEIGHT_EDITOR: return g_rendererSettingsSet.mapHeightEditor;
            case EditorMode::MAP_OBJECT_EDITOR: return g_rendererSettingsSet.mapObjectEditor;
            }
        }
        return g_rendererSettingsSet.game;
    }

    void ToggleLighting() {
        Audio::PlayAudio(AUDIO_SELECT, 1.00f);
        RendererSettings& rendererSettings = GetCurrentRendererSettings();
        rendererSettings.enableLighting = !rendererSettings.enableLighting;

        std::string onOff = rendererSettings.enableLighting ? "ON" : "OFF";
        Debug::BlitQuickDebugMessage("Lighting: " + onOff);
    }

    void TogglePointCloud() {
        Audio::PlayAudio(AUDIO_SELECT, 1.00f);
        RendererSettings& rendererSettings = GetCurrentRendererSettings();
        rendererSettings.debugDrawPointCloud = !rendererSettings.debugDrawPointCloud;

        if (rendererSettings.debugDrawPointCloud) {
            rendererSettings.debugDrawPointCloudGrid = false;
        }

        std::string onOff = rendererSettings.debugDrawPointCloud ? "ON" : "OFF";
        Debug::BlitQuickDebugMessage("Point Cloud: " + onOff);
    }

    void TogglePointCloudGrid() {
        Audio::PlayAudio(AUDIO_SELECT, 1.00f);
        RendererSettings& rendererSettings = GetCurrentRendererSettings();
        rendererSettings.debugDrawPointCloudGrid = !rendererSettings.debugDrawPointCloudGrid;

        if (rendererSettings.debugDrawPointCloudGrid) {
            rendererSettings.debugDrawPointCloud = false;
        }

        std::string onOff = rendererSettings.debugDrawPointCloudGrid ? "ON" : "OFF";
        Debug::BlitQuickDebugMessage("Point Cloud Grid: " + onOff);
    }

    void ToggleSphericalHarmonics() {
        Audio::PlayAudio(AUDIO_SELECT, 1.00f);
        RendererSettings& rendererSettings = GetCurrentRendererSettings();
        rendererSettings.irradianceUsesSH = !rendererSettings.irradianceUsesSH;

        std::string onOff = rendererSettings.irradianceUsesSH ? "SPHERICAL HARMONICS" : "OCTAL MAPPING";
        Debug::BlitQuickDebugMessage("Irradiance path: " + onOff);
    }

    void ToggleScreenSpaceReflections() {
        Audio::PlayAudio(AUDIO_SELECT, 1.00f);
        RendererSettings& rendererSettings = GetCurrentRendererSettings();
        rendererSettings.screenspaceReflections = !rendererSettings.screenspaceReflections;

        std::string onOff = rendererSettings.screenspaceReflections ? "ON" : "OFF";
        Debug::BlitQuickDebugMessage("Screenspace Reflections: " + onOff);
    }

    void ToggleIrradianceProbeSampling() {
        Audio::PlayAudio(AUDIO_SELECT, 1.00f);
        RendererSettings& rendererSettings = GetCurrentRendererSettings();
        rendererSettings.enableIrradianceProbeSampling = !rendererSettings.enableIrradianceProbeSampling;

        std::string onOff = rendererSettings.enableIrradianceProbeSampling ? "ON" : "OFF";
        Debug::BlitQuickDebugMessage("Irradiance Probe Sampling: " + onOff);
    }

    void ToggleOverrideState(RendererOverrideState state) {
        Audio::PlayAudio(AUDIO_SELECT, 1.00f);
        RendererSettings& rendererSettings = GetCurrentRendererSettings();
        if (rendererSettings.rendererOverrideState == state) {
            SetRendererOverrideState(RendererOverrideState::NONE);
        }
        else {
            SetRendererOverrideState(state);
        }
    }

    void SetRendererOverrideState(RendererOverrideState state) {
        RendererSettings& rendererSettings = GetCurrentRendererSettings();
        rendererSettings.rendererOverrideState = state;

        Debug::BlitQuickDebugMessage("Renderer Override State: " + Util::EnumToString(state));
        Audio::PlayAudio(AUDIO_SELECT, 1.00f);
    }

    void NextRendererOverrideState() {
        RendererSettings& rendererSettings = GetCurrentRendererSettings();
        int i = static_cast<int>(rendererSettings.rendererOverrideState);
        i = (i + 1) % static_cast<int>(RendererOverrideState::STATE_COUNT);

        SetRendererOverrideState(static_cast<RendererOverrideState>(i));
    }

	void NextProbeDebugState() {
		RendererSettings& rendererSettings = GetCurrentRendererSettings();
		int i = static_cast<int>(rendererSettings.probeDebugState);
		i = (i + 1) % static_cast<int>(ProbeDebugState::STATE_COUNT);

		SetProbeDebugState(static_cast<ProbeDebugState>(i));
    }

	void NextRendererMode() {
		int i = static_cast<int>(g_rendererMode);
		i = (i + 1) % static_cast<int>(RendererMode::RENDERER_MODE_COUNT);
		SetRendererMode(static_cast<RendererMode>(i));
	}

	void SetRendererMode(RendererMode rendererMode) {
        g_rendererMode = rendererMode;

		Debug::BlitQuickDebugMessage("Renderer Mode: " + Util::EnumToString(rendererMode));
		Audio::PlayAudio(AUDIO_SELECT, 1.00f);
	}

    RendererMode GetRendererMode() {
        return g_rendererMode;
    }

	void SetProbeDebugState(ProbeDebugState state) {
		RendererSettings& rendererSettings = GetCurrentRendererSettings();
		rendererSettings.probeDebugState = state;

        rendererSettings.debugDrawIrradianceProbes = rendererSettings.probeDebugState != ProbeDebugState::HIDDEN;

		Debug::BlitQuickDebugMessage("Irradiance Probes: " + Util::EnumToString(state));
		Audio::PlayAudio(AUDIO_SELECT, 1.00f);
	}
}