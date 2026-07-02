#include "Shark.h"

namespace Unloved {

    std::string Shark::GetDebugInfoAsString() {
        std::string result = "\nShark\n";
        //result += "- Movement State: " + Util::SharkMovementStateToString(m_movementState) + "\n";
        //result += "- Hunting State: " + Util::SharkHuntingStateToString(m_huntingState) + "\n";
        //result += "- Hunted player ID: " + std::to_string(m_huntedPlayerId) + "\n";
        //result += "- Forward: " + Hell::String::FormatVec3(m_forward) + "\n";
        //result += "- Left: " + Hell::String::FormatVec3(m_left) + "\n";
        //result += "- Right: " + Hell::String::FormatVec3(m_right) + "\n";
        //result += "- Target Position: " + Hell::String::FormatVec3(m_targetPosition) + "\n";
        //result += "- Last Known Target Position: " + Hell::String::FormatVec3(m_lastKnownTargetPosition) + "\n";
        //result += "- Path size: " + std::to_string(m_path.size()) + "\n";
        //result += "- Next path point index: " + std::to_string(m_nextPathPointIndex) + "\n";
        return result;
    }

    SharkHuntingState m_huntingState = SharkHuntingState::UNDEFINED;
    SharkMovementState m_movementState = SharkMovementState::FOLLOWING_PATH;

}
