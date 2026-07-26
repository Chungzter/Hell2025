#pragma once

#include "EditorSessionTypes.h"

namespace Unloved::EditorSession::Layout {

    void Update();
    void UpdateDividerInput(bool allowInput);
    void RenderBackgrounds();
    void RenderOverlay();
    void CancelInteraction();

    void SetFileMenuHeight(int32_t height);
    void SetHierarchyWidth(int32_t width);
    void SetPropertiesWidth(int32_t width);
    void SetViewportLayout(EditorViewportLayout layout);
    void SetPanelEdges(EditorPanelId panelId, EditorPanelEdge edges);

    int32_t GetFileMenuHeight();
    int32_t GetHierarchyWidth();
    int32_t GetPropertiesWidth();
    uint32_t GetViewportCount();
    EditorViewportLayout GetViewportLayout();
    bool WantsMouseCapture();

    const EditorPanel& GetFileMenuPanel();
    const EditorPanel& GetHierarchyPanel();
    const EditorPanel& GetViewportsPanel();
    const EditorPanel& GetPropertiesPanel();
    EditorRect GetHierarchyContentRect();
    EditorRect GetPropertiesContentRect();
    const EditorViewportRegion* GetViewportRegionByIndex(uint32_t index);
}
