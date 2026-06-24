#pragma once
#include <Game/Types.h>

namespace Config {
    void Init();
	const Resolutions& GetResolutions();
	const float GetNearPlane();
	const float GetFarPlane();
    //void SetDepthPeelCount(int count);
}