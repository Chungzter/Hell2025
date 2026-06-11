#include "World.h"
#include "Input/Input.h"
#include "Core/Game.h"
#include "Renderer/Renderer.h"
#include <cstdlib>

namespace World {

    struct BulletTrail {
        glm::vec3 position;
        glm::vec3 forward;
        float speed;
        float distanceTraveled;
        float maxDistance;
        float phaseAccumulator;
        float randomPhaseOffset;
    };

    struct TrailParticle {
        glm::vec3 position;
        glm::vec3 velocity;
        float rotation;
        float rotationalVelocity;
    };

    std::vector<BulletTrail> g_bulletTrails;
    std::vector<TrailParticle> g_particles;

    void HackTest() {
        float deltaTime = Game::GetDeltaTime();

        glm::vec3 origin = glm::vec3(36.0f, 32.5f, 37.0f);
        glm::vec3 forward = glm::normalize(glm::vec3(0.0f, 0.25f, -1.0f));

        if (Input::KeyPressed(HELL_KEY_E)) {
            g_bulletTrails.clear();
            g_particles.clear();
        }

        if (Input::KeyPressed(HELL_KEY_P)) {
            BulletTrail b;
            b.position = origin;
            b.forward = forward;
            b.speed = 60.0f;
            b.distanceTraveled = 0.0f;
            b.maxDistance = 120.0f;
            b.phaseAccumulator = 0.0f;
            b.randomPhaseOffset = (static_cast<float>(rand()) / RAND_MAX) * 100.0f;

            g_bulletTrails.push_back(b);
        }

        float spiralFrequency = 3.0f;
        float spiralScale = 0.0125f;
        float particleSpacing = 0.01f;

        for (size_t i = 0; i < g_bulletTrails.size(); ) {
            BulletTrail& bullet = g_bulletTrails[i];

            // TODO: 
            // update bullet.maxDistance with any BVH of PhysX scene hit, plus some threshold so bullets still register

            // Determine remaining distance
            float remainingDistance = bullet.maxDistance - bullet.distanceTraveled;

            // Determine step distance
            glm::vec3 translation = bullet.forward * bullet.speed * deltaTime;
            float desiredStepDistance = glm::length(translation);
            float stepDistance = std::min(desiredStepDistance, std::max(0.0f, remainingDistance));

            // Determine new position
            glm::vec3 oldPos = bullet.position;
            glm::vec3 translationDir = bullet.forward;
            bullet.position += translationDir * stepDistance;
            bullet.distanceTraveled += stepDistance;

            // Determine up and right vectors
            // TODO: do it when creating the bullet trail, it doesn't change
            glm::vec3 tempUp = (glm::abs(bullet.forward.y) < 0.999f) ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
            glm::vec3 right = glm::normalize(glm::cross(bullet.forward, tempUp));
            glm::vec3 up = glm::cross(right, bullet.forward);

            float traveledThisFrame = 0.0f;

            while (traveledThisFrame < stepDistance) {
                bullet.phaseAccumulator += particleSpacing;

                // Determine corkscrew spiral offset
                float segmentRatio = traveledThisFrame / stepDistance;
                glm::vec3 interpolatingPos = glm::mix(oldPos, bullet.position, segmentRatio);
                float theta = (bullet.phaseAccumulator + bullet.randomPhaseOffset) * spiralFrequency;
                float radius = (std::sin(bullet.phaseAccumulator * 0.4f) * 0.5f + 0.5f) * spiralScale;
                glm::vec3 offset = (right * std::cos(theta) + up * std::sin(theta)) * radius;

                // Create the particle
                TrailParticle& particle = g_particles.emplace_back();
                particle.position = interpolatingPos + offset;

                // Jitter particle spawn position
                float jitterX = Util::RandomFloat(-1.0f, 1.0f);
                float jitterY = Util::RandomFloat(-1.0f, 1.0f);
                float jitterZ = Util::RandomFloat(-1.0f, 1.0f);
                particle.position += glm::vec3(jitterX, jitterY, jitterZ) * 0.015f;

                // Particle velocity
                float driftSpeed = Util::RandomFloat(0.05f, 0.2f);
                float recoilSpeed = Util::RandomFloat(0.1f, 0.3f);
                glm::vec3 drift = glm::normalize(offset) * driftSpeed;
                glm::vec3 recoil = -bullet.forward * recoilSpeed;
                particle.velocity = drift + recoil;
                particle.rotation = Util::RandomFloat(0.0f, HELL_PI * 2.0f);
                particle.rotationalVelocity = Util::RandomFloat(-25.0f, 25.0f);

                // Increment particle spacing
                traveledThisFrame += particleSpacing;
            }

            // Remove bullet if it traveled its max distance
            if (bullet.distanceTraveled >= bullet.maxDistance) {
                g_bulletTrails.erase(g_bulletTrails.begin() + i);
            }
            else {
                i++;
            }

            // Segment this bullet trail traveled this frame
            glm::vec3 p1 = oldPos;
            glm::vec3 p2 = bullet.position;
            Renderer::DrawLine(p1, p2, YELLOW);
        }

        for (auto& p : g_particles) {
            // Water resistance
            p.velocity *= 0.95f;
            p.rotationalVelocity *= 0.98f;

            // Step physics
            p.position += p.velocity * deltaTime;
            p.rotation += p.rotationalVelocity * deltaTime;

            Renderer::DrawPoint(p.position, RED);

            //float lineLength = 0.15f;
            //glm::vec3 spinOffset = glm::vec3(std::cos(p.rotation), std::sin(p.rotation), 0.0f) * lineLength;
            //Renderer::DrawLine(p.position, p.position + spinOffset, GREEN);
        }
    }
}