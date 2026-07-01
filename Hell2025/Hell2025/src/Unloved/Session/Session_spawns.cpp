#include "Session.h"

#include "Hell/Common/Random.h"

#include "Unloved/World/World.h"

#include <glm/geometric.hpp>

namespace Unloved::Session {

    void CreateFallbackCampaignSpawnPoints() {
        World::AddSpawnPointCampaign({ glm::vec3(43.9485, 32.6516, 36.7408), glm::vec3(-0.294, -5.0020, 0) });
        World::AddSpawnPointCampaign({ glm::vec3(40.3495, 32.6486, 34.1408), glm::vec3(-0.168, -9.4820, 0) });
        World::AddSpawnPointCampaign({ glm::vec3(42.6229, 32.6482, 41.4889), glm::vec3(-0.282, -11.772, 0) });
        World::AddSpawnPointCampaign({ glm::vec3(34.7497, 35.4520, 37.4222), glm::vec3(-0.206, -15.736, 0) });
        World::AddSpawnPointCampaign({ glm::vec3(34.9035, 32.6505, 39.5006), glm::vec3(-0.146, -14.242, 0) });
        World::AddSpawnPointCampaign({ glm::vec3(34.8531, 32.6496, 33.6023), glm::vec3(-0.258, -15.138, 0) });
        World::AddSpawnPointCampaign({ glm::vec3(33.3506, 32.6481, 41.1310), glm::vec3(-0.166, -18.282, 0) });
        World::AddSpawnPointCampaign({ glm::vec3(57.3242, 33.5911, 48.8959), glm::vec3(-0.134, -18.100, 0) });
        World::AddSpawnPointCampaign({ glm::vec3(40.0950, 32.4311, 31.6613), glm::vec3(-0.110, -14.256, 0) });
    }

    void CreateFallbackDeathmatchSpawnPoints() {
        World::AddSpawnPointDeathMatch({ glm::vec3(43.9485, 32.6516, 36.7408), glm::vec3(-0.294, -5.0020, 0) });
        World::AddSpawnPointDeathMatch({ glm::vec3(40.3495, 32.6486, 34.1408), glm::vec3(-0.168, -9.4820, 0) });
        World::AddSpawnPointDeathMatch({ glm::vec3(42.6229, 32.6482, 41.4889), glm::vec3(-0.282, -11.772, 0) });
        World::AddSpawnPointDeathMatch({ glm::vec3(34.7497, 35.4520, 37.4222), glm::vec3(-0.206, -15.736, 0) });
        World::AddSpawnPointDeathMatch({ glm::vec3(34.9035, 32.6505, 39.5006), glm::vec3(-0.146, -14.242, 0) });
        World::AddSpawnPointDeathMatch({ glm::vec3(34.8531, 32.6496, 33.6023), glm::vec3(-0.258, -15.138, 0) });
        World::AddSpawnPointDeathMatch({ glm::vec3(33.3506, 32.6481, 41.1310), glm::vec3(-0.166, -18.282, 0) });
        World::AddSpawnPointDeathMatch({ glm::vec3(57.3242, 33.5911, 48.8959), glm::vec3(-0.134, -18.100, 0) });
        World::AddSpawnPointDeathMatch({ glm::vec3(40.0950, 32.4311, 31.6613), glm::vec3(-0.110, -14.256, 0) });
    }

    bool SpawnPointIsSafeDistance(const SpawnPoint& spawnPoint) {
        for (int i = 0; i < GetLocalPlayerCount(); i++) {
            Player* player = GetLocalPlayerByViewportIndex(i);
            if (!player) {
                continue;
            }

            float distanceToOtherPlayer = glm::distance(spawnPoint.GetPosition(), player->GetFootPosition());

            if (distanceToOtherPlayer < 1.0f) {
                return false;
            }
        }

        return true;
    }

    const SpawnPoint& GetRandomSafeSpawnPoint(Hell::SlotMap<SpawnPoint>& spawnPoints) {
        const int32_t spawnPointCount = static_cast<int32_t>(spawnPoints.size());
        const int32_t fallbackIndex = Hell::Random::Int(0, spawnPointCount - 1);

        for (int32_t attempt = 0; attempt < spawnPointCount; attempt++) {
            const int32_t index = Hell::Random::Int(0, spawnPointCount - 1);
            const SpawnPoint& spawnPoint = spawnPoints[index];
            if (SpawnPointIsSafeDistance(spawnPoint)) {
                return spawnPoint;
            }
        }

        return spawnPoints[fallbackIndex];
    }

    const SpawnPoint& GetRandomCampaignSpawnPoint() {
        Hell::SlotMap<SpawnPoint>& spawnPoints = World::GetSpawnPointsCampaign();

        //if (spawnPoints.empty()) {
            CreateFallbackCampaignSpawnPoints();
        //}

        return GetRandomSafeSpawnPoint(spawnPoints);
    }

    const SpawnPoint& GetRandomDeathmatchSpawnPoint() {
        Hell::SlotMap<SpawnPoint>& spawnPoints = World::GetSpawnPointsDeathMatch();

        //if (spawnPoints.empty()) {
            CreateFallbackDeathmatchSpawnPoints();
        //}

        return GetRandomSafeSpawnPoint(spawnPoints);
    }
}
