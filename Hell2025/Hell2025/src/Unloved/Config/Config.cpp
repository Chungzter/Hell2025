#include "Config.h"

namespace Config {
	Resolutions g_resolutions;
	float g_nearPlane = 0.005f;
	float g_farPlane = 256.00f;

    void Init() {
        g_resolutions.gBuffer = { 1920, 1080 };
        g_resolutions.gBufferHalfRes = g_resolutions.gBuffer / 2;
        g_resolutions.finalImage = { 1920 / 2, 1080 / 2 };
        g_resolutions.ui = { 1920, 1080 };
        g_resolutions.hair = { 1920 / 2, 1080 / 2 };
    }

    const Resolutions& GetResolutions() {
        return g_resolutions;
    }

    const float GetNearPlane() {
        return g_nearPlane;
    }

    const float GetFarPlane() {
        return g_farPlane;
    }
}