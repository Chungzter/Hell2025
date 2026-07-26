#pragma once

#include <cstdint>

namespace Unloved::EditorSession::Selection {

    void Reset();
    void Update(bool allowInput);
    void SelectObject(uint64_t objectId);
    void SelectChristmasLightPoint(uint64_t objectId, int32_t pointIndex);
    void SelectWallSegment(uint64_t objectId, int32_t segmentIndex);
    bool DeleteSelected();
    void ClearSelection();

    uint64_t GetHoveredObjectId();
    uint64_t GetSelectedObjectId();
    int32_t GetSelectedChristmasLightPointIndex();
    int32_t GetSelectedWallSegmentIndex();
    bool HasSelectedChristmasLightPoint();
    bool HasSelectedWallSegment();
    bool HasSelection();
    bool ShouldOutlineObject(uint64_t objectId);
}
