#pragma once
#include "Callbacks.h"
#include "Hell/Backend/BackEnd.h"
#include "Unloved/Editor/Editor.h"
#include "Hell/Logging.h"
#include "Unloved/Maps/MapManager.h"
#include "World/LegacyWorld.h"

#include <iostream>

namespace Unloved::Callbacks {
    void NewHeightMap(const std::string& filename) {
        int defaultChunkWidth = 8;
        int defaultChunkDepth = 16;
        float initialHeight = 30.0f;
        MapManager::NewMap(filename, defaultChunkWidth, defaultChunkDepth, initialHeight);
    }

    void NewHouse(const std::string& filename) {
        Logging::ToDo() << "TODO: NewHouse() callback: " << filename << "\n";
    }

    void NewMap(const std::string& filename) {
        Logging::ToDo() << "TODO: NewMap() callback: " << filename << "\n";
    }

    void OpenHouse(const std::string& filename) {
        Logging::ToDo() << "TODO: OpenHouse() callback: " << filename << "\n";
    }

    void OpenMap(const std::string& filename) {
        MapManager::LoadMap(filename);
    }

    void OpenHouseEditor() {
        Unloved::Editor::OpenHouseEditor();
    }

    void OpenMapHeightEditor() {
        Unloved::Editor::OpenMapHeightEditor();
        Logging::Debug() << "Opened map height editor";
    }

    void OpenMapObjectEditor() {
        Unloved::Editor::OpenMapObjectEditor();
        Logging::Debug() << "Opened map object editor";
    }

    void NewRun() {
        Unloved::LegacyWorld::NewRun();
    }

    void BeginAddingDoor() {
        Unloved::Editor::SetEditorState(EditorState::PLACE_DOOR);
    }

    void BeginAddingHouse() {
        Unloved::Editor::SetEditorState(EditorState::PLACE_HOUSE);
    }

    void BeginAddingPlayerCampaignSpawn() {
        Unloved::Editor::SetEditorState(EditorState::PLACE_PLAYER_CAMPAIGN_SPAWN);
    }

    void BeginAddingPlayerDeathMatchSpawn() {
        Unloved::Editor::SetEditorState(EditorState::PLACE_PLAYER_DEATHMATCH_SPAWN);
    }

    void BeginAddingPictureFrame() {
        Unloved::Editor::SetEditorState(EditorState::PLACE_PICTURE_FRAME);
    }

    void BeginAddingWindow() {
        Unloved::Editor::SetEditorState(EditorState::PLACE_WINDOW);
    }

    void QuitProgram() {
        Hell::BackEnd::ForceCloseWindow();
    }
}