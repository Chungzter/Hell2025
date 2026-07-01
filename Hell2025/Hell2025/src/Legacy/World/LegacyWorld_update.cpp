#include "LegacyWorld.h"

#include "Unloved/Systems/Pathfinding/AStarMap.h"
#include "Unloved/Viewport/ViewportManager.h"
#include "Unloved/Bible/Bible.h"
#include "Unloved/Objects/House/DoorChain.h"
#include "Unloved/ObjectId.h"
#include "Unloved/Systems/Blood/BloodSystem.h"

#include "Hell/Audio.h"
#include "Hell/Containers/SlotMap.h"
#include "Hell/Logging.h"
#include "Hell/Input.h"

#include "Unloved/Systems/P90Mag/P90MagManager.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Systems/DirtyTracker/DirtyTracker.h"
#include "Unloved/World/World.h"

#include "Legacy/Renderer/RenderDataManager.h"
#include "Legacy/Renderer/Renderer.h"

namespace Input = Hell::Input;

namespace Unloved::LegacyWorld {

    uint64_t g_testAnimatedGameObject = 0;
    AnimatedGameObject* GetDobermannTest() {
        return Unloved::World::GetAnimatedGameObjectByObjectId(g_testAnimatedGameObject);
    }

    uint64_t g_trapKingID = 0;
    uint64_t g_ratKidAO = 0;
    std::vector<SpriteSheetObject> g_bubbleSpriteSheetObjects;


    std::vector<SpriteSheetObject>& GetBubbleSpriteSheetObjects() {
        return g_bubbleSpriteSheetObjects;
    }

    AnimatedGameObject* GetTrapKingAO() {
        return Unloved::World::GetAnimatedGameObjectByObjectId(g_trapKingID);
    }
    AnimatedGameObject* GetRadKidAO() {
        return Unloved::World::GetAnimatedGameObjectByObjectId(g_ratKidAO);
    }


	void InitRatKing(const std::string& modelName) {
        RemoveObject(g_ratKidAO);

		g_ratKidAO = CreateAnimatedGameObject();
        AnimatedGameObject* ratKidAO = GetRadKidAO();

        ratKidAO->SetSkinnedModel("RatKing", "RatKing");
        ratKidAO->SetSkinnedModel(modelName, "RatKing");

        ratKidAO->SetPosition(glm::vec3(37.0f, 31.0f, 36.23f));
        ratKidAO->PlayAndLoopAnimation("Main", "RatKid_GlockIdle3", 1.0f);

    }

    void Update(float deltaTime) {

        if (Input::KeyPressed(HELL_KEY_4)) {
            Unloved::Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(0);
            player->SetFootPosition(glm::vec3(34.49f, 31.0f, 37.48f));
            player->GetCamera().SetEulerRotation(glm::vec3(-0.15f, 1.58f, 0.0f));
        }

        if (Input::KeyPressed(HELL_KEY_NUMPAD_1)) {
            AnimatedGameObject* ratKidAO = GetRadKidAO();
            ratKidAO->SetAnimationModeToBindPose();
            for (Light& light : Unloved::World::GetLights()) {
                light.ForceDirty();
            }
        }
        if (Input::KeyPressed(HELL_KEY_NUMPAD_2)) {
            AnimatedGameObject* ratKidAO = GetRadKidAO();
            ratKidAO->PlayAndLoopAnimation("Main", "RatKid_GlockIdle3", 1.0f);
            for (Light& light : Unloved::World::GetLights()) {
                light.ForceDirty();
            }
        }

        static bool ogPos = true;

        if (Input::MiddleMousePressed()) {
            ogPos = !ogPos;

            AnimatedGameObject* ratKidAO = GetRadKidAO();
            if (ogPos) {
                ratKidAO->SetPosition(glm::vec3(37.0f, 31.0f, 36.73f));
            }
            else {
                ratKidAO->SetPosition(glm::vec3(35.6f, 31.0f, 36.83f));
            }
            for (Light& light : Unloved::World::GetLights()) {
                light.ForceDirty();
            }
        }

        if (g_trapKingID == 666) {
            g_trapKingID = CreateAnimatedGameObject();
            AnimatedGameObject* trapKingAO = GetTrapKingAO();

            trapKingAO->SetSkinnedModel("TrapKing");
            trapKingAO->SetMeshMaterialByMeshName("Body", "TrapKingBodyHead");
            trapKingAO->SetMeshMaterialByMeshName("Body2", "TrapKingBodyTorso");
            trapKingAO->SetMeshMaterialByMeshName("Body3", "TrapKingBodyArms");
            trapKingAO->SetMeshMaterialByMeshName("Body4", "TrapKingBodyLegs");
            trapKingAO->SetMeshMaterialByMeshName("Body5", "TrapKingNails");
            trapKingAO->SetMeshMaterialByMeshName("Body6", "TrapKingEyeLashes");
            trapKingAO->SetBlendingModeByMeshName("Body6", BlendingMode::BLENDED);

            trapKingAO->SetMeshMaterialByMeshName("Tongue", "TrapKingTongue");

            trapKingAO->SetMeshMaterialByMeshName("Teeth", "TrapKingTeethUpper");
            trapKingAO->SetMeshMaterialByMeshName("Teeth2", "TrapKingTeethLower");

            trapKingAO->SetMeshMaterialByMeshName("DreadsTop", "TrapKingHairScalp");
            trapKingAO->SetMeshMaterialByMeshName("DreadsBottom", "TrapKingHairScalp");
            trapKingAO->SetMeshMaterialByMeshName("DreadsFront", "TrapKingHairScalp");
            trapKingAO->SetMeshMaterialByMeshName("DreadsShoulder", "TrapKingHairScalp");
            trapKingAO->SetMeshMaterialByMeshName("DreadsKnot", "TrapKingHairScalp");
            trapKingAO->SetMeshMaterialByMeshName("DreadsScalp", "TrapKingHairScalp");

            trapKingAO->SetBlendingModeByMeshName("DreadsScalp", BlendingMode::BLENDED);

            trapKingAO->SetBlendingModeByMeshName("EyeOcclusion", BlendingMode::DO_NOT_RENDER);
            trapKingAO->SetBlendingModeByMeshName("EyeOcclusion2", BlendingMode::DO_NOT_RENDER);

            trapKingAO->SetMeshMaterialByMeshName("Eye", "TrapKingEye");
            trapKingAO->SetBlendingModeByMeshName("Eye2", BlendingMode::DO_NOT_RENDER);
            trapKingAO->SetMeshMaterialByMeshName("Eye3", "TrapKingEye");
            trapKingAO->SetBlendingModeByMeshName("Eye4", BlendingMode::DO_NOT_RENDER);

            trapKingAO->SetBlendingModeByMeshName("Brow", BlendingMode::DO_NOT_RENDER);
            trapKingAO->SetMeshMaterialByMeshName("Brow2", "TrapKingBrow");
            trapKingAO->SetBlendingModeByMeshName("Brow2", BlendingMode::BLENDED);

            trapKingAO->SetBlendingModeByMeshName("TearLine", BlendingMode::DO_NOT_RENDER);
            trapKingAO->SetBlendingModeByMeshName("TearLine2", BlendingMode::DO_NOT_RENDER);

            trapKingAO->SetMeshMaterialByMeshName("Pants", "TrapKingPants");
            trapKingAO->SetMeshMaterialByMeshName("Boxers", "TrapKingBoxes");

            trapKingAO->SetPosition(glm::vec3(37.4f, 31.0f, 36.23f));

            trapKingAO->SetAnimationModeToBindPose();
        }

        glm::vec3 bunnyPos = glm::vec3(41.05f, 31.0f, 40.25f);
        Unloved::World::GetGameObjects()[0].SetPosition(bunnyPos);
        Unloved::World::GetGameObjects()[0].SetRotationY(-2.2f);
        Unloved::World::GetGameObjects()[0].m_meshNodes.EnablePointLightShadows();

      if (g_ratKidAO == 0) {
          InitRatKing("RatKing");

          AnimatedGameObject* ratKidAO = GetRadKidAO();
          ratKidAO->SetPosition(glm::vec3(37.0f, 31.0f, 36.73f));
      }

        auto& ragdolls = Hell::Physics::GetRagdolls();
        for (auto it = ragdolls.begin(); it != ragdolls.end(); ) {
            Ragdoll& ragdoll = it->second;

            //ragdoll.Update();

            if (Input::KeyPressed(HELL_KEY_Y)) {
                ragdoll.SetToInitialPose();
                ragdoll.DisableSimulation();

                for (Light& light : Unloved::World::GetLights()) {
                    AABB aabb = ragdoll.GetWorldSpaceAABB();
                    if (aabb.IntersectsSphere(light.GetPosition(), light.GetRadius())) {
                        light.ForceDirty();
                    }
                }
            }

            if (Input::KeyPressed(HELL_KEY_O)) {
                ragdoll.EnableSimulation();

                for (Light& light : Unloved::World::GetLights()) {
                    AABB aabb = ragdoll.GetWorldSpaceAABB();
                    if (aabb.IntersectsSphere(light.GetPosition(), light.GetRadius())) {
                        light.ForceDirty();
                    }
                }
            }
            ++it;
        }

        for (AnimatedGameObject& object : Unloved::World::GetAnimatedGameObjects()) object.Update(deltaTime);
        for (BulletCasing& object : Unloved::World::GetBulletCasings())             object.Update(deltaTime);
        for (ChristmasLightSet& object : Unloved::World::GetChristmasLightSets())   object.Update(deltaTime);
        for (ChristmasTree& object : Unloved::World::GetChristmasTrees())        object.Update(deltaTime);
        for (Dobermann& object : Unloved::World::GetDobermanns())                object.Update(deltaTime);
        for (Door& object : Unloved::World::GetDoors())                             object.Update(deltaTime);
        for (Fence& object : Unloved::World::GetFences())                           object.Update();
        for (Fireplace& object : Unloved::World::GetFireplaces())                   object.Update(deltaTime);
        for (GameObject& object : Unloved::World::GetGameObjects())              object.Update(deltaTime);
        //for (HousePlane& object : GetHousePlanes())               object.Update(deltaTime);
        for (GenericObject& object : Unloved::World::GetGenericObjects())           object.Update(deltaTime);
        for (Kangaroo& object : Unloved::World::GetKangaroos())                  object.Update(deltaTime);
        for (Ladder& object : Unloved::World::GetLadders())                         object.Update(deltaTime);
        for (Mermaid& object : Unloved::World::GetMermaids())                    object.Update(deltaTime);
        for (Piano& object : Unloved::World::GetPianos())                        object.Update(deltaTime);
        for (PickUp& object : Unloved::World::GetPickUps())                         object.Update(deltaTime);
        for (PictureFrame& object : Unloved::World::GetPictureFrames())             object.Update();
        for (PowerPoleSet& object : Unloved::World::GetPowerPoleSets())             object.Update();
        for (Road& object : GetRoads())                             object.Update();
        for (Shark& object : GetSharks())                           object.Update(deltaTime);
        for (Staircase& object : Unloved::World::GetStaircases())                   object.Update(deltaTime);
        //for (Tree& object : GetTrees())                             object.Update(deltaTime);
        for (TrimSet& object : Unloved::World::GetTrimSets())                       object.Update();
        for (Window& object : Unloved::World::GetWindows())                         object.Update(deltaTime);

        // These must run in this order otherwise various dirty flags are stale
        for (DDGIVolume& object : Unloved::World::GetDDGIVolumes())          object.Update();
        for (Light& object : Unloved::World::GetLights())                        object.Update(deltaTime);
        for (Decal& object : GetDecals())                           object.Update();

        // Update player weapon attachments. Must happen after AnimatedGameObject updates so that animated transforms are correct
        for (int i = 0; i < Unloved::Session::GetLocalPlayerCount(); i++) {
            Unloved::Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(i);
            if (!player) continue;

            player->UpdateWeaponAttachments();
			player->UpdateSpriteSheets(deltaTime);
        }

        if (Input::KeyPressed(HELL_KEY_BACKSPACE)) {
            for (BulletCasing& bulletCasing : Unloved::World::GetBulletCasings()) {
                bulletCasing.CleanUp();
            }

            GetDecals().clear();
            Unloved::BloodSystem::GetBloodScreenSpaceDecals().clear();
            Unloved::World::GetBulletCasings().clear();
        }

        P90MagManager::SubmitRenderItems();

        Unloved::BloodSystem::Update(deltaTime);
    }

    void RecreateAllDoorAndWindowCubeTransforms() {
        std::vector<Transform>& transforms = GetDoorAndWindowCubeTransforms();

        transforms.clear();
        transforms.reserve(Unloved::World::GetDoors().size() + Unloved::World::GetWindows().size());
        float padding = 0.02f;

        for (Door& door : Unloved::World::GetDoors()) {
            Transform& transform = transforms.emplace_back();
            transform.position = door.GetPosition();
            transform.position.y += DOOR_HEIGHT * 0.5f;
            transform.rotation = door.GetRotation();
            transform.scale = glm::vec3(0.2f, DOOR_HEIGHT + padding, DOOR_WIDTH + padding);
        }

        for (Window& window : Unloved::World::GetWindows()) {
            Transform& transform = transforms.emplace_back();
            transform.position = window.GetPosition();
            transform.position.y += 1.48f;
            transform.rotation = window.GetRotation();
            transform.scale = glm::vec3(0.2f, 1.185074f, 0.85f);
        }
    }
}
