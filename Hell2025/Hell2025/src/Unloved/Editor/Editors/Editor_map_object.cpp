#include "Hell/Audio.h"

#include "Legacy/File/JSON.h"
#include "Unloved/Systems/Map/MapManager.h"
#include "Unloved/Render/Renderer.h"
#include "Legacy/World/LegacyWorld.h"

#include "Unloved/Config/Config.h"
#include "Unloved/Editor/Editor.h"
#include "Unloved/Editor/Gizmo.h"
#include "Unloved/UI/Imgui/ImguiBackEnd.h"
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

    void OpenMapObjectEditor() {
        // If it's closed, open it
        if (IsClosed()) {
            OpenEditor();
        }
        // If it's already open, do nothing
        else if (GetEditorMode() == EditorMode::MAP_OBJECT_EDITOR) {
            return;
        }
        SetEditorMode(EditorMode::MAP_OBJECT_EDITOR);

        // World state
        MapCreateInfo mapCreateInfo;
        mapCreateInfo.mapName = Editor::GetEditorMapName();
        mapCreateInfo.spawnOffsetChunkX = 0;
        mapCreateInfo.spawnOffsetChunkZ = 0;
        LegacyWorld::ClearAllObjects();
        LegacyWorld::LoadMapsHeightMapData({ mapCreateInfo });
        if (MapData* mapData = MapManager::GetMapDataByName(Editor::GetEditorMapName())) {
            World::LoadMapObjects(*mapData, SpawnOffset());
        }
        // Init UI
        InitFileMenuImGuiElements();
        InitLeftPanel();

        Audio::PlayAudio(AUDIO_SELECT, 1.0f);
    }

    void CreateMapObjectEditorImGuiElements() {
        BeginLeftPanel();

        EndLeftPanel();
        //SectorEditorImguiElements& elements = g_sectorEditorImguiElements;
        //
        //elements.fileMenu.CreateImguiElements();
        //elements.leftPanel.BeginImGuiElement();
        //
        //const std::string& sectorName = GetSectorName();
        //SectorCreateInfo* sectorCreateInfo = SectorManager::GetSectorCreateInfoByName(sectorName);
        //
        //if (!sectorCreateInfo) return;
        //
        //// Renderer settings
        //if (elements.rendererSettingsHeader.CreateImGuiElement()) {
        //    if (elements.drawGrass.CreateImGuiElements()) {
        //        RendererSettings& renderSettings = Renderer::GetCurrentRendererSettings();
        //        renderSettings.drawGrass = elements.drawGrass.GetState();
        //     
        //    }
        //    if (elements.drawWater.CreateImGuiElements()) {
        //        std::cout << elements.drawWater.GetState();
        //    }
        //    ImGui::Dummy(ImVec2(0.0f, 10.0f));
        //}
        //
        //// Sector properties
        //if (elements.sectorPropertiesHeader.CreateImGuiElement()) {
        //    elements.sectorNameInput.CreateImGuiElement();
        //    ImGui::Dummy(ImVec2(0.0f, 20.0f));
        //}
        //
        //// Object settings
        //if (elements.objectSettingsHeader.CreateImGuiElement()) {         
        //    if (elements.objectPositon.CreateImGuiElements()) {
        //        std::cout << Hell::String::FormatVec3(elements.objectPositon.GetValue()) << "\n";;
        //    }
        //    if (elements.objectRotation.CreateImGuiElements()) {
        //        std::cout << Hell::String::FormatVec3(elements.objectRotation.GetValue()) << "\n";;
        //    }
        //    if (elements.objectScale.CreateImGuiElements()) {
        //        std::cout << Hell::String::FormatVec3(elements.objectScale.GetValue()) << "\n";;
        //    }
        //    ImGui::Dummy(ImVec2(0.0f, 20.0f));
        //}
        //
        //// Outliner settings
        //if (elements.outlinerHeader.CreateImGuiElement()) {
        //    elements.outliner.CreateImGuiElements();
        //    ImGui::Dummy(ImVec2(0.0f, 20.0f));
        //}
        //
        //elements.leftPanel.EndImGuiElement();
        //
        //// Windows
        //if (elements.newFileWindow.IsVisible()) {
        //    elements.newFileWindow.CreateImGuiElements();
        //}
        //if (elements.openFileWindow.IsVisible()) {
        //    elements.openFileWindow.CreateImGuiElements();
        //}
    }

    void ShowNewSectorWindow() {
        CloseAllEditorWindows();
        ImguiElements& elements = g_imguiElements;
        elements.newFileWindow.Show();
    }

    void ShowOpenSectorWindow() {
        CloseAllEditorWindows();
        ImguiElements& elements = g_imguiElements;
        elements.openFileWindow.Show();
    }

    void CloseAllMapObjectEditorWindows() {
        ImguiElements& elements = g_imguiElements;
        elements.newFileWindow.Close();
        elements.openFileWindow.Close();
    }

    void UpdateMapObjectEditor() {
        // Nothing as of yet
    }
}



