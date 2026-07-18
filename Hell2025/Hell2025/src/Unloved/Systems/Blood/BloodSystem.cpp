#include "BloodSystem.h"

#include "Hell/Common/Color.h"
#include "Hell/Common/Random.h"
#include "Hell/Common/String.h"
#include "Hell/Debug/DebugDraw.h"
#include "Hell/Input.h"
#include "Hell/Physics.h"
#include "Hell/Time.h"
#include "Hell/Transform.h"

#include "Unloved/Debug/Debug.h"
#include "Unloved/Objects/Renderables/VATInstance.h"
#include "Unloved/Systems/WorldBVH/WorldBVH.h"

namespace {
    std::vector<TestParticle> g_particles;
    std::vector<VATInstance> g_vatInstances;
    std::vector<VATRenderItem> g_vatRenderItems;
    int32_t g_magic = 0;

}

namespace Unloved::BloodSystem {

    const std::vector<VATRenderItem>& GetVATRenderItems() { return g_vatRenderItems; }
    const std::vector<TestParticle>& GetTestParticles()   { return g_particles; }

    void UpdateVATInstances();
    void UpdateParticles();

    void BeginFrame() {
        g_vatRenderItems.clear();

        for (int i = (int)g_vatInstances.size() - 1; i >= 0; i--) {
            if (g_vatInstances[i].IsAnimationComplete()) {
                g_vatInstances.erase(g_vatInstances.begin() + i);
            }
        }
    }

    void Update() {
        UpdateVATInstances();
        UpdateParticles();
    }

    void UpdateVATInstances() {
        for (VATInstance& vatInstance : g_vatInstances) {
            vatInstance.Update(Hell::Time::DeltaTime());
            g_vatRenderItems.emplace_back(vatInstance.CreateRenderItem());
        }
    }

    void UpdateParticles() {
        for (int i = 0; i < g_particles.size(); i++) {
            g_particles[i].Update(Hell::Time::DeltaTime());

            const glm::vec3& rayOrigin = g_particles[i].m_position;
            const glm::vec3 rayDir = glm::normalize(g_particles[i].m_positionPrev - g_particles[i].m_position);
            float rayLength = glm::distance(g_particles[i].m_positionPrev, g_particles[i].m_position);

            //g_particles[i].DebugDraw(i);

            if (g_particles[i].m_lifeTime < 0.1f) continue;

            // PhysX ray
            PhysXRayResult physXRayResult = Hell::Physics::CastPhysXRay(rayOrigin, rayDir, rayLength, false);
            if (physXRayResult.hitFound) {
                g_particles[i].m_position = physXRayResult.hitPosition;
                g_particles[i].m_finalRestingNormal = rayDir;
                g_particles[i].m_stopped = true;
                continue;
            }

            // BVH ray
            BvhRayResult bvhRayResult = WorldBVH::ClosestHit(rayOrigin, rayDir, rayLength);
            if (bvhRayResult.hitFound) {
                g_particles[i].m_position = bvhRayResult.hitPosition;
                g_particles[i].m_finalRestingNormal = rayDir;
                g_particles[i].m_stopped = true;
                continue;
            }
        }
    }

    void SpawnVatBlood(const glm::vec3& position, const glm::vec3& forward, float scale, uint64_t parentHitObjectId) {
        g_magic++;
        g_magic = g_magic % 6;

        VATInstance& vatInstance = g_vatInstances.emplace_back();

        VATInstanceCreateInfo createInfo;
        createInfo.playbackSpeed = 7.5f;
        createInfo.loop = false;
        createInfo.worldPosition = position;
        createInfo.worldForward = forward;
        createInfo.scale = scale;

        if (g_magic == 0) {
            createInfo.resourceName = "Blood19";
            createInfo.mirror = false;
        }
        if (g_magic == 1) {
            createInfo.resourceName = "Blood20";
            createInfo.mirror = false;
        }
        if (g_magic == 2) {
            createInfo.resourceName = "Blood22";
            createInfo.mirror = false;
        }
        if (g_magic == 3) {
            createInfo.resourceName = "Blood19";
            createInfo.mirror = true;
        }
        if (g_magic == 4) {
            createInfo.resourceName = "Blood20";
            createInfo.mirror = true;
        }
        if (g_magic == 5) {
            createInfo.resourceName = "Blood22";
            createInfo.mirror = true;
        }

        vatInstance.Init(createInfo);

        // TODO: figure out how you are gonna do the this thing: parentHitObjectId
        // TODO: figure out how you are gonna do the this thing: parentHitObjectId
        // TODO: figure out how you are gonna do the this thing: parentHitObjectId

        uint32_t particleCount = 20;

        Hell::LocalFrame localFrame = Hell::LocalFrame(forward);
        Hell::QuatTransform transform = Hell::QuatTransform(position, localFrame, glm::vec3(0.05f));

        for (uint32_t i = 0; i < particleCount; i++) {

            glm::vec3 vel = localFrame.right * Hell::Random::Float(-1.0f, 1.0f) +
                localFrame.up * Hell::Random::Float(0.1f, 1.0f) +
                localFrame.forward * Hell::Random::Float(0.0f, 0.5f);

            g_particles.push_back(TestParticle(position, vel));
        }
    }
}

TestParticle::TestParticle(const glm::vec3& position, const glm::vec3& velocity) {
    m_position = position;
    m_positionPrev = position;
    m_velocity = velocity;
}

void TestParticle::Update(float deltaTime) {
    m_positionPrev = m_position;
    m_lifeTime += deltaTime;

    if (!m_stopped) {
        m_velocity.y += m_gravity * deltaTime;
        m_position += m_velocity * deltaTime; \
    }
}

void TestParticle::DebugDraw(int32_t randomSeed) {
    Hell::DebugDraw::DrawPoint(m_position, glm::vec4(Hell::Color::Random(randomSeed), 1.0f));
    Hell::DebugDraw::DrawLine(m_position, m_positionPrev, glm::vec4(Hell::Color::Random(randomSeed), 1.0f));
}