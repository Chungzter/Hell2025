#include "PlacementObjectCreation.h"

#include "Hell/Common/Enum.h"
#include "Hell/Logging.h"

#include "Unloved/World/World.h"

namespace Unloved::Editor {
    using EditorSession::PlacementTool;
    using EditorSession::PlacementToolInfo;


    void PlaceDirectObject(PlacementTool tool, const EditorRayResult& rayResult, const PlacementToolInfo& toolInfo) {

        // Dobermanns
        if (tool == PlacementTool::DOBERMANN) {
            DobermannCreateInfo createInfo;
            createInfo.position = rayResult.position;
            createInfo.defaultEditorName = toolInfo.defaultEditorName;
            World::AddDobermann(createInfo, SpawnOffset());
            return;
        }

        // Jetties
        if (tool == PlacementTool::JETTY) {
            JettyCreateInfo createInfo;
            createInfo.position = rayResult.position;
            createInfo.defaultEditorName = toolInfo.defaultEditorName;
            World::AddJetty(createInfo, SpawnOffset());
            return;
        }

        // Mermaids
        if (tool == PlacementTool::MERMAID) {
            MermaidCreateInfo createInfo;
            createInfo.position = rayResult.position;
            createInfo.rotation = glm::vec3(0.0f);
            createInfo.editorName = "Mermaid Shop Owner";
            createInfo.defaultEditorName = toolInfo.defaultEditorName;
            World::AddMermaid(createInfo, SpawnOffset());
            return;
        }

        // Sharks
        if (tool == PlacementTool::SHARK) {
            SharkCreateInfo createInfo;
            createInfo.position = rayResult.position;
            createInfo.editorName = UNDEFINED_STRING;
            createInfo.defaultEditorName = toolInfo.defaultEditorName;
            World::AddShark(createInfo, SpawnOffset());
            return;
        }

        // Generic Objects
        GenericObjectType genericObjectType = GenericObjectType::UNDEFINED;

        switch (tool) {
            case PlacementTool::GENERIC_BATHROOM_BASIN:              genericObjectType = GenericObjectType::BATHROOM_BASIN;              break;
            case PlacementTool::GENERIC_BATHROOM_CABINET:            genericObjectType = GenericObjectType::BATHROOM_CABINET;            break;
            case PlacementTool::GENERIC_BATHROOM_TOWEL_RACK:         genericObjectType = GenericObjectType::BATHROOM_TOWEL_RACK;         break;
            case PlacementTool::GENERIC_CHAIR_RE:                    genericObjectType = GenericObjectType::CHAIR_RE;                    break;
            case PlacementTool::GENERIC_CHAIR_SPINDLE_BACK:          genericObjectType = GenericObjectType::CHAIR_SPINDLE_BACK;          break;
            case PlacementTool::GENERIC_CHRISTMAS_PRESENT_LARGE:     genericObjectType = GenericObjectType::CHRISTMAS_PRESENT_LARGE;     break;
            case PlacementTool::GENERIC_CHRISTMAS_PRESENT_SMALL:     genericObjectType = GenericObjectType::CHRISTMAS_PRESENT_SMALL;     break;
            case PlacementTool::GENERIC_CHRISTMAS_TREE:              genericObjectType = GenericObjectType::CHRISTMAS_TREE;              break;
            case PlacementTool::GENERIC_COUCH:                       genericObjectType = GenericObjectType::COUCH;                       break;
            case PlacementTool::GENERIC_DRAWERS_LARGE:               genericObjectType = GenericObjectType::DRAWERS_LARGE;               break;
            case PlacementTool::GENERIC_DRAWERS_SMALL:               genericObjectType = GenericObjectType::DRAWERS_SMALL;               break;
            case PlacementTool::GENERIC_MERMAID_ROCK:                genericObjectType = GenericObjectType::MERMAID_ROCK;                break;
            case PlacementTool::GENERIC_PLANT_BLACKBERRIES:          genericObjectType = GenericObjectType::PLANT_BLACKBERRIES;          break;
            case PlacementTool::GENERIC_PLANT_TREE:                  genericObjectType = GenericObjectType::PLANT_TREE;                  break;
            case PlacementTool::GENERIC_TEST_MODEL:                  genericObjectType = GenericObjectType::TEST_MODEL;                  break;
            case PlacementTool::GENERIC_TEST_MODEL2:                 genericObjectType = GenericObjectType::TEST_MODEL2;                 break;
            case PlacementTool::GENERIC_TEST_MODEL3:                 genericObjectType = GenericObjectType::TEST_MODEL3;                 break;
            case PlacementTool::GENERIC_TEST_MODEL4:                 genericObjectType = GenericObjectType::TEST_MODEL4;                 break;
            case PlacementTool::GENERIC_TOILET:                      genericObjectType = GenericObjectType::TOILET;                      break;
            default:                                                                                                                     break;
        }

        if (genericObjectType != GenericObjectType::UNDEFINED) {
            GenericObjectCreateInfo createInfo;
            createInfo.position = rayResult.position;
            createInfo.rotation.y = 0.0f;
            createInfo.type = genericObjectType;
            createInfo.defaultEditorName = toolInfo.defaultEditorName;

            Unloved::World::AddGenericObject(createInfo, SpawnOffset());
        }

        Logging::Error() << "Editor::PlaceDirectObject(..) failed because '" << Hell::Enum::ToString(tool) << "' is not implemented\n";
    }
}
