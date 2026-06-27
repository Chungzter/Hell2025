#pragma once
#include "DebugTypes.h"
#include <Unloved/Render/RendererEnums.h>

#include <string>

namespace Debug {
    void Update();
    void AddText(const std::string& text);
    void BlitQuickDebugMessage(const std::string& message);
    void EndFrame();
	void NextDebugRenderMode();
	void NextDebugTextMode();
    void SetDebugRenderMode(DebugRenderMode mode);

    void PrintModelMeshNames(const std::string& name);

    const std::string& GetText();
    const DebugRenderMode& GetDebugRenderMode();
    const DebugTextMode& GetDebugTextMode();
}