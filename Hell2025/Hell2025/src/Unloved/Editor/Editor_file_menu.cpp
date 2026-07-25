#include "Editor.h"
#include "Editor_placement.h"

#include "Hell/Backend/BackEnd.h"
#include "Hell/Logging.h"

#include "Unloved/UI/Imgui/Types/Types.h"

namespace Unloved::Editor {

    EditorUI::FileMenu g_fileMenu;

    void InitFileMenuImGuiElements() {
        g_fileMenu.Reset();

        EditorUI::FileMenuNode& file = g_fileMenu.AddMenuNode("File", Shortcut::NONE);
        file.AddChild("New",    Shortcut::F2,       ShowNewMapWindow);
        file.AddChild("Open",   Shortcut::F3,       ShowOpenMapWindow);
        file.AddChild("Save",   Shortcut::CTRL_S,   Editor::Save);
        file.AddChild("Quit",   Shortcut::ESC,      Hell::BackEnd::ForceCloseWindow);

        EditorUI::FileMenuNode& editor = g_fileMenu.AddMenuNode("Editor", Shortcut::NONE);
        editor.AddChild("House",                Shortcut::F4,   OpenHouseEditor);
        editor.AddChild("Map Objects",          Shortcut::F5,   OpenMapObjectEditor);
        editor.AddChild("Map Height",           Shortcut::F6,   OpenMapHeightEditor);

        if (GetEditorMode() != EditorMode::MAP_HEIGHT_EDITOR) {
            EditorUI::FileMenuNode& insert = g_fileMenu.AddMenuNode("Insert", Shortcut::NONE);
            insert.AddChild("Reinsert last",    Shortcut::CTRL_T,   nullptr);

            EditorUI::FileMenuNode& bathroom = insert.AddChild("Bathroom", Shortcut::NONE);
            bathroom.AddChild("Basin",          Shortcut::NONE, BeginPlacement, PlacementTool::GENERIC_BATHROOM_BASIN);
            bathroom.AddChild("Cabinet",        Shortcut::NONE, BeginPlacement, PlacementTool::GENERIC_BATHROOM_CABINET);
            bathroom.AddChild("Toilet",         Shortcut::NONE, BeginPlacement, PlacementTool::GENERIC_TOILET);
            bathroom.AddChild("Towel",          Shortcut::NONE, BeginPlacement, PlacementTool::GENERIC_BATHROOM_TOWEL_RACK);

            EditorUI::FileMenuNode& christmas = insert.AddChild("Christmas", Shortcut::NONE);
            christmas.AddChild("Christmas Lights", Shortcut::NONE, BeginPlacement, PlacementTool::CHRISTMAS_LIGHTS);
            christmas.AddChild("Present Small",    Shortcut::NONE, BeginPlacement, PlacementTool::GENERIC_CHRISTMAS_PRESENT_SMALL);
            christmas.AddChild("Present Large",    Shortcut::NONE, BeginPlacement, PlacementTool::GENERIC_CHRISTMAS_PRESENT_LARGE);
            christmas.AddChild("Tree",             Shortcut::NONE, BeginPlacement, PlacementTool::GENERIC_CHRISTMAS_TREE);

            EditorUI::FileMenuNode& enemies = insert.AddChild("Enemies", Shortcut::NONE);
            enemies.AddChild("Dobermann",       Shortcut::NONE, BeginPlacement, PlacementTool::DOBERMANN);
            enemies.AddChild("Kangaroo",        Shortcut::NONE, BeginPlacement, PlacementTool::KANGAROO);
            enemies.AddChild("Shark",           Shortcut::NONE, BeginPlacement, PlacementTool::SHARK);

            EditorUI::FileMenuNode& exterior = insert.AddChild("Exterior", Shortcut::NONE);
            exterior.AddChild("Fence (Farm)",  Shortcut::NONE, Editor::SetEditorState, EditorState::PLACE_FENCE);
            exterior.AddChild("Jetty",         Shortcut::NONE, BeginPlacement, PlacementTool::JETTY);
            exterior.AddChild("Power Poles",   Shortcut::NONE, BeginPlacement, PlacementTool::POWER_POLES);

            EditorUI::FileMenuNode& interior = insert.AddChild("Interior", Shortcut::NONE);
            interior.AddChild("Chair RE",           Shortcut::NONE, BeginPlacement, PlacementTool::GENERIC_CHAIR_RE);
            interior.AddChild("Chair Spindle Back", Shortcut::NONE, BeginPlacement, PlacementTool::GENERIC_CHAIR_SPINDLE_BACK);
            interior.AddChild("Couch",              Shortcut::NONE, BeginPlacement, PlacementTool::GENERIC_COUCH);
            interior.AddChild("Drawers Small",      Shortcut::NONE, BeginPlacement, PlacementTool::GENERIC_DRAWERS_SMALL);
            interior.AddChild("Drawers Large",      Shortcut::NONE, BeginPlacement, PlacementTool::GENERIC_DRAWERS_LARGE);
            interior.AddChild("Door",               Shortcut::NONE, PlaceObject, ObjectType::DOOR);
			interior.AddChild("Fireplace (Open)",   Shortcut::NONE, PlaceFireplace, FireplaceType::DEFAULT, "Fireplace");
			interior.AddChild("Fireplace (Stove)",  Shortcut::NONE, PlaceFireplace, FireplaceType::WOOD_STOVE, "Wood Stove");

            interior.AddChild("Window",             Shortcut::NONE, SetEditorState, EditorState::PLACE_WINDOW);

            EditorUI::FileMenuNode& lighting = insert.AddChild("Lighting", Shortcut::NONE);
            lighting.AddChild("Christmas Lights", Shortcut::NONE, BeginPlacement, PlacementTool::CHRISTMAS_LIGHTS);
            lighting.AddChild("DDGI Volume",      Shortcut::NONE, SetEditorState, EditorState::PLACE_DDGI_VOLUME);
            lighting.AddChild("Light",            Shortcut::NONE, PlaceObject, ObjectType::LIGHT);

            EditorUI::FileMenuNode& mermaids = insert.AddChild("Mermaids", Shortcut::NONE);
            mermaids.AddChild("Mermaid Shop Owner", Shortcut::NONE, BeginPlacement, PlacementTool::MERMAID);

            EditorUI::FileMenuNode& misc = insert.AddChild("Misc", Shortcut::NONE);
            misc.AddChild("Ladder",             Shortcut::NONE, PlaceObject, ObjectType::LADDER);
            misc.AddChild("Staircase",          Shortcut::NONE, PlaceObject, ObjectType::STAIRCASE);

            EditorUI::FileMenuNode& nature = insert.AddChild("Nature", Shortcut::NONE);
            nature.AddChild("Mermaid Visitor Rock",     Shortcut::NONE, BeginPlacement, PlacementTool::GENERIC_MERMAID_ROCK);
            nature.AddChild("BlackBerries",             Shortcut::NONE, BeginPlacement, PlacementTool::GENERIC_PLANT_BLACKBERRIES);
            nature.AddChild("Tree",                     Shortcut::NONE, BeginPlacement, PlacementTool::GENERIC_PLANT_TREE);

            EditorUI::FileMenuNode& pickups = insert.AddChild("Pick Ups", Shortcut::NONE);

            EditorUI::FileMenuNode& testModels = insert.AddChild("Test Models", Shortcut::NONE);
            testModels.AddChild("Test Model 1", Shortcut::NONE, BeginPlacement, PlacementTool::GENERIC_TEST_MODEL);
            testModels.AddChild("Test Model 2", Shortcut::NONE, BeginPlacement, PlacementTool::GENERIC_TEST_MODEL2);
            testModels.AddChild("Test Model 3", Shortcut::NONE, BeginPlacement, PlacementTool::GENERIC_TEST_MODEL3);
            testModels.AddChild("Test Model 4", Shortcut::NONE, BeginPlacement, PlacementTool::GENERIC_TEST_MODEL4);

            EditorUI::FileMenuNode& weapons = pickups.AddChild("Weapons", Shortcut::NONE);
            weapons.AddChild("AKS74U",          Shortcut::NONE, PlacePickUp, "AKS74U");
            weapons.AddChild("FN-P90",          Shortcut::NONE, PlacePickUp, "P90");
            weapons.AddChild("Glock",           Shortcut::NONE, PlacePickUp, "Glock");
            weapons.AddChild("Golden Glock",    Shortcut::NONE, PlacePickUp, "GoldenGlock");
            weapons.AddChild("Remington 870",   Shortcut::NONE, PlacePickUp, "Remington870");
            weapons.AddChild("SPAS",            Shortcut::NONE, PlacePickUp, "SPAS");
            weapons.AddChild("Tokarev",         Shortcut::NONE, PlacePickUp, "Tokarev");

			weapons.AddChild("Relief Pills",    Shortcut::NONE, PlacePickUp, "Pills");

            EditorUI::FileMenuNode& ammo = pickups.AddChild("Ammo", Shortcut::NONE);
            ammo.AddChild("AKS74U",                     Shortcut::NONE,     nullptr);
            ammo.AddChild("FN-P90",                     Shortcut::NONE,     nullptr);
            ammo.AddChild("Glock",                      Shortcut::NONE,     nullptr);
            ammo.AddChild("Shotgun Shells Buckshot",    Shortcut::NONE,     PlacePickUp, "12GaugeBuckShot");
            ammo.AddChild("Shotgun Shells Slug",        Shortcut::NONE,     PlacePickUp, "12GaugeSlug");
            ammo.AddChild("Tokarev",                    Shortcut::NONE,     nullptr);

            if (GetEditorMode() == EditorMode::MAP_OBJECT_EDITOR) {
                EditorUI::FileMenuNode& locations = g_fileMenu.AddMenuNode("Locations", Shortcut::NONE, nullptr);
                locations.AddChild("House",                     Shortcut::NONE, SetEditorState, EditorState::PLACE_HOUSE);
                locations.AddChild("Player Spawn (Campaign)",   Shortcut::NONE, SetEditorState, EditorState::PLACE_PLAYER_CAMPAIGN_SPAWN);
                locations.AddChild("Player Spawn (Deathmatch)", Shortcut::NONE, SetEditorState, EditorState::PLACE_PLAYER_DEATHMATCH_SPAWN);
            }

            if (GetEditorMode() == EditorMode::HOUSE_EDITOR) {
                EditorUI::FileMenuNode& build = g_fileMenu.AddMenuNode("Build", Shortcut::NONE, nullptr);
                build.AddChild("Ceiling", Shortcut::NONE, PlaceWorldPlane, WorldPlaneType::CEILING, "Ceiling");
                build.AddChild("Floor", Shortcut::NONE,   PlaceWorldPlane, WorldPlaneType::FLOOR, "Floor");
                build.AddChild("Wall", Shortcut::NONE,    SetEditorState, EditorState::PLACE_WALL);
            }
        }

        //EditorUI::FileMenuNode& run = g_fileMenu.AddMenuNode("Run");
        //run.AddChild("New Run", nullptr, "F1");
    }

    void CreateFileMenuImGuiElements() {
        g_fileMenu.CreateImguiElements();
    }
}


