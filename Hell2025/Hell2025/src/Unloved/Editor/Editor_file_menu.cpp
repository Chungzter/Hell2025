#include "Unloved/Editor/Editor.h"

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
            bathroom.AddChild("Basin",          Shortcut::NONE, Editor::PlaceGenericObject, GenericObjectType::BATHROOM_BASIN, "Basin");
            bathroom.AddChild("Cabinet",        Shortcut::NONE, Editor::PlaceGenericObject, GenericObjectType::BATHROOM_CABINET, "Cabinet");
            bathroom.AddChild("Toilet",         Shortcut::NONE, Editor::PlaceGenericObject, GenericObjectType::TOILET, "Toilet");
            bathroom.AddChild("Towel",          Shortcut::NONE, Editor::PlaceGenericObject, GenericObjectType::BATHROOM_TOWEL_RACK, "Towel Rack");

            EditorUI::FileMenuNode& christmas = insert.AddChild("Christmas", Shortcut::NONE);
            christmas.AddChild("Lights",        Shortcut::NONE, Editor::SetEditorState, EditorState::PLACE_CHRISTMAS_LIGHTS);
            christmas.AddChild("Present Small", Shortcut::NONE, Editor::PlaceGenericObject, GenericObjectType::CHRISTMAS_PRESENT_SMALL, "Christmas Present Small");
            christmas.AddChild("Present Large", Shortcut::NONE, Editor::PlaceGenericObject, GenericObjectType::CHRISTMAS_PRESENT_LARGE, "Christmas Present Large");
            christmas.AddChild("Tree",          Shortcut::NONE, Editor::PlaceGenericObject, GenericObjectType::CHRISTMAS_TREE, "Christmas Tree");

            EditorUI::FileMenuNode& exterior = insert.AddChild("Exterior", Shortcut::NONE);
            exterior.AddChild("Power Poles",   Shortcut::NONE, Editor::SetEditorState, EditorState::PLACE_POWER_POLES);
            exterior.AddChild("Fence (Farm)",  Shortcut::NONE, Editor::SetEditorState, EditorState::PLACE_FENCE);

            EditorUI::FileMenuNode& interior = insert.AddChild("Interior", Shortcut::NONE);
            interior.AddChild("Chair RE",           Shortcut::NONE, PlaceGenericObject, GenericObjectType::CHAIR_RE, "Chair RE");
            interior.AddChild("Chair Spindle Back", Shortcut::NONE, PlaceGenericObject, GenericObjectType::CHAIR_SPINDLE_BACK, "Chair Spindle Back");
            interior.AddChild("Couch",              Shortcut::NONE, PlaceGenericObject, GenericObjectType::COUCH, "Couch");
            interior.AddChild("Drawers Small",      Shortcut::NONE, PlaceGenericObject, GenericObjectType::DRAWERS_SMALL, "Drawers Small");
            interior.AddChild("Drawers Large",      Shortcut::NONE, PlaceGenericObject, GenericObjectType::DRAWERS_LARGE, "Drawers Large");
            interior.AddChild("Door",               Shortcut::NONE, PlaceObject, ObjectType::DOOR);
			interior.AddChild("Fireplace (Open)",   Shortcut::NONE, PlaceFireplace, FireplaceType::DEFAULT, "Fireplace");
			interior.AddChild("Fireplace (Stove)",  Shortcut::NONE, PlaceFireplace, FireplaceType::WOOD_STOVE, "Wood Stove");

            interior.AddChild("Window",             Shortcut::NONE, SetEditorState, EditorState::PLACE_WINDOW);

            EditorUI::FileMenuNode& lighting = insert.AddChild("Lighting", Shortcut::NONE);
            lighting.AddChild("Christmas Lights", Shortcut::NONE, SetEditorState, EditorState::PLACE_CHRISTMAS_LIGHTS);
            lighting.AddChild("DDGI Volume",      Shortcut::NONE, SetEditorState, EditorState::PLACE_DDGI_VOLUME);
            lighting.AddChild("Light",            Shortcut::NONE, PlaceObject, ObjectType::LIGHT);

            EditorUI::FileMenuNode& misc = insert.AddChild("Misc", Shortcut::NONE);
            misc.AddChild("Ladder",             Shortcut::NONE, PlaceObject, ObjectType::LADDER);
            misc.AddChild("Staircase",          Shortcut::NONE, PlaceObject, ObjectType::STAIRCASE);

            EditorUI::FileMenuNode& nature = insert.AddChild("Nature", Shortcut::NONE);
            nature.AddChild("Mermaid Visitor Rock",     Shortcut::NONE, PlaceGenericObject, GenericObjectType::MERMAID_ROCK, "Mermaid Visitor Rock");
            nature.AddChild("BlackBerries",             Shortcut::NONE, PlaceGenericObject, GenericObjectType::PLANT_BLACKBERRIES, "Blackberries");
            nature.AddChild("Tree",                     Shortcut::NONE, PlaceGenericObject, GenericObjectType::PLANT_TREE, "Tree");

            EditorUI::FileMenuNode& pickups = insert.AddChild("Pick Ups", Shortcut::NONE);

            EditorUI::FileMenuNode& testModels = insert.AddChild("Test Models", Shortcut::NONE);
            testModels.AddChild("Test Model 1", Shortcut::NONE, PlaceGenericObject, GenericObjectType::TEST_MODEL, "Test Model 1");
            testModels.AddChild("Test Model 2", Shortcut::NONE, PlaceGenericObject, GenericObjectType::TEST_MODEL2, "Test Model 2");
            testModels.AddChild("Test Model 3", Shortcut::NONE, PlaceGenericObject, GenericObjectType::TEST_MODEL3, "Test Model 3");
            testModels.AddChild("Test Model 4", Shortcut::NONE, PlaceGenericObject, GenericObjectType::TEST_MODEL4, "Test Model 4");

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
                build.AddChild("Ceiling", Shortcut::NONE, PlaceHousePlane, WorldPlaneType::CEILING, "Ceiling");
                build.AddChild("Floor", Shortcut::NONE,   PlaceHousePlane, WorldPlaneType::FLOOR, "Floor");
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


