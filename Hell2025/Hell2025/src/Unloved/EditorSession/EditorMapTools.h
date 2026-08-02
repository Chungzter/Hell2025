#pragma once

#include "EditorSessionTypes.h"

#include <cstdint>

namespace Unloved::EditorSession::MapTools {

    enum class Mode : uint8_t {
        OBJECT,
        HEIGHT_MAP
    };

    enum class HeightMapTool : uint8_t {
        ADD,
        FLAT,
        SLOPE,
        SMOOTH,
        TEXTURE_PAINT,
        TEXTURE_SPRAY,
        AUTO_SHADER
    };

    void Init();
    void Reset();
    void Update(bool allowInput);
    void Render();

    void SetMode(Mode mode);
    Mode GetMode();
    EditorRenderMode GetRenderMode();
    HeightMapTool GetHeightMapTool();
    bool IsTerrainLayersOpen();
    bool WantsMouseCapture();
}
