#include "LegacyWorld.h"
#include "Unloved/Session/Session.h"
#include "Unloved/SubSystems/GameAudio.h"
#include "Viewport/ViewportManager.h"
#include "Renderer/Renderer.h"
#include "Renderer/RenderDataManager.h"

#include "Game/UniqueID.h"

namespace LegacyWorld {
    bool g_awaitingFleshAudio = false;

    void SpawnBlood(const glm::vec3& position, const glm::vec3& direction);

    void ProcessDobermannHit(uint64_t objectId, uint64_t physicsId, const Bullet& bullet, const glm::vec3& hitPosition);
    void ProcessKangarooHit(uint64_t objectId, uint64_t physicsId, const Bullet& bullet, const glm::vec3& hitPosition);
    void ProcessPlayerHit(uint64_t objectId, uint64_t physicsId, const Bullet& bullet, const glm::vec3& hitPosition);
    void ProcessSharkHit(uint64_t objectId, uint64_t physicsId, const Bullet& bullet, const glm::vec3& hitPosition);

    void ProcessBullets() {
        std::vector<Bullet>& bullets = GetBullets();
        std::vector<Bullet> newBullets;
        bool glassWasHit = false;
        bool rooDeath = false;

        for (Bullet& bullet : bullets) {
            // Cast PhysX ray
            glm::vec3 rayOrigin = bullet.GetOrigin();
            glm::vec3 rayDirection = bullet.GetDirection();
            float rayLength = bullet.GetRayLength();

            std::vector<PxRigidActor*> ignoredActors;

            int playerCount = Unloved::Session::GetLocalPlayerCount();
            for (int i = 0; i < playerCount; i++) {
                if (Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(i)) {
                    if (PxRigidDynamic* characterControllerActor = player->GetCharacterControllerActor()) {
                        ignoredActors.push_back(characterControllerActor);
                    }
                }
            }

            Player* player = Unloved::Session::GetPlayerById(bullet.GetOwnerObjectId());
            if (player) {
                auto RagdollActors = Hell::Physics::GetRagdollPxRigidActors(player->GetRagdollId());
                ignoredActors.insert(ignoredActors.end(), RagdollActors.begin(), RagdollActors.end());
            }

            PhysXRayResult physXRayResult = Hell::Physics::CastPhysXRay(rayOrigin, rayDirection, rayLength, false, ignoredActors);
            BvhRayResult bvhRayResult = LegacyWorld::ClosestHit(rayOrigin, bullet.GetDirection(), rayLength);

            // Defaults
            bool hitFound = false;
            uint64_t objectId = 0;
            uint64_t physicsId = 0;
            int32_t localMeshNodeIndex = -1;
            glm::vec3 hitPosition = glm::vec3(0.0f);
            glm::vec3 hitNormal = glm::vec3(0.0f);

            // BVH hit
            if (bvhRayResult.hitFound) {
                hitFound = true;
                objectId = bvhRayResult.objectId;
                physicsId = 0;
                localMeshNodeIndex = bvhRayResult.localMeshNodeIndex;
                hitPosition = bvhRayResult.hitPosition;
                hitNormal = bvhRayResult.hitNormal;

                //std::cout << "bvh hit found: " << bvhRayResult.hitPosition << "\n";
            }

            // PhysX hit
            if (physXRayResult.hitFound) {
                float distToPhysXHit = glm::distance(physXRayResult.hitPosition, bullet.GetOrigin());

                //std::cout << "physX hit found: " << physXRayResult.hitPosition << "\n";

                // Overwrite only if PhysX hit is closer
                if (!bvhRayResult.hitFound || bvhRayResult.hitFound && distToPhysXHit < bvhRayResult.distanceToHit) {
                    hitFound = true;
                    objectId = physXRayResult.userData.objectId;
                    physicsId = physXRayResult.userData.physicsId;
                    localMeshNodeIndex = -1;
                    hitPosition = physXRayResult.hitPosition;
                    hitNormal = physXRayResult.hitNormal;

                    //std::cout << "physX hit closer" << "\n";
                    //LegacyWorld::GetPictureFrames()[0].SetPosition(physXRayResult.hitPosition);
                   // LegacyWorld::GetPictureFrames()[0].SetScale(glm::vec3(0.0f));
                }
            }

            // Hit found?
            if (hitFound) {

                ObjectType hitObjectType = UniqueID::GetType(objectId);
                Hell::Physics::PhysicsObjectType physicsObjectType = Hell::Physics::GetPhysicsObjectType(physicsId);

                std::cout << "\n";
                std::cout << "Hit found " << Unloved::Session::GetSessionTime() << "\n";
                std::cout << " ObjectId          " << objectId << "\n";
                std::cout << " PhysicsId         " << physicsId << "\n";
                std::cout << " ObjectType        " << Util::EnumToString(hitObjectType) << "\n";
                std::cout << " PhysicsObjectType " << Util::EnumToString(physicsObjectType) << "\n";

                glm::vec3 appliedForce = rayDirection * glm::vec3(5.0f);
                if (physicsObjectType == Hell::Physics::PhysicsObjectType::RAGDOLL) {
                    Hell::Physics::AddForceToRagdoll(physicsId, appliedForce);
                }
                else if (physicsObjectType == Hell::Physics::PhysicsObjectType::RIGID_DYNAMIC) {
                    Hell::Physics::AddFoceToRigidDynamic(physicsId, appliedForce);
                }

                if (hitObjectType == ObjectType::DOBERMANN) ProcessDobermannHit(objectId, physicsId, bullet, hitPosition);
                if (hitObjectType == ObjectType::KANGAROO)  ProcessKangarooHit(objectId, physicsId, bullet, hitPosition);
                if (hitObjectType == ObjectType::PLAYER)    ProcessPlayerHit(objectId, physicsId, bullet, hitPosition);
                if (hitObjectType == ObjectType::SHARK)     ProcessSharkHit(objectId, physicsId, bullet, hitPosition);
              
                if (hitObjectType == ObjectType::RAGDOLL_STANDALONE && physicsObjectType == Hell::Physics::PhysicsObjectType::RAGDOLL) {
                    Unloved::GameAudio::TryPlayFleshImpactAudio();
                    SpawnBlood(hitPosition, rayDirection);
                }

                // Bullet enters water
                if (hitObjectType == ObjectType::WATER_PLANE_TOP) {
                    BulletCreateInfo createInfo = bullet.GetCreateInfo();
                    createInfo.origin = hitPosition + (bullet.GetDirection() * 0.1f);

                    AddBulletTrail(createInfo);
                    std::cout << "Spawning a new bullet trail beneath the ocean\n";
                }

                // Bullet leaves water
                if (hitObjectType == ObjectType::WATER_PLANE_BOTTOM) {
                    BulletCreateInfo bulletCreateInfo;
                    bulletCreateInfo.origin = hitPosition + (bullet.GetDirection() * 0.1f);
                    bulletCreateInfo.direction = bullet.GetDirection();
                    bulletCreateInfo.damage = bullet.GetDamage();
                    bulletCreateInfo.weaponIndex = bullet.GetWeaponIndex();
                    bulletCreateInfo.ownerObjectId = bullet.GetOwnerObjectId();
                    bulletCreateInfo.rayLength = 1000.0f;
                    newBullets.emplace_back(Bullet(bulletCreateInfo));

                    std::cout << "Spawning a new bullet exiting the ocean\n";
                }

                // If this bullet belongs to a bullet trail, then destroy the trail
                if (bullet.GetParentBulletTrailId() != 0) {
                    LegacyWorld::RemoveObject(bullet.GetParentBulletTrailId());
                }

                // Retrieve the hit MeshNode, this could be nullptr if the hit was a physics object
                MeshNode* meshNode = LegacyWorld::GetMeshNodeByObjectIdAndLocalNodeIndex(objectId, localMeshNodeIndex);

                bool glassHit = meshNode && (meshNode->blendingMode == BlendingMode::GLASS ||
                                             meshNode->blendingMode == BlendingMode::MIRROR ||
                                             meshNode->blendingMode == BlendingMode::STAINED_GLASS);

                bool createDecal = (meshNode && meshNode->decalType != DecalType::UNDEFINED) ||
                                   (Hell::Physics::GetRigidStaitcById(physicsId) != nullptr);

                if (!bullet.CreatesDecals()) {
                    createDecal = false;
                }

                // Create the decal
                if (createDecal) {
                    DecalCreateInfo decalCreateInfo;
                    decalCreateInfo.surfaceHitPosition = hitPosition;
                    decalCreateInfo.parentObjectId = objectId;
                    decalCreateInfo.localMeshNodeIndex = localMeshNodeIndex;
                    decalCreateInfo.surfaceHitNormal = hitNormal;
                    AddDecal2(decalCreateInfo);

                    // Create second decal on opposite side if glass was hit + spawn a new bullet
                    if (glassHit) {
                        decalCreateInfo.surfaceHitNormal *= glm::vec3(-1.0f);
                        AddDecal2(decalCreateInfo);

                        if (bullet.CreatesFolloWThroughBulletOnGlassHit() && meshNode->blendingMode != BlendingMode::MIRROR) {
                            BulletCreateInfo bulletCreateInfo;
                            bulletCreateInfo.origin = hitPosition + bullet.GetDirection() * glm::vec3(0.05f);
                            bulletCreateInfo.direction = bullet.GetDirection();
                            bulletCreateInfo.damage = bullet.GetDamage();
                            bulletCreateInfo.weaponIndex = bullet.GetWeaponIndex();
                            bulletCreateInfo.ownerObjectId = bullet.GetOwnerObjectId();
                            newBullets.emplace_back(Bullet(bulletCreateInfo));
                        }

                        Unloved::GameAudio::PlayGlassHitAudio();
                    }
                }

                // Trigger the closest piano note on piano hit
                if (bullet.PlaysPiano()) {
                    if (Piano* piano = LegacyWorld::GetPianoByObjectId(objectId)) {
                        piano->TriggerInternalNoteFromExternalBulletHit(hitPosition);
                    }
                }

                // Decal texture painting
                if (bullet.CreatesDecalTexturePaintedWounds()) {
                    DecalPaintingInfo decalPaintingInfo;
                    decalPaintingInfo.rayOrigin = bullet.GetOrigin();
                    decalPaintingInfo.rayDirection = bullet.GetDirection();
                    RenderDataManager::SubmitDecalPaintingInfo(decalPaintingInfo);
                }

                // This is probably sketchy...
                if (PickUp* pickUp = LegacyWorld::GetPickUpByObjectId(objectId)) {
                    float strength = 250.0f;
                    glm::vec3 force = bullet.GetDirection() * strength;
                    pickUp->GetMeshNodes().AddForceToPhsyics(force);
                }
                if (GenericObject* object = LegacyWorld::GetGenericObjectById(objectId)) {
                    float strength = 250.0f;
                    glm::vec3 force = bullet.GetDirection() * strength;
                    object->GetMeshNodes().AddForceToPhsyics(force);
                }
            }
        }

        // Wipe old bullets, and replace with any new ones that got spawned from glass hits
        bullets = newBullets;
    }

    // Spawn blood

    void SpawnBlood(const glm::vec3& position, const glm::vec3& direction) {
        AddVATBlood(position, direction);

        // For the screen space decal blood, cast a ray directly down and create it at the ray hit position
        glm::vec3 rayOrigin = position;
        glm::vec3 rayDirection = glm::vec3(0.0f, -1.0f, 0.0f);
        float rayLength = 100;
        PhysXRayResult rayResult = Hell::Physics::CastPhysXRayStaticEnvironment(rayOrigin, rayDirection, rayLength);

        if (rayResult.hitFound) {
            ScreenSpaceBloodDecalCreateInfo decalCreateInfo;
            decalCreateInfo.position = rayResult.hitPosition;
            decalCreateInfo.direction = direction;
            AddScreenSpaceBloodDecal(decalCreateInfo);
        }
    }

    // Dobermann hit

    void ProcessDobermannHit(uint64_t objectId, uint64_t physicsId, const Bullet& bullet, const glm::vec3& hitPosition) {
        Dobermann* dobermann = LegacyWorld::GetDobermannByObjectId(objectId);
        if (!dobermann) return;

        dobermann->TakeDamage(bullet.GetDamage());

        SpawnBlood(hitPosition, -bullet.GetDirection());
        Unloved::GameAudio::TryPlayFleshImpactAudio();
    }

    // Shark hit

    void ProcessSharkHit(uint64_t objectId, uint64_t physicsId, const Bullet& bullet, const glm::vec3& hitPosition) {
        Shark* shark = LegacyWorld::GetSharkByObjectId(objectId);
        if (!shark) return;

        shark->GiveDamage(bullet.GetOwnerObjectId(), bullet.GetDamage());

        SpawnBlood(hitPosition, -bullet.GetDirection());
        Unloved::GameAudio::TryPlayFleshImpactAudio();
    }

    // Kangaroo hit

    void ProcessKangarooHit(uint64_t objectId, uint64_t physicsId, const Bullet& bullet, const glm::vec3& hitPosition) {
        Kangaroo* kangaroo = LegacyWorld::GetKangarooByObjectId(objectId);
        if (!kangaroo) return;

        kangaroo->GiveDamage(bullet.GetDamage());

        SpawnBlood(hitPosition, -bullet.GetDirection());
        Unloved::GameAudio::TryPlayFleshImpactAudio();
    }

    // Player hit

    void ProcessPlayerHit(uint64_t objectId, uint64_t physicsId, const Bullet& bullet, const glm::vec3& hitPosition) {
        Player* player = Unloved::Session::GetPlayerById(objectId);
        if (!player) return;

        Ragdoll* ragdoll = player->GetRagdoll();
        if (!ragdoll) return;

        // Head shot
        if (ragdoll->GetBoneNameByPhysicsId(physicsId) == "CC_Base_Head") {
            player->Kill(true);
            std::cout << "[Player head shot]\n";
        }
        // body shot
        else {
            player->GiveDamage(bullet.GetDamage(), bullet.GetOwnerObjectId());
            std::cout << "[Player body shot]\n";
        }

        Unloved::GameAudio::TryPlayFleshImpactAudio();
        SpawnBlood(hitPosition, -bullet.GetDirection());
    }
}
