#include "Editor.h"

#include "Hell/Audio.h"
#include "Hell/Backend/BackEnd.h"
#include "Hell/Input.h"
#include "Hell/Logging.h"
#include "Hell/Profiling/CPUProfiler.h"

#include "Unloved/Camera/Camera.h"
#include "Unloved/Config/Config.h"
#include "Unloved/Debug/Debug.h"
#include "Unloved/Debug/DebugDraw.h"
#include "Unloved/Editor/Editor_placement.h"
#include "Unloved/Editor/PlacementTools.h"
#include "Unloved/Editor/Gizmo.h"
#include "Unloved/Systems/Map/MapManager.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Systems/House/HouseManager.h"
#include "Unloved/UI/Imgui/ImguiBackEnd.h"
#include "Unloved/Viewport/ViewportManager.h"
#include "Unloved/World/World.h"

#include "Unloved/Common/Constants.h"
#include "Unloved/Render/Renderer.h"
#include "Legacy/World/LegacyWorld.h"

namespace Audio = Hell::Audio;
namespace Input = Hell::Input;
namespace InputMulti = Hell::InputMulti;

namespace Unloved::Editor {

    EditorMode g_editorMode = EditorMode::UNDEFINED;
    int g_activeViewportIndex = 3;
    bool g_isOpen = false;
    bool g_isOrthographic[4];
    bool g_editorStateWasIdleLastFrame = true;
    float g_OrthographicSizes[4];
    float g_verticalDividerXPos = 0.2f;
    float g_horizontalDividerYPos = 0.2f;

    ObjectType g_placementObjectType = ObjectType::NO_TYPE;
    PlacementObjectSubtype g_placementObjectSubtype;

    ObjectType g_hoveredObjectType = ObjectType::NO_TYPE;
    ObjectType g_selectedObjectType = ObjectType::NO_TYPE; 
    //GenericObjectType g_placementGenericObjectType = GenericObjectType::UNDEFINED;

    uint64_t g_hoveredObjectId = 0;
    uint64_t g_selectedObjectId = 0;

    ShadingMode g_shadingModes[4];
    EditorViewportSplitMode g_editorViewportSplitMode = EditorViewportSplitMode::SINGLE;
    Axis g_axisConstraint = Axis::NONE;

    std::string g_editorHouseName = UNDEFINED_STRING;
    std::string g_editorMapName = UNDEFINED_STRING;

    float g_orthoCameraDistances[4];
    EditorState g_editorState;
    EditorSelectionMode g_editorSelectionMode = EditorSelectionMode::OBJECT;
    SelectionRectangleState g_viewportSelectionRectangleState;

    void Init() {
        Editor::InitPlacementTools();
        ResetViewports();
        ResetCameras();

        //InitMapObjectEditor();
        InitMapHeightEditor();
        InitHouseEditor();
    }

    void OpenEditor() {
        Audio::PlayAudio("UI_Select.wav", 1.0f);
        Editor::SetEditorState(EditorState::IDLE);
        Editor::SetEditorSelectionMode(EditorSelectionMode::OBJECT);
        Editor::ResetAxisConstraint();
        Input::ShowCursor();
        Input::CenterMouseCursor();

        // Load default map
        if (GetEditorMapName() != "Shit") {
            SetEditorMapName("Shit");
        }

        // Load default house
        if (GetEditorHouseName() != "TestHouse") {
            SetEditorHouseName("TestHouse");
        }

        g_isOpen = true;
    }

    void CloseEditor() {
        Audio::PlayAudio("UI_Select.wav", 1.0f);
        Input::DisableCursor();

        UnselectAnyObject();
        SetHoveredObjectType(ObjectType::NO_TYPE);
        SetHoveredObjectId(0);

        if (GetEditorMode() == EditorMode::HOUSE_EDITOR) {
            // Update the house file with everything in the world
            HouseManager::UpdateCreateInfoCollectionFromWorld(Editor::GetEditorHouseName());

            // Reload the single editor house
            LegacyWorld::ResetWorld();
            if (HouseData* houseData = HouseManager::GetHouseDataByName(Editor::GetEditorHouseName())) {
                World::LoadHouse(*houseData, SpawnOffset());
            }
        }
        if (GetEditorMode() == EditorMode::MAP_HEIGHT_EDITOR) {
            // Reload everything from the editor map file
            Renderer::RecalculateAllHeightMapData(false);
            LegacyWorld::ClearAllObjects();
            if (MapData* mapData = MapManager::GetMapDataByName(Editor::GetEditorMapName())) {
                World::LoadMap(*mapData, SpawnOffset());
            }
        }
        if (GetEditorMode() == EditorMode::MAP_OBJECT_EDITOR) {
            // Update the map file with everything in the world
            MapManager::UpdateCreateInfoCollectionFromWorld(Editor::GetEditorMapName());

            // Reload everything from the editor map file, except the height map
            LegacyWorld::ClearAllObjects();
            if (MapData* mapData = MapManager::GetMapDataByName(Editor::GetEditorMapName())) {
                World::LoadMap(*mapData, SpawnOffset());
            }
        }

        g_isOpen = false;
    }

    void Save() {
        if (Editor::GetEditorMode() == EditorMode::HOUSE_EDITOR) {
            HouseManager::SaveHouse(Editor::GetEditorHouseName());
        }
        else if (Editor::GetEditorMode() == EditorMode::MAP_HEIGHT_EDITOR || Editor::GetEditorMode() == EditorMode::MAP_OBJECT_EDITOR) {
            MapManager::SaveMap(Editor::GetEditorMapName());
        }
    }

    void ResetViewports() {

        const Resolutions& resolutions = Config::GetResolutions();

        // Center the viewport splits
        float editorWidth = resolutions.ui.x - EDITOR_LEFT_PANEL_WIDTH;
        float editorHeight = resolutions.ui.y - ImGuiBackEnd::GetFileMenuHeight();
        float editorWidthNormalized = editorWidth / resolutions.ui.x;
        float editorHeightNormalized = editorHeight / resolutions.ui.y;
        float panelRightEdgeNormalized = EDITOR_LEFT_PANEL_WIDTH / resolutions.ui.x;
        float fileMenuHeightNormalized = ImGuiBackEnd::GetFileMenuHeight() / resolutions.ui.y;
        g_verticalDividerXPos = panelRightEdgeNormalized + (editorWidthNormalized * 0.5f);
        g_horizontalDividerYPos = fileMenuHeightNormalized + (editorHeightNormalized * 0.5f);

        float ORTHO_CAMERA_DISTANCE_FROM_ORIGIN = 250.0f;
        g_orthoCameraDistances[0] = ORTHO_CAMERA_DISTANCE_FROM_ORIGIN;
        g_orthoCameraDistances[1] = ORTHO_CAMERA_DISTANCE_FROM_ORIGIN;
        g_orthoCameraDistances[2] = ORTHO_CAMERA_DISTANCE_FROM_ORIGIN;
        g_orthoCameraDistances[3] = ORTHO_CAMERA_DISTANCE_FROM_ORIGIN;

        // Top left
        SetViewportView(0, Gizmo::GetPosition(), CameraView::RIGHT);
        //ViewportManager::GetViewportByIndex(0)->SetOrthoSize(1.14594f);
        Unloved::ViewportManager::GetViewportByIndex(0)->SetOrthoSize(76.1911f);

        // Top right
        SetViewportView(1, Gizmo::GetPosition(), CameraView::LEFT);
        Unloved::ViewportManager::GetViewportByIndex(1)->SetOrthoSize(1.1958f);

        // Bottom left
        SetViewportView(2, Gizmo::GetPosition(), CameraView::TOP);
        Unloved::ViewportManager::GetViewportByIndex(2)->SetOrthoSize(1.19627f);

        // Bottom right
        SetViewportView(3, Gizmo::GetPosition(), CameraView::FRONT);
        Unloved::ViewportManager::GetViewportByIndex(3)->SetOrthoSize(1.1958f);
    }

    void Update(float deltaTime) {
        ProfilerCPUZoneFunction();

        // Close editor
        if (Input::KeyPressed(HELL_KEY_TAB)) {
            InputMulti::ClearKeyStates();
            Audio::PlayAudio(AUDIO_SELECT, 1.0f);
            CloseEditor();
        }

        if (!IsOpen()) {
            return;
        }

       // if (Input::KeyPressed(HELL_KEY_T)) {
       //     PlaceGenericObject(GenericObjectType::PLANT_TREE);
       // }
       // if (Input::KeyPressed(HELL_KEY_B)) {
       //     PlaceGenericObject(GenericObjectType::PLANT_BLACKBERRIES);
       // }
       //

        g_editorStateWasIdleLastFrame = (g_editorState == EditorState::IDLE);

        UpdateCamera();    // you swapped these two, maybe it was better before?
        UpdateMouseRays(); // you swapped these two, maybe it was better before?
        Gizmo::Update();
        UpdateObjectHover();
        UpdateObjectSelection();
        UpdateObjectGizmoInteraction();
        UpdateDividers();
        UpdateInput();
        UpdateUI();
        UpdateCursor();
        UpdateDebug();
        UpdateCameraInterpolation(deltaTime);
        Gizmo::UpdateRenderItems();


        if (GetEditorState() == EditorState::PLACEMENT) {
            UpdatePlacement();
        }
        else if (GetEditorMode() == EditorMode::HOUSE_EDITOR || GetEditorMode() == EditorMode::MAP_OBJECT_EDITOR) {
            UpdateObjectPlacement();
        }
        if (GetEditorState() == EditorState::IDLE) {
            ExitObjectPlacement();
        }

        UpdateOutliner(); // make this nicer

        switch (GetEditorMode()) {
            case EditorMode::HOUSE_EDITOR:      UpdateHouseEditor();       break;
            case EditorMode::MAP_HEIGHT_EDITOR: UpdateMapHeightEditor();   break;
            case EditorMode::MAP_OBJECT_EDITOR: UpdateMapObjectEditor();   break;
            default: break;
        }

        if (Input::KeyPressed(HELL_KEY_Q)) {
            if (GetEditorViewportSplitMode() == EditorViewportSplitMode::SINGLE) {
                SetEditorViewportSplitMode(EditorViewportSplitMode::FOUR_WAY_SPLIT);
                std::cout << "four way\n";
            }
            else {
                SetEditorViewportSplitMode(EditorViewportSplitMode::SINGLE);
                std::cout << "single\n";
            }
        }

        // Draw world map perimeter and spawn points
        if (GetEditorMode() == EditorMode::MAP_HEIGHT_EDITOR || GetEditorMode() == EditorMode::MAP_OBJECT_EDITOR) {
            float h = 30.0f;
            float w = LegacyWorld::GetWorldSpaceWidth();
            float d = LegacyWorld::GetWorldSpaceDepth();

            // Draw perimeter
            glm::vec3 p0 = glm::vec3(0.0f, h, 0.0f);
            glm::vec3 p1 = glm::vec3(w, h, 0.0f);
            glm::vec3 p2 = glm::vec3(0.0f, h, d);
            glm::vec3 p3 = glm::vec3(w, h, d);
            DebugDraw::DrawLine(p0, p1, GRID_COLOR, true);
            DebugDraw::DrawLine(p0, p2, GRID_COLOR, true);
            DebugDraw::DrawLine(p2, p3, GRID_COLOR, true);
            DebugDraw::DrawLine(p1, p3, GRID_COLOR, true);

            // Draw spawn points as little dots
            MapData* mapData = MapManager::GetMapDataByName(GetEditorMapName());
            if (mapData) {
                for (const SpawnPointCreateInfo& spawnPoint : mapData->GetCreateInfoCollection().spawnPointsCampaign) {
                    DebugDraw::DrawPoint(spawnPoint.position, GREEN);
                }
                for (const SpawnPointCreateInfo& spawnPoint : mapData->GetCreateInfoCollection().spawnPointsDeathMatch) {
                    DebugDraw::DrawPoint(spawnPoint.position, YELLOW);
                }
            }

            // Draw cubes around spawn points
            for (SpawnPoint& spawnPoint : World::GetSpawnPointsCampaign()) {
                spawnPoint.DrawDebugCube();
            }
        }

        // Draw house editor grid
        const Resolutions& resolutions = Config::GetResolutions();
        float pixelSizeX = 2.0f / resolutions.gBuffer.x;
        float pixelSizeY = 2.0f / resolutions.gBuffer.y;

        float gridWorldSpaceSize = 20.0f;
        float gridSpacing = 0.5f;
        float yHeight = -0.01f;

        for (float x = -gridWorldSpaceSize; x <= gridWorldSpaceSize; x += gridSpacing) {
            for (float z = -gridWorldSpaceSize; z <= gridWorldSpaceSize; z += gridSpacing) {
                glm::vec3 p1 = glm::vec3(x, yHeight, -gridWorldSpaceSize);
                glm::vec3 p2 = glm::vec3(x, yHeight, gridWorldSpaceSize);
                glm::vec3 p3 = glm::vec3(-gridWorldSpaceSize, yHeight, z);
                glm::vec3 p4 = glm::vec3(gridWorldSpaceSize, yHeight, z);
                DebugDraw::DrawLine(p1, p2, GRID_COLOR, true);
                DebugDraw::DrawLine(p3, p4, GRID_COLOR, true);
            }
        }

        for (int x = 0; x <= 1; x++) {
            for (int y = 0; y <= 1; y++) {
                glm::vec3 n = glm::vec3(gridWorldSpaceSize, yHeight, 0.0f);
                glm::vec3 s = glm::vec3(-gridWorldSpaceSize, yHeight, 0.0f);
                glm::vec3 e = glm::vec3(0.0f, yHeight, gridWorldSpaceSize);
                glm::vec3 w = glm::vec3(0.0f, yHeight, -gridWorldSpaceSize);

                DebugDraw::DrawLine(n, s, WHITE, true);
                DebugDraw::DrawLine(e, w, WHITE, true);
            }
        }
    }

    void ToggleEditorOpenState() {
        g_isOpen = !g_isOpen;
        if (g_isOpen) {
            OpenEditor();
        }
        else {
            CloseEditor();
        }
    }

    void PlaceObject(ObjectType objectType) {
        SetEditorState(EditorState::PLACE_OBJECT);
        g_placementObjectSubtype.Reset();
        g_placementObjectType = objectType;
    }

    void PlaceFireplace(FireplaceType fireplaceType, const std::string& defaultEditorName) {
        SetEditorState(EditorState::PLACE_OBJECT);
        g_placementObjectSubtype.Reset();
        g_placementObjectSubtype.fireplace = fireplaceType;
        g_placementObjectSubtype.defaultEditorName = defaultEditorName;
        g_placementObjectType = ObjectType::FIREPLACE;
    }

    void PlacePickUp(const std::string& name) {
        SetEditorState(EditorState::PLACE_OBJECT);
        g_placementObjectSubtype.Reset();
        g_placementObjectSubtype.pickUpName = name;
        g_placementObjectSubtype.defaultEditorName = name;
        g_placementObjectType = ObjectType::PICK_UP;
    }

    void PlaceWorldPlane(WorldPlaneType worldPlaneType, const std::string& defaultEditorName) {
        SetEditorState(EditorState::PLACE_OBJECT);
        g_placementObjectSubtype.Reset();
        g_placementObjectSubtype.worldPlane = worldPlaneType;
        g_placementObjectSubtype.defaultEditorName = defaultEditorName;
        g_placementObjectType = ObjectType::WORLD_PLANE;
    }

    ObjectType GetPlacementObjectType() {
        return g_placementObjectType;
    }

    PlacementObjectSubtype GetPlacementObjectSubtype() {
        return g_placementObjectSubtype;
    }

    const std::string& GetPlacementPickUpName() {
        return g_placementObjectSubtype.pickUpName;
    }

    void ResetPlacementObjectSubtype() {
        g_placementObjectSubtype.Reset();
    }

    void SetEditorMode(EditorMode editorMode) {
        g_editorMode = editorMode;
    }

    void SetActiveViewportIndex(int index) {
        g_activeViewportIndex = index;
    }

    int GetActiveViewportIndex() {
        return g_activeViewportIndex;
    }

    int GetHoveredViewportIndex() {
        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (viewport->IsVisible() && viewport->IsHovered()) {
                return i;
            }
        }
        return 0;
    }

    bool IsOpen() {
        return g_isOpen;
    }

    bool IsClosed() {
        return !g_isOpen;
    }

    bool EditorIsIdle() {
        return g_editorState == EditorState::IDLE;
    }

    bool EditorWasIdleLastFrame() {
        return g_editorStateWasIdleLastFrame;
    }

    //void SetSelectedObjectIndex(int index) {
    //    g_selectedObjectIndex = index;
    //}
    //
    //void SetHoveredObjectIndex(int index) {
    //    g_hoveredObjectIndex = index;
    //}


    void SetSplitX(float value) {
        g_verticalDividerXPos = value;
        //ViewportManager::UpdateViewports();
    }

    void SetSplitY(float value) {
        g_horizontalDividerYPos = value;
        //ViewportManager::UpdateViewports();
    }

    //void SetCurrentMapName(const std::string& filename) {
    //    g_currentMapName = filename;
    //}

    void SetHoveredObjectType(ObjectType objectType) {
        g_hoveredObjectType = objectType;
    }

    void SetHoveredObjectId(uint64_t objectId) {
        g_hoveredObjectId = objectId;

        //if (objectId != 0) std::cout << "hovered: " << objectId << "\n";
    }

    void SetSelectedObjectType(ObjectType objectType) {
        g_selectedObjectType = objectType;
    }


    // Maybe remove me?
    void SetSelectedObjectId(uint64_t objectId) {
        g_selectedObjectId = objectId;

        //if (objectId != 0) std::cout << "selected: " << objectId << "\n";
    }

    void SelectObject(uint64_t objectId) {
        Audio::PlayAudio(AUDIO_SELECT, 1.0f);

        Gizmo::SetPosition(World::GetPositionById(objectId));
        Gizmo::SetRotation(World::GetRotationById(objectId));

        ExitObjectPlacement();
        g_selectedObjectId = objectId;
    }

    ObjectType GetSelectedObjectType() {
        return g_selectedObjectType;
    }

    ObjectType GetHoveredObjectType() {
        return g_hoveredObjectType;
    }

    uint64_t GetSelectedObjectId() {
        return g_selectedObjectId;
    }

    uint64_t GetHoveredObjectId() {
        return g_hoveredObjectId;
    }

    bool IsViewportOrthographic(uint32_t viewportIndex) {
        if (viewportIndex >= 0 && viewportIndex < 4) {
            return g_isOrthographic[viewportIndex];
        }
        else {
            std::cout << "Game::GetCameraByIndex(int index) failed. " << viewportIndex << " out of range of editor viewport count 4\n";
            return false;
        }
    }

    //Camera* GetCameraByIndex(uint32_t index) {
    //    if (index >= 0 && index < 4) {
    //        return &g_cameras[index];
    //    }
    //    else {
    //        std::cout << "Game::GetCameraByIndex(int index) failed. " << index << " out of range of editor viewport count 4\n";
    //        return nullptr;
    //    }
    //}

    Unloved::Viewport* GetActiveViewport() {
        if (g_activeViewportIndex >= 0 && g_activeViewportIndex < 4) {
            return Unloved::ViewportManager::GetViewportByIndex(g_activeViewportIndex);
        }
        else {
            std::cout << "Editor::GetActiveViewport(int index) failed. " << g_activeViewportIndex << " out of range of editor viewport count 4\n";
            return nullptr;
        }
    }

    ShadingMode GetViewportModeByIndex(uint32_t viewportIndex) {
        if (viewportIndex >= 0 && viewportIndex < 4) {
            return g_shadingModes[viewportIndex];
        }
        else {
            std::cout << "Editor::GetViewportModeByIndex(uint32_t viewportIndex) failed. " << viewportIndex << " out of range of editor viewport count 4\n";
            return ShadingMode::SHADED;
        }
    }

    //void SetCameraView(uint32_t cameraViewIndex, CameraView cameraView) {
    //    if (cameraViewIndex >= 0 && cameraViewIndex < 4) {
    //        g_cameraViews[cameraViewIndex] = cameraView;
    //    }
    //    else {
    //        std::cout << "Editor::SetCameraViewByIndex(uint32_t cameraViewIndex, CameraView cameraView) failed. " << cameraViewIndex << " out of range of editor viewport count 4\n";
    //    }
    //}

    void SetEditorViewportSplitMode(EditorViewportSplitMode mode) {
        g_editorViewportSplitMode = mode;
    }

    void SetViewportOrthoSize(uint32_t viewportIndex, float size) {
        if (viewportIndex >= 0 && viewportIndex < 4) {
            g_OrthographicSizes[viewportIndex] = size;
        }
        else {
            std::cout << "Editor::SetViewportOrthoSize(uint32_t viewportIndex, float size) failed. " << viewportIndex << " out of range of editor viewport count 4\n";
        }
    }

    EditorState GetEditorState() {
        return g_editorState;
    }

    EditorSelectionMode GetEditorSelectionMode() {
        return g_editorSelectionMode;
    }

    SelectionRectangleState& GetSelectionRectangleState() {
        return g_viewportSelectionRectangleState;
    }

    EditorMode& GetEditorMode() {
        return g_editorMode;
    }

    float GetVerticalDividerXPos() {
        return g_verticalDividerXPos;
    }

    float GetHorizontalDividerYPos() {
        return g_horizontalDividerYPos;
    }

    EditorViewportSplitMode GetEditorViewportSplitMode() {
        return g_editorViewportSplitMode;
    }

    void SetEditorState(EditorState editorState) {
        g_editorState = editorState;
    }
    
    void SetEditorSelectionMode(EditorSelectionMode editorSelectionMode) {
        g_editorSelectionMode = editorSelectionMode;
    }

    void SetViewportOrthographicState(uint32_t index, bool state) {
        if (index >= 0 && index < 4) {
            g_isOrthographic[index] = state;
        }
        else {
            std::cout << "Editor::SetViewportOrthographicStateByIndex(uint32_t index, bool state) failed. " << index << " out of range of editor viewport count 4\n";
        }
    }

    void SetAxisConstraint(Axis axis) {
        g_axisConstraint = axis;
    }

    void ResetAxisConstraint() {
        g_axisConstraint = Axis::NONE;
    }

    Axis GetAxisConstraint() {
        return g_axisConstraint;
    }

    void CloseAllEditorWindows() {
        CloseAllHouseEditorWindows();
        CloseAllMapHeightEditorWindows();
        CloseAllMapObjectEditorWindows();
    }

    void SetEditorHouseName(const std::string& houseName) {
        g_editorHouseName = houseName;
    }

    void SetEditorMapName(const std::string& mapName) {
        g_editorMapName = mapName;
    }

    const std::string& GetEditorHouseName() {
        return g_editorHouseName;
    }

    const std::string& GetEditorMapName() {
        return g_editorMapName;
    }
}
