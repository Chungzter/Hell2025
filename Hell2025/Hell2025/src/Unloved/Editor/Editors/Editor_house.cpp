#include "Hell/Audio.h"
#include "Hell/Backend/BackEnd.h"

#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Render/Renderer.h"
#include "Legacy/World/LegacyWorld.h"

#include "Unloved/Config/Config.h"
#include "Unloved/Editor/Editor.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Systems/House/HouseManager.h"
#include "Unloved/UI/Imgui/Types/Types.h"
#include "Unloved/Viewport/ViewportManager.h"
#include "Unloved/World/World.h"

#include <imgui/imgui.h>

namespace Audio = Hell::Audio;

namespace Unloved::Editor {

    namespace {
        struct ImguiElements {
            EditorUI::CollapsingHeader sectorPropertiesHeader;
            EditorUI::CollapsingHeader rendererSettingsHeader;
            EditorUI::CollapsingHeader objectSettingsHeader;
            EditorUI::CheckBox drawGrass;
            EditorUI::CheckBox drawWater;
            EditorUI::Vec3Input objectPositon;
            EditorUI::Vec3Input objectRotation;
            EditorUI::Vec3Input objectScale;
            EditorUI::NewFileWindow newFileWindow;
            EditorUI::OpenFileWindow openFileWindow;
        } g_imguiElements;
    }

    //void InitHouseEditorFileMenu();
    void InitHouseEditorPropertiesElements();
    //void ReconfigureHouseEditorImGuiElements();

    //struct HouseEditorEditorImguiElements {
    //    EditorUI::FileMenu fileMenu;
    //    EditorUI::LeftPanel leftPanel;
    //    EditorUI::CollapsingHeader housePropertiesHeader;
    //    EditorUI::StringInput houseNameInput;
    //    EditorUI::NewFileWindow newFileWindow;
    //    EditorUI::OpenFileWindow openFileWindow;
    //} g_houseEditorImguiElements;

    //void InitHouseEditorFileMenu();
    //void InitHouseEditorPropertiesElements();
    //void ReconfigureHMapEditorImGuiElements();

    // Wall placement
    void BeginWall();
    //void CancelWallPlacement();
    void UpdateWallPlacement();

    void OpenHouseEditor() {
        // If it's closed, open it
        if (IsClosed()) {
            OpenEditor();
        }
        // If it's already open, do nothing
        else if (GetEditorMode() == EditorMode::HOUSE_EDITOR) {
            return;
        }
        SetEditorMode(EditorMode::HOUSE_EDITOR);

        // World state
        LegacyWorld::ResetWorld();
        if (HouseData* houseData = HouseManager::GetHouseDataByName(Editor::GetEditorHouseName())) {
            World::LoadHouse(*houseData, SpawnOffset());
        }
        // Init UI
        InitFileMenuImGuiElements();
        InitLeftPanel();
        //ReconfigureHouseEditorImGuiElements();

        // Move player somewhere reasonable
        if (Unloved::Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(0)) {
            if (player->GetFootPosition().y > 10) {
                player->SetFootPosition(glm::vec3(2.25f, 0.0, 1.68f));
                player->GetCamera().SetEulerRotation(glm::vec3(-0.2f, 0.0, 0.0f));
            }
        }

        Audio::PlayAudio(AUDIO_SELECT, 1.0f);
    }

    void InitHouseEditor() {
        InitHouseEditorPropertiesElements();
    }

    void CreateHouseEditorImGuiElements() {
        BeginLeftPanel();

        EndLeftPanel();
    }


    void InitHouseEditorPropertiesElements() {

    }

    void UpdateHouseEditor() {
        // Restrict renderer states
        RendererSettings& rendererSettings = Renderer::GetCurrentRendererSettings();
        while (//rendererSettings.rendererOverrideState == RendererOverrideState::NONE ||
               rendererSettings.rendererOverrideState == RendererOverrideState::RMA ||
               rendererSettings.rendererOverrideState == RendererOverrideState::NORMALS ||
               rendererSettings.rendererOverrideState == RendererOverrideState::METALIC ||
               rendererSettings.rendererOverrideState == RendererOverrideState::ROUGHNESS ||
               rendererSettings.rendererOverrideState == RendererOverrideState::TILE_HEATMAP_LIGHTS ||
               rendererSettings.rendererOverrideState == RendererOverrideState::STATE_COUNT ||
               rendererSettings.rendererOverrideState == RendererOverrideState::AO) {
            Renderer::NextRendererOverrideState();
        }

        // Test mouse hover on point
        //glm::vec3 testPoint = glm::vec3(0, 1, 0);
        //glm::vec3 color = WHITE;
        //Viewport* viewport = ViewportManager::GetViewportByIndex(0);
        //SpaceCoords gbufferSpaceCoords = viewport->GetGBufferSpaceCoords();
        //int mouseX = gbufferSpaceCoords.localMouseX;
        //int mouseY = gbufferSpaceCoords.localMouseY;
        //int screenWidth = gbufferSpaceCoords.width;
        //int screenHeight = gbufferSpaceCoords.height;
        //glm::mat4 projectionView = RenderDataManager::GetViewportData()[0].projectionView;
        //glm::ivec2 testPosScreenSpace = Hell::Projection::WorldToScreen(testPoint, projectionView, screenWidth, screenHeight, true);
        //glm::ivec2 mousePos = glm::ivec2(mouseX, mouseY);
        //int threshold = 20;
        //if (Hell::Math::WithinDistance(mousePos, testPosScreenSpace, threshold)) {
        //    color = OUTLINE_COLOR;
        //}
        //DebugDraw::DrawPoint(testPoint, color);




        // Render selected wall/plane lines and vertices
       //if (GetSelectedObjectType() == ObjectType::WALL) {
       //   // MOVED TO EDITOR_OBJECTS.cpp
       //   // MOVED TO EDITOR_OBJECTS.cpp
       //   // MOVED TO EDITOR_OBJECTS.cpp
       //   // MOVED TO EDITOR_OBJECTS.cpp
       //   // MOVED TO EDITOR_OBJECTS.cpp
       //}
       //if (GetSelectedObjectType() == ObjectType::PLANE) {
       //    Plane* plane = LegacyWorld::GetPlaneByObjectId(GetSelectedObjectId());
       //    if (plane) {
       //        plane->DrawEdges(OUTLINE_COLOR);
       //        plane->DrawVertices(OUTLINE_COLOR);
       //    }
       //}
    }

    void ShowNewHouseWindow() {
        CloseAllEditorWindows();
    }

    void ShowOpenHouseWindow() {
        CloseAllEditorWindows();
    }

    void CloseAllHouseEditorWindows() {

    }
}
