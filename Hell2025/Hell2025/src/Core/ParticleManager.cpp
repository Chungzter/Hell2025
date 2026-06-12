#include "ParticleManager.h"
#include "Input/Input.h"
#include "Core/Game.h"
#include "Ocean/Ocean.h"
#include "Util/Util.h"

namespace ParticleManager {
    std::vector<Particle> g_particles;
    float g_bubbleSpawnCooldownTimer = 0.0f;
    float g_bubbleSpawnCooldownMax = 0.2f;


    void SpawnParticle(const glm::vec3& position);

    void Update(float deltaTime) {

        Player* player = Game::GetLocalPlayerByIndex(0);
        if (!player) return;

        if (player->IsMoving()) {
            g_bubbleSpawnCooldownTimer = g_bubbleSpawnCooldownMax;
        }

        if (player->GetCameraPosition().y > Ocean::GetOceanOriginY()) {
            g_bubbleSpawnCooldownTimer = 0.0f;
        }

        if (g_bubbleSpawnCooldownTimer > 0) {
            g_bubbleSpawnCooldownTimer -= Game::GetDeltaTime();

            bool spawn = Util::RandomInt(0, 2) == 1;

            if (spawn) {

                const glm::vec3& cameraPosition = player->GetCameraPosition();
                const glm::vec3& cameraForward = player->GetCameraForward();
                const glm::vec3& cameraRight = player->GetCameraRight();

                glm::vec3 spawnPos = cameraPosition;
                spawnPos += (cameraForward * 0.4f) + Util::RandomFloat(-0.05f, 0.05f);
                spawnPos += (cameraRight * Util::RandomFloat(-0.3f, 0.3f));
                SpawnParticle(spawnPos);

                //spawnPos = cameraPosition;
                //spawnPos += (cameraForward * 0.4f) + Util::RandomFloat(-0.05f, 0.05f);
                //spawnPos -= (cameraRight * 0.25f) + Util::RandomFloat(-0.1f, 0.1f);
                //SpawnParticle(spawnPos);
            }
        }

        g_bubbleSpawnCooldownTimer = std::max(g_bubbleSpawnCooldownTimer, 0.0f);

        if (Input::KeyDown(HELL_KEY_4)) {
            glm::vec3 position = glm::vec3(36.25, 32.0, 37.0);
            SpawnParticle(position);
        }

        for (Particle& particle : g_particles) {
            particle.position += particle.velocity * deltaTime;
            particle.alphaFade -= deltaTime * 1.0f;
            particle.scale -= deltaTime * 0.1f;

            particle.scale = std::max(particle.scale, 0.0f);
            particle.alphaFade = std::max(particle.alphaFade, 0.0f);
        }
    }

    void SpawnParticle(const glm::vec3& position) {
        Particle& particle = g_particles.emplace_back();
        particle.position = position;
        particle.velocity.x = Util::RandomFloat(-0.3, 0.3);
        particle.velocity.y = Util::RandomFloat(0.7, 1.2);
        particle.velocity.z = Util::RandomFloat(-0.3, 0.3);
        particle.rotation = Util::RandomFloat(0, HELL_PI * 2.0f);
        particle.scale = Util::RandomFloat(0.1, 0.025);
        particle.alphaFade = 1.0f;
    }

    std::vector<Particle>& GetParticles() {
        return g_particles;
    }
}