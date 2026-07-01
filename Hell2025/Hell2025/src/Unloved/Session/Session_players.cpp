#include "Session.h"

#include "Hell/Containers/SlotMap.h"
#include "Hell/Logging.h"

#include "Unloved/ObjectId.h"

namespace Unloved::Session {
    Hell::SlotMap<Unloved::Player> g_players(8);
    std::vector<uint64_t> g_playerIds;
    std::vector<uint64_t> g_localPlayerIds;
    std::vector<uint64_t> g_remotePlayerIds;

    void AddLocalPlayer(const glm::vec3& position, const glm::vec3& rotation) {
        if (g_localPlayerIds.size() == 4) {
            return;
        }

        const uint64_t playerId = Unloved::GetNextObjectId(ObjectType::PLAYER);
        const int32_t viewportIndex = static_cast<int32_t>(g_localPlayerIds.size());
        if (!g_players.emplace_with_id(playerId)) {
            return;
        }

        Unloved::Player* player = g_players.get(playerId);
        if (!player) {
            return;
        }

        player->Init(playerId, position, rotation, viewportIndex);
        g_playerIds.push_back(playerId);
        g_localPlayerIds.push_back(playerId);
    }

    void AddRemotePlayer(const glm::vec3& position, const glm::vec3& rotation) {
        const uint64_t playerId = Unloved::GetNextObjectId(ObjectType::PLAYER);
        if (!g_players.emplace_with_id(playerId)) {
            return;
        }

        Unloved::Player* player = g_players.get(playerId);
        if (!player) {
            return;
        }

        player->Init(playerId, position, rotation, -1);
        g_playerIds.push_back(playerId);
        g_remotePlayerIds.push_back(playerId);
    }

    void BeginFrame() {
        for (uint64_t playerId : g_localPlayerIds) {
            if (Unloved::Player* player = GetPlayerById(playerId)) {
                player->BeginFrame();
            }
        }
    }

    void RespawnPlayers() {
        for (uint64_t playerId : g_localPlayerIds) {
            if (Unloved::Player* player = GetPlayerById(playerId)) {
                player->Respawn();
            }
        }
    }

    const std::vector<uint64_t>& GetLocalPlayerIds() {
        return g_localPlayerIds;
    }

    Unloved::Player* GetPlayerById(uint64_t playerId) {
        return g_players.get(playerId);
    }

    Unloved::Player* GetLocalPlayerByViewportIndex(uint32_t index) {
        for (uint64_t playerId : g_localPlayerIds) {
            Unloved::Player* player = GetPlayerById(playerId);
            if (player && player->GetViewportIndex() == static_cast<int32_t>(index)) {
                return player;
            }
        }
        return nullptr;
    }

    Unloved::Camera* GetLocalPlayerCameraByViewportIndex(uint32_t index) {
        if (Unloved::Player* player = GetLocalPlayerByViewportIndex(index)) {
            return &player->GetCamera();
        }
        else {
            Logging::Debug() << "Session::GetLocalPlayerCameraByViewportIndex(..) failed. " << index << " out of range of local player count " << g_localPlayerIds.size() << "\n";
            return nullptr;
        }
    }

    float GetLocalPlayerFovByViewportIndex(uint32_t index) {
        if (Unloved::Player* player = GetLocalPlayerByViewportIndex(index)) {
            return player->GetFov();
        }
        else {
            Logging::Debug() << "Session::GetLocalPlayerFovByViewportIndex(..) failed. " << index << " out of range of local player count " << g_localPlayerIds.size() << "\n";
            return 1.0f;
        }
    }

    int32_t GetPlayerCount() {
        return static_cast<int32_t>(g_playerIds.size());
    }

    int32_t GetLocalPlayerCount() {
        return static_cast<int32_t>(g_localPlayerIds.size());
    }

    int32_t GetRemotePlayerCount() {
        return static_cast<int32_t>(g_remotePlayerIds.size());
    }

    int32_t GetOnlinePlayerCount() {
        return GetRemotePlayerCount();
    }

    void SetPlayerKeyboardAndMouseIndex(int playerIndex, int keyboardIndex, int mouseIndex) {
        if (playerIndex < 0) {
            return;
        }

        if (Unloved::Player* player = GetLocalPlayerByViewportIndex(static_cast<uint32_t>(playerIndex))) {
            player->SetKeyboardIndex(keyboardIndex);
            player->SetMouseIndex(mouseIndex);
        }
    }
}
