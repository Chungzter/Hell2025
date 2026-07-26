#include "EditorSession.h"

#include "EditorCoordinates.h"
#include "EditorHierarchy.h"
#include "EditorInputElements.h"
#include "EditorLayout.h"
#include "EditorMenuBar.h"
#include "EditorObjectProperties.h"
#include "EditorPlacement.h"
#include "EditorSelection.h"
#include "EditorViewports.h"
#include "EditorWorkspace.h"

#include "Hell/Backend/BackEnd.h"
#include "Hell/Input.h"
#include "Hell/UI/UIBackEnd.h"

#include "Unloved/Common/Constants.h"
#include "Unloved/Debug/DebugDraw.h"
#include "Unloved/Editor/Gizmo.h"
#include "Unloved/Objects/House/Door.h"
#include "Unloved/Objects/Props/GenericObject.h"
#include "Unloved/Objects/Props/PickUp.h"
#include "Unloved/World/World.h"

#include <algorithm>

namespace Unloved::EditorSession {
    namespace {
        bool g_isActive = false;

        void DrawGrid() {
            constexpr float GRID_EXTENT = 20.0f;
            constexpr float GRID_SPACING = 0.5f;
            constexpr float GRID_HEIGHT = -0.01f;

            // Sit the grid below world zero so it does not fight floor geometry
            for (float coordinate = -GRID_EXTENT; coordinate <= GRID_EXTENT; coordinate += GRID_SPACING) {
                DebugDraw::DrawLine(glm::vec3(coordinate, GRID_HEIGHT, -GRID_EXTENT), glm::vec3(coordinate, GRID_HEIGHT, GRID_EXTENT), GRID_COLOR, true);
                DebugDraw::DrawLine(glm::vec3(-GRID_EXTENT, GRID_HEIGHT, coordinate), glm::vec3(GRID_EXTENT, GRID_HEIGHT, coordinate), GRID_COLOR, true);
            }

            DebugDraw::DrawLine(glm::vec3(-GRID_EXTENT, GRID_HEIGHT, 0.0f), glm::vec3(GRID_EXTENT, GRID_HEIGHT, 0.0f), WHITE, true);
            DebugDraw::DrawLine(glm::vec3(0.0f, GRID_HEIGHT, -GRID_EXTENT), glm::vec3(0.0f, GRID_HEIGHT, GRID_EXTENT), WHITE, true);
        }

        void RefreshNativeLayout() {
            // The editor UI owns the whole native canvas
            UIBackEnd::SetCanvasResolution(UICanvas::NATIVE, static_cast<uint32_t>(std::max(1, Hell::BackEnd::GetDrawableWidth())), static_cast<uint32_t>(std::max(1, Hell::BackEnd::GetDrawableHeight())));
            Layout::Update();
        }

        void HandleMenuAction(MenuBar::EditorMenuAction action) {
            switch (action) {
                case MenuBar::EditorMenuAction::SAVE:         Workspace::Save(); break;
                case MenuBar::EditorMenuAction::CLOSE_EDITOR: Close(); break;
                case MenuBar::EditorMenuAction::VIEWPORT_SINGLE:     Layout::SetViewportLayout(EditorViewportLayout::SINGLE);     break;
                case MenuBar::EditorMenuAction::VIEWPORT_LEFT_RIGHT: Layout::SetViewportLayout(EditorViewportLayout::LEFT_RIGHT); break;
                case MenuBar::EditorMenuAction::VIEWPORT_TOP_BOTTOM: Layout::SetViewportLayout(EditorViewportLayout::TOP_BOTTOM); break;
                case MenuBar::EditorMenuAction::VIEWPORT_FOUR:       Layout::SetViewportLayout(EditorViewportLayout::FOUR);       break;
                default: break;
            }
        }

    }

    void Init() {
        InitPlacementTools();
        Selection::Reset();
        Viewports::Init();
        MenuBar::Init();
        Hierarchy::Init();
    }

    void Open() {
        Workspace::Discard();
        SetActive(true);
    }

    void Open(EditorSessionMode mode) {
        if (IsActive()) {
            if (Workspace::HasMode() && Workspace::GetMode() == mode) return;
            Close();
        }
        if (!Workspace::Open(mode)) return;

        if (mode == EditorSessionMode::MAP) {
            // Put authored pickups back and delete player dropped items
            const auto pickUpIds = World::GetPickUps().ids();
            for (uint64_t objectId : pickUpIds) {
                PickUp* pickUp = World::GetPickUpByObjectId(objectId);
                if (pickUp->GetRespawnState()) pickUp->Respawn();
                else World::RemoveObjectById(objectId);
            }
        }

        SetActive(true);
    }

    void Close() {
        if (!IsActive()) return;

        Workspace::Close();
        SetActive(false);

        // Push authored transforms back into PhysX before gameplay reads them
        for (GenericObject& genericObject : World::GetGenericObjects()) genericObject.ResetPhysics();
        for (PickUp& pickUp : World::GetPickUps()) {
            if (pickUp.GetRespawnState()) pickUp.Respawn();
        }

        // Doors always return to their authored start state when gameplay resumes
        for (Door& door : World::GetDoors()) door.Reset();
    }

    void SetActive(bool active) {
        // No interaction survives crossing the editor boundary
        Selection::Reset();
        InputElements::Reset();
        Placement::Cancel();

        if (!active) {
            Viewports::CancelNavigation();
            Gizmo::SetVisible(true);
        }

        g_isActive = active;

        if (g_isActive) {
            Hell::Input::ShowCursor();
        }
        else {
            Hell::Input::DisableCursor();
        }

        MenuBar::Close();
        Layout::CancelInteraction();
        Gizmo::CancelInteraction();

        if (!g_isActive) {
            UIBackEnd::ClearCanvas(UICanvas::NATIVE);
            return;
        }

        // Refresh scene backed UI only when entering the editor
        Hierarchy::Refresh();
        Gizmo::SetPosition(glm::vec3(0.0f));
        Gizmo::SetRotation(glm::vec3(0.0f));
        Gizmo::SetSourceObjectOffeset(glm::vec3(0.0f));
        RefreshNativeLayout();
        Viewports::ApplyInitialView();
        MenuBar::RefreshLayout();
    }

    void Update() {
        if (!IsActive()) return;

        // Tab cycles every viewport layout
        if (!WantsKeyboardCapture() && Hell::Input::KeyPressed(HELL_KEY_TAB)) {
            switch (Layout::GetViewportLayout()) {
                case EditorViewportLayout::SINGLE:     Layout::SetViewportLayout(EditorViewportLayout::LEFT_RIGHT); break;
                case EditorViewportLayout::LEFT_RIGHT: Layout::SetViewportLayout(EditorViewportLayout::TOP_BOTTOM); break;
                case EditorViewportLayout::TOP_BOTTOM: Layout::SetViewportLayout(EditorViewportLayout::FOUR);       break;
                case EditorViewportLayout::FOUR:       Layout::SetViewportLayout(EditorViewportLayout::SINGLE);     break;
            }
        }

        DrawGrid();
        RefreshNativeLayout();
        MenuBar::Update();
        HandleMenuAction(MenuBar::ConsumeAction());
        Placement::Begin(MenuBar::ConsumePlacementTool());

        if (!IsActive()) return;

        Layout::Update();
        Layout::UpdateDividerInput(!MenuBar::WantsMouseCapture());
        Hierarchy::Update(!Placement::IsActive() && !MenuBar::WantsMouseCapture() && !Layout::WantsMouseCapture());

        if (!Layout::WantsMouseCapture() && WantsMouseCapture()) Hell::BackEnd::SetCursor(HELL_CURSOR_ARROW);
    }

    void Render() {
        if (!IsActive()) return;

        Layout::RenderBackgrounds();
        Hierarchy::Render();
        ObjectProperties::Render(Layout::GetPropertiesContentRect());
        Layout::RenderOverlay();
        Viewports::RenderLabels();
        MenuBar::Render();
    }

    void UpdateViewportInput() {
        if (!IsActive()) return;

        Viewports::Update();
        // UI input wins before the viewport or gizmo sees it
        const bool allowMouseInput = !MenuBar::WantsMouseCapture() && !Layout::WantsMouseCapture() && !Hierarchy::WantsMouseCapture();
        const bool allowKeyboardInput = !MenuBar::WantsKeyboardCapture() && !InputElements::WantsKeyboardCapture();

        Viewports::UpdateInput(allowKeyboardInput, allowMouseInput);

        // Placement owns the click so the gizmo and selection never see it
        if (Placement::IsActive()) {
            Placement::Update(allowKeyboardInput && !Viewports::IsFlyMode(), allowMouseInput && !Viewports::IsFlyMode());
            Gizmo::SetVisible(false);
            Hell::BackEnd::SetCursor(Placement::IsActive() && allowMouseInput && !Viewports::IsFlyMode() && Viewports::GetHoveredViewportIndex() >= 0 ? HELL_CURSOR_CROSSHAIR : HELL_CURSOR_ARROW);
            return;
        }

        if (allowKeyboardInput && Selection::HasSelection() && (Hell::Input::KeyPressed(HELL_KEY_BACKSPACE) || Hell::Input::KeyPressed(HELL_KEY_DELETE))) {
            const uint64_t objectId = Selection::GetSelectedObjectId();
            const bool christmasLightPointSelected = Selection::HasSelectedChristmasLightPoint();
            if (Selection::DeleteSelected()) {
                if (christmasLightPointSelected) Hierarchy::RefreshChristmasLightPoints(objectId);
                else Hierarchy::RemoveObject(objectId);
            }
        }

        Gizmo::SetVisible(!Viewports::IsFlyMode() && Selection::HasSelection() && !Selection::HasSelectedWallSegment());
        Gizmo::Update(!Viewports::IsFlyMode() && allowKeyboardInput && allowMouseInput && Selection::HasSelection() && !Selection::HasSelectedWallSegment());
        Selection::Update(allowMouseInput);
    }

    bool IsActive() {
        return g_isActive;
    }

    bool IsInactive() {
        return !g_isActive;
    }

    bool HasMode() {
        return Workspace::HasMode();
    }

    EditorSessionMode GetMode() {
        return Workspace::GetMode();
    }

    bool WantsMouseCapture() {
        if (!IsActive()) return false;
        if (MenuBar::WantsMouseCapture()) return true;
        if (Layout::WantsMouseCapture()) return true;
        if (Hierarchy::WantsMouseCapture()) return true;
        if (Viewports::IsPanning() || Viewports::IsOrbiting() || Viewports::IsFlyMode()) return true;
        if (Gizmo::HasHover() || Gizmo::GetAction() == GizmoAction::DRAGGING) return true;

        // Mouse inside a viewport belongs to the scene
        const glm::ivec2 mousePosition = Coordinates::GetMousePositionUI();
        for (uint32_t i = 0; i < 4; i++) {
            const EditorViewportRegion* region = Layout::GetViewportRegionByIndex(i);
            if (region && region->visible && region->rect.Contains(mousePosition)) {
                return false;
            }
        }
        return true;
    }

    bool WantsKeyboardCapture() {
        return IsActive() && (Placement::IsActive() || MenuBar::WantsKeyboardCapture() || InputElements::WantsKeyboardCapture() || Viewports::IsFlyMode());
    }
}
