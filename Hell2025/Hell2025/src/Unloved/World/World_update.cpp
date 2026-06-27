#include "World.h"

#include "Hell/Time.h"

#include "Unloved/Editor/Editor.h"
#include "Unloved/UI/Imgui/ImguiBackEnd.h"
#include "Legacy/World/LegacyWorld.h"

#include "Unloved/Session/Session.h"

namespace Unloved::World {
    void UpdateBvhs() {
        LegacyWorld::UpdateBvhs();
    }

    void UpdatePlayers() {
        const float deltaTime = Hell::Time::DeltaTime();
        const bool disableControl = Editor::IsOpen() || ImGuiBackEnd::OwnsMouse();

        for (uint64_t playerId : Session::GetLocalPlayerIds()) {
            Unloved::Player* player = Session::GetPlayerById(playerId);
            if (!player) continue;

            if (disableControl) {
                player->DisableControl();
            }
            else {
                player->EnableControl();
            }
        }

        for (uint64_t playerId : Session::GetLocalPlayerIds()) {
            Unloved::Player* player = Session::GetPlayerById(playerId);
            if (!player) continue;

            player->Update(deltaTime);
        }
    }

    void UpdateLegacyObjects() {
        LegacyWorld::Update(Hell::Time::DeltaTime());
    }
}
