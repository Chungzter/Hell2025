#include "World.h"
#include "Core/GameOLD.h"
#include "Renderer/Renderer.h"
#include "Types/Game/BulletTrail.h"
#include <cstdlib>
#include "Hell/Input.h"
namespace Input = Hell::Input;


namespace World {

    void UpdateBulletTrails(float deltaTime);
    void UpdateBulletTrailParticles(float deltaTime);

    void HackTest() {
        if (Input::KeyPressed(HELL_KEY_E)) {
            Hell::SlotMap<BulletTrail>& bulletTrails = GetBulletTrails();
            std::vector<BulletTrailParticle>& bulletTrailParticles = GetBulletTrailParticles();
            bulletTrails.clear();
            bulletTrailParticles.clear();
        }

        float deltaTime = GameOLD::GetDeltaTime();

        UpdateBulletTrails(deltaTime);
        UpdateBulletTrailParticles(deltaTime);
    }

    void UpdateBulletTrails(float deltaTime) {
        Hell::SlotMap<BulletTrail>& bulletTrails = GetBulletTrails();
        std::vector<BulletTrailParticle>& bulletTrailParticles = GetBulletTrailParticles();

        for (BulletTrail& bulletTrail : bulletTrails) {
            bulletTrail.Update(deltaTime);

            // Remove bullet if it traveled its max distance
            if (bulletTrail.m_distanceTraveled >= bulletTrail.m_maxDistance) {
                RemoveObject(bulletTrail.m_objectId);
            }
        }
    }

    void UpdateBulletTrailParticles(float deltaTime) {
        return;

        // THIS IS ALL DONE ON THE GPU NOIW
        // THIS IS ALL DONE ON THE GPU NOIW
        // THIS IS ALL DONE ON THE GPU NOIW
        // THIS IS ALL DONE ON THE GPU NOIW

        std::vector<BulletTrailParticle>& bulletTrailParticles = GetBulletTrailParticles();

        for (int i = 0; i < bulletTrailParticles.size();) {
            BulletTrailParticle& particle = bulletTrailParticles[i];

            // Water resistance
            particle.velocity *= 0.95f;
            particle.rotationalVelocity *= 0.98f;
            particle.lifeTime += deltaTime;

            // Remove if lifetime exceeds some value
            if (particle.lifeTime > 0.4f) {
                bulletTrailParticles.erase(bulletTrailParticles.begin() + i);
                continue;
            }
            else {
                i++;
            }

            // Step physics
            particle.position += particle.velocity * deltaTime;
            particle.rotation += particle.rotationalVelocity * deltaTime;

            //DebugDraw::DrawPoint(particle.position, RED);

            //float lineLength = 0.15f;
            //glm::vec3 spinOffset = glm::vec3(std::cos(particle.rotation), std::sin(particle.rotation), 0.0f) * lineLength;
            //DebugDraw::DrawLine(particle.position, particle.position + spinOffset, GREEN);
        }
    }
}