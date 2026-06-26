#include "World.h"

#include "Hell/Time.h"

#include "Legacy/Editor/Editor.h"
#include "Legacy/Imgui/ImguiBackEnd.h"

#include "Unloved/Session/Session.h"

namespace Unloved::World {
    void UpdatePlayers() {
        const float deltaTime = Hell::Time::DeltaTime();
        const bool disableControl = Editor::IsOpen() || ImGuiBackEnd::OwnsMouse();

        for (uint64_t playerId : Session::GetLocalPlayerIds()) {
            Player* player = Session::GetPlayerById(playerId);
            if (!player) continue;

            if (disableControl) {
                player->DisableControl();
            }
            else {
                player->EnableControl();
            }
        }

        for (uint64_t playerId : Session::GetLocalPlayerIds()) {
            Player* player = Session::GetPlayerById(playerId);
            if (!player) continue;

            player->Update(deltaTime);
        }
    }
}
