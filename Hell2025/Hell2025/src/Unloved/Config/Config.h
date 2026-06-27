#pragma once
#include "Unloved/Common/Types.h"

namespace Config {
    void Init();
	const Resolutions& GetResolutions();
	const float GetNearPlane();
	const float GetFarPlane();
    //void SetDepthPeelCount(int count);
}