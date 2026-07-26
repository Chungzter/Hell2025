#include "PlacementSequencePoints.h"

#include "Hell/Common/Enum.h"
#include "Hell/Logging.h"

#include "Unloved/Common/CreateInfo.h"
#include "Unloved/Objects/Exterior/PowerPoleSet.h"
#include "Unloved/Objects/Props/Christmas/ChristmasLights.h"
#include "Unloved/Systems/WorldBVH/WorldBVH.h"
#include "Unloved/World/World.h"

namespace Unloved::Editor {
    using EditorSession::PlacementTool;
    using EditorSession::PlacementToolInfo;


    uint64_t CreatePointSequenceObject(PlacementTool tool, const std::vector<SequencePoint>& sequencePoints, const PlacementToolInfo& toolInfo) {
        uint64_t objectId = 0;

        switch (tool) {
        case PlacementTool::CHRISTMAS_LIGHTS: {
            ChristmasLightsCreateInfo createInfo;
            createInfo.sequencePoints = sequencePoints;
            createInfo.defaultEditorName = toolInfo.defaultEditorName;
            objectId = World::AddChristmasLights(createInfo, SpawnOffset());
            break;
        }
        case PlacementTool::POWER_POLES: {
            PowerPoleSetCreateInfo createInfo;
            createInfo.sequencePoints = sequencePoints;
            createInfo.defaultEditorName = toolInfo.defaultEditorName;
            objectId = World::AddPowerPoleSet(createInfo, SpawnOffset());
            break;
        }
        default:
            Logging::Error() << "Editor::CreatePointSequenceObject(..) failed because '" << Hell::Enum::ToString(tool) << "' is not implemented\n";
            break;
        }

        WorldBVH::MarkStaticSceneBvhDirty();
        return objectId;
    }

    void UpdatePointSequenceObject(PlacementTool tool, uint64_t objectId, const std::vector<SequencePoint>& sequencePoints) {
        switch (tool) {
        case PlacementTool::CHRISTMAS_LIGHTS: {
            if (ChristmasLightSet* christmasLights = World::GetChristmasLightsByObjectId(objectId)) {
                christmasLights->UpdateSequencePoints(sequencePoints);
            }
            break;
        }
        case PlacementTool::POWER_POLES: {
            if (PowerPoleSet* powerPoleSet = World::GetPowerPoleSetByObjectId(objectId)) {
                powerPoleSet->UpdateSequencePoints(sequencePoints);
            }
            break;
        }
        default:
            Logging::Error() << "Editor::UpdatePointSequenceObject(..) failed because '" << Hell::Enum::ToString(tool) << "' is not implemented\n";
            break;
        }

        WorldBVH::MarkStaticSceneBvhDirty();
    }
}
