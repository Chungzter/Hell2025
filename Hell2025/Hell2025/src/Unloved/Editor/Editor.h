#pragma once

#include "Unloved/Editor/ObjectNames.h"
#include "Unloved/Camera/Camera.h"
#include "Unloved/Common/CreateInfo.h"
#include "Unloved/Common/Types.h"
#include "Unloved/Viewport/Viewport.h"

#include "Unloved/ObjectId/ObjectId_types.h"

#include <string>

namespace Unloved::Editor {

    struct PlacementObjectSubtype {
        GenericObjectType genericObject = GenericObjectType::UNDEFINED;
        FireplaceType fireplace = FireplaceType::UNDEFINED;
        WorldPlaneType housePlane = WorldPlaneType::UNDEFINED;
        std::string pickUpName = UNDEFINED_STRING;
        std::string defaultEditorName = UNDEFINED_STRING;

        void Reset() {
            genericObject = GenericObjectType::UNDEFINED;
            fireplace = FireplaceType::UNDEFINED;
            housePlane = WorldPlaneType::UNDEFINED;
            pickUpName = UNDEFINED_STRING;
            defaultEditorName = UNDEFINED_STRING;

        }
    };

    void Init();
    void ResetCameras();
    void ResetViewports();
    void Update(float deltaTime);
    void UpdateCursor();
    void UpdateDividers();
    void UpdateInput();
    void UpdateUI();
    void UpdateCamera();
    void UpdateMouseRays();
    void UpdateCameraInterpolation(float deltaTime);
    void UpdateDebug();
    void OpenEditor();
    void CloseEditor();
    void ToggleEditorOpenState();
    void SetEditorMode(EditorMode editorMode);
    void SetActiveViewportIndex(int index);
    void SetSelectedObjectType(ObjectType editorObjectType);
    void SetHoveredObjectType(ObjectType editorObjectType);
    void SetSplitX(float value);
    void SetSplitY(float value);
    void SetViewportView(uint32_t viewportIndex, glm::vec3 viewportOrigin, CameraView targetView);
    void SetEditorSelectionMode(EditorSelectionMode editorSelectionMode);
    void SetEditorState(EditorState editorState);
    void SetViewportOrthographicState(uint32_t viewportIndex, bool state);
    void SetViewportOrthoSize(uint32_t viewportIndex, float size);
    void SetEditorViewportSplitMode(EditorViewportSplitMode mode);

    void SetPlantType(TreeType treeType);

    void UpdateGizmoInteract();

    float GetScalingFactor(int targetSizeInPixels);

    // Settings
    void SetBackfaceCulling(bool value);
    bool BackfaceCullingEnabled();
    bool BackfaceCullingDisabled();

    // Object hover
    void UpdateObjectHover();
    void SetHoveredObjectType(ObjectType objectType);
    void SetHoveredObjectId(uint64_t objectId);
    ObjectType GetHoveredObjectType();
    uint64_t GetHoveredObjectId();

    // Height map
    bool HeightMapMouseHitFound();
    const glm::vec3& GetHeightMapMouseHitPosition();

    // Object selection
    void SelectObject(uint64_t objectId);
    void UnselectAnyObject();
    void UpdateObjectSelection();
    void SetSelectedObjectType(ObjectType objectType);
    void SetSelectedObjectId(uint64_t objectId);
    ObjectType GetSelectedObjectType();
    uint64_t GetSelectedObjectId();

    // Gizmo shit
    void UpdateObjectGizmoInteraction();

    // Axis constraint
    void ResetAxisConstraint();
    void SetAxisConstraint(Axis axis);
    Axis GetAxisConstraint();

    // File Menu
    void InitFileMenuImGuiElements();
    void CreateFileMenuImGuiElements();

    // Left panel
    void InitLeftPanel();
    void BeginLeftPanel();
    void EndLeftPanel();
    void UpdateOutliner();

    // New/Open
    void ShowNewMapWindow();
    void ShowOpenMapWindow();

    // House Editor
    void InitHouseEditor();
    void OpenHouseEditor();
    void UpdateHouseEditor();
    void ShowNewHouseWindow();
    void ShowOpenHouseWindow();
    void CloseAllHouseEditorWindows();
    void CreateHouseEditorImGuiElements();

    // Map Height Editor
    void InitMapHeightEditor();
    void OpenMapHeightEditor();
    void UpdateMapHeightEditor();
    void CloseAllMapHeightEditorWindows();
    void CreateMapHeightEditorImGuiElements();

    // Map Object Editor
    void OpenMapObjectEditor();
    void UpdateMapObjectEditor();
    void CreateMapObjectEditorImGuiElements();
    void ShowNewSectorWindow();
    void ShowOpenSectorWindow();
    void CloseAllMapObjectEditorWindows();

    float GetMapHeightNoiseScale();
    float GetMapHeightBrushSize();
    float GetMapHeightBrushStrength();
    float GetMapHeightNoiseStrength();
    float GetMapHeightMinPaintHeight();
    float GetMapHeightMaxPaintHeight();

    void CloseAllEditorWindows();

    void Save();

    void SelectObjectByObjectId(uint64_t objectId);

    // Object placement
    void PlaceHousePlane(WorldPlaneType housePlaneType, const std::string& defaultEditorName = "House Plane");
    void PlaceFireplace(FireplaceType fireplaceType, const std::string& defaultEditorName = "Fireplace");
    void PlaceGenericObject(GenericObjectType objectType, const std::string& defaultEditorName = "Generic Object");
    void PlacePickUp(const std::string& name);
    void PlaceObject(ObjectType objectType); // used for windows, doors, etc

    ObjectType GetPlacementObjectType();
    PlacementObjectSubtype GetPlacementObjectSubtype();
    const std::string& GetPlacementPickUpName();
    void ResetPlacementObjectSubtype();

    void UpdatePictureFramePlacement();
    void UpdatePlayerCampaignSpawnPlacement();
    void UpdatePlayerDeathmatchSpawnPlacement();
    void UpdateTreePlacement();
    void UpdateWallPlacement();
    void UpdateObjectPlacement();
    void ExitObjectPlacement();
    void SetPlacementObjectId(uint64_t objectId);

    // Util
    PhysXRayResult GetMouseRayPhsyXHitPosition();

    uint64_t GetPlacementObjectId();

    // Ray intersections
    glm::vec3 GetMouseRayPlaneIntersectionPoint(glm::vec3 planeOrigin, glm::vec3 planeNormal);

    int GetActiveViewportIndex();
    int GetHoveredViewportIndex();
    bool IsOpen();
    bool IsClosed();
    bool IsViewportOrthographic(uint32_t viewportIndex);
    bool EditorIsIdle();
    bool EditorWasIdleLastFrame();
    float GetVerticalDividerXPos();
    float GetHorizontalDividerYPos();
    glm::vec3 GetMouseRayOriginByViewportIndex(int32_t viewportIndex);
    glm::vec3 GetMouseRayDirectionByViewportIndex(int32_t viewportIndex);
    glm::mat4 GetViewportViewMatrix(int32_t viewportIndex);
    float GetEditorOrthoSize(int32_t viewportIndex);
    Unloved::Viewport* GetActiveViewport();
    ShadingMode GetViewportModeByIndex(uint32_t index);
    CameraView GetCameraViewByIndex(uint32_t index);
    EditorState GetEditorState();
    EditorSelectionMode GetEditorSelectionMode();
    EditorViewportSplitMode GetEditorViewportSplitMode();
    SelectionRectangleState& GetSelectionRectangleState();
    EditorMode& GetEditorMode();
    Axis GetAxisConstraint();

    void SetEditorHouseName(const std::string& houseName);
    void SetEditorMapName(const std::string& mapName);
    const std::string& GetEditorHouseName();
    const std::string& GetEditorMapName();

    // Dividers
    bool IsVerticalDividerHovered();
    bool IsHorizontalDividerHovered();
}
