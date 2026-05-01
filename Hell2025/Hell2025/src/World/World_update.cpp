#include "World.h"
#include "Audio/Audio.h"
#include "Core/Game.h"
#include "Core/P90MagManager.h"
#include <Hell/Logging.h>
#include "Input/Input.h"
#include "Pathfinding/AStarMap.h"
#include "Renderer/RenderDataManager.h"
#include "Renderer/Renderer.h"
#include "Viewport/ViewportManager.h"

#include "Ragdoll/RagdollManager.h"
#include "Pathfinding/NavMesh.h"

#include "AssetManagement/AssetManager.h"

#include "Types/Misc/DoorChain.h"

#include <Hell/SlotMap.h>

namespace World {

    void LazyDebugSpawns();
    void CalculateGPULights();
    void CalculateDirtyAABBs();

    // REMOVE ME!
    uint64_t g_testAnimatedGameObject = 0;
    AnimatedGameObject* GetDobermannTest() {
        return GetAnimatedGameObjectByObjectId(g_testAnimatedGameObject);
    }

    uint64_t g_trapKingID = 0;
    uint64_t g_ratKidAO = 0;
    AnimatedGameObject* GetTrapKingAO() {
        return GetAnimatedGameObjectByObjectId(g_trapKingID);
    }
    AnimatedGameObject* GetRadKidAO() {
        return GetAnimatedGameObjectByObjectId(g_ratKidAO);
    }


	void InitRatKing(const std::string& modelName) {
        RemoveObject(g_ratKidAO);

		g_ratKidAO = CreateAnimatedGameObject();
		AnimatedGameObject* ratKidAO = GetRadKidAO();

		ratKidAO->SetSkinnedModel(modelName);
		ratKidAO->SetAnimationModeToBindPose();



		ratKidAO->SetMeshMaterialByMeshName("Slim_Jeans", "Jeans");

		ratKidAO->SetMeshMaterialByMeshName("CC_Base_Body", "RatKingHead");
		ratKidAO->SetMeshMaterialByMeshName("CC_Base_Body2", "RatKingTorso");
		ratKidAO->SetMeshMaterialByMeshName("CC_Base_Body3", "RatKingArms");
		ratKidAO->SetMeshMaterialByMeshName("CC_Base_Body4", "RatKingLegs");
		ratKidAO->SetMeshMaterialByMeshName("CC_Base_Body5", "RatKingNails");
		ratKidAO->SetMeshMaterialByMeshName("CC_Base_Body6", "RatKingLashes2");
		ratKidAO->SetBlendingModeByMeshName("CC_Base_Body6", BlendingMode::BLENDED);

		ratKidAO->SetMeshMaterialByMeshName("CC_Base_Eye", "RatKingEye");
		ratKidAO->SetBlendingModeByMeshName("CC_Base_Eye2", BlendingMode::DO_NOT_RENDER);
		ratKidAO->SetMeshMaterialByMeshName("CC_Base_Eye3", "RatKingEye");
		ratKidAO->SetBlendingModeByMeshName("CC_Base_Eye4", BlendingMode::DO_NOT_RENDER);

		ratKidAO->SetBlendingModeByMeshName("CC_Base_TearLine", BlendingMode::DO_NOT_RENDER);
		ratKidAO->SetBlendingModeByMeshName("CC_Base_TearLine2", BlendingMode::DO_NOT_RENDER);

		ratKidAO->SetMeshMaterialByMeshName("CC_Base_Tongue", "TrapKingTongue");

		ratKidAO->SetMeshMaterialByMeshName("CC_Base_Teeth", "TrapKingTeethUpper");
		ratKidAO->SetMeshMaterialByMeshName("CC_Base_Teeth2", "TrapKingTeethLower");

		ratKidAO->SetBlendingModeByMeshName("Brows_Bushy", BlendingMode::DO_NOT_RENDER);
		ratKidAO->SetMeshMaterialByMeshName("Brows_Bushy2", "RatKingBrows");
		ratKidAO->SetBlendingModeByMeshName("Brows_Bushy2", BlendingMode::BLENDED);
		ratKidAO->SetBlendingModeByMeshName("Brows_Bushy3", BlendingMode::DO_NOT_RENDER);

		// NEW
		ratKidAO->SetMeshMaterialByMeshName("Chunky_Highlights_Bangs", "RatKingHair2");
		ratKidAO->SetBlendingModeByMeshName("Chunky_Highlights_Bangs", BlendingMode::HAIR);
		ratKidAO->SetMeshMaterialByMeshName("Chunky_Highlights_Bangs2", "RatKingHair2");
		ratKidAO->SetBlendingModeByMeshName("Chunky_Highlights_Bangs2", BlendingMode::HAIR);
		ratKidAO->SetMeshMaterialByMeshName("Chunky_Highlights_Bangs.001", "RatKingHair2");
		ratKidAO->SetBlendingModeByMeshName("Chunky_Highlights_Bangs.001", BlendingMode::HAIR);
		ratKidAO->SetMeshMaterialByMeshName("Chunky_Highlights_Bangs.0012", "RatKingHair2");
        ratKidAO->SetBlendingModeByMeshName("Chunky_Highlights_Bangs.0012", BlendingMode::HAIR);
        //

		ratKidAO->SetMeshMaterialByMeshName("Side_swept_L", "RatKingHair");
		ratKidAO->SetBlendingModeByMeshName("Side_swept_L", BlendingMode::HAIR);


		ratKidAO->SetMeshMaterialByMeshName("Long_Hair_R", "RatKingHair");
		ratKidAO->SetBlendingModeByMeshName("Long_Hair_R", BlendingMode::HAIR);
		
		ratKidAO->SetMeshMaterialByMeshName("Long_Hair_R2", "RatKingHair");
		ratKidAO->SetBlendingModeByMeshName("Long_Hair_R2", BlendingMode::HAIR);

		ratKidAO->SetMeshMaterialByMeshName("Side_swept_Long_Hair_L", "RatKingScalp");
		ratKidAO->SetBlendingModeByMeshName("Side_swept_Long_Hair_L", BlendingMode::BLENDED);
		ratKidAO->SetMeshMaterialByMeshName("Side_swept_Long_Hair_L2", "RatKingHair");
		ratKidAO->SetBlendingModeByMeshName("Side_swept_Long_Hair_L2", BlendingMode::HAIR);

		ratKidAO->SetMeshMaterialByMeshName("Side_swept_Long_Hair_R", "RatKingScalp");
		ratKidAO->SetBlendingModeByMeshName("Side_swept_Long_Hair_R", BlendingMode::BLENDED);
		ratKidAO->SetMeshMaterialByMeshName("Side_swept_Long_Hair_R2", "RatKingHair");
		ratKidAO->SetBlendingModeByMeshName("Side_swept_Long_Hair_R2", BlendingMode::HAIR);

		ratKidAO->SetMeshMaterialByMeshName("Long_bangs", "RatKingHair2");
		ratKidAO->SetBlendingModeByMeshName("Long_bangs", BlendingMode::HAIR);


		ratKidAO->SetMeshMaterialByMeshName("Scalp_Male", "RatKingScalp");
		ratKidAO->SetBlendingModeByMeshName("Scalp_Male", BlendingMode::BLENDED);

		ratKidAO->SetMeshMaterialByMeshName("Side_swept_L2", "RatKingScalp");
		ratKidAO->SetBlendingModeByMeshName("Side_swept_L2", BlendingMode::BLENDED);


		ratKidAO->SetBlendingModeByMeshName("CC_Base_EyeOcclusion", BlendingMode::DO_NOT_RENDER);
		ratKidAO->SetBlendingModeByMeshName("CC_Base_EyeOcclusion2", BlendingMode::DO_NOT_RENDER);

		ratKidAO->SetBlendingModeByMeshName("Lash_Low_Short_Sparse", BlendingMode::BLENDED);
		ratKidAO->SetBlendingModeByMeshName("Lash_Low_Sparse", BlendingMode::BLENDED);
		ratKidAO->SetBlendingModeByMeshName("Lash_Up_Downward", BlendingMode::BLENDED);
		ratKidAO->SetBlendingModeByMeshName("Lash_Up_Short_Sparse", BlendingMode::BLENDED);

		ratKidAO->SetMeshMaterialByMeshName("Lash_Low_Short_Sparse", "RatKingLashes");
		ratKidAO->SetMeshMaterialByMeshName("Lash_Low_Sparse", "RatKingLashes");
		ratKidAO->SetMeshMaterialByMeshName("Lash_Up_Downward", "RatKingLashes");
		ratKidAO->SetMeshMaterialByMeshName("Lash_Up_Short_Sparse", "RatKingLashes");

















		ratKidAO->SetMeshMaterialByMeshName("Jeans", "Jeans");

		ratKidAO->SetMeshMaterialByMeshName("Body", "RatKingHead");
		ratKidAO->SetMeshMaterialByMeshName("Body2", "RatKingTorso");
		ratKidAO->SetMeshMaterialByMeshName("Body3", "RatKingArms");
		ratKidAO->SetMeshMaterialByMeshName("Body4", "RatKingLegs");
		ratKidAO->SetMeshMaterialByMeshName("Body5", "RatKingNails");
		ratKidAO->SetMeshMaterialByMeshName("Body6", "RatKingLashes2");
		ratKidAO->SetBlendingModeByMeshName("Body6", BlendingMode::BLENDED);

		ratKidAO->SetMeshMaterialByMeshName("Eye", "RatKingEye");
		ratKidAO->SetBlendingModeByMeshName("Eye2", BlendingMode::DO_NOT_RENDER);
		ratKidAO->SetMeshMaterialByMeshName("Eye3", "RatKingEye");
		ratKidAO->SetBlendingModeByMeshName("Eye4", BlendingMode::DO_NOT_RENDER);

		ratKidAO->SetBlendingModeByMeshName("TearLine", BlendingMode::DO_NOT_RENDER);
		ratKidAO->SetBlendingModeByMeshName("TearLine2", BlendingMode::DO_NOT_RENDER);

		ratKidAO->SetMeshMaterialByMeshName("Tongue", "TrapKingTongue");

		ratKidAO->SetMeshMaterialByMeshName("Teeth", "TrapKingTeethUpper");
		ratKidAO->SetMeshMaterialByMeshName("Teeth2", "TrapKingTeethLower");

		ratKidAO->SetBlendingModeByMeshName("Brows", BlendingMode::DO_NOT_RENDER);
		ratKidAO->SetMeshMaterialByMeshName("Brows2", "RatKingBrows");
		ratKidAO->SetBlendingModeByMeshName("Brows2", BlendingMode::BLENDED);
		ratKidAO->SetBlendingModeByMeshName("Brows3", BlendingMode::DO_NOT_RENDER);

		ratKidAO->SetMeshMaterialByMeshName("HairL", "RatKingHair");
		ratKidAO->SetMeshMaterialByMeshName("HairR", "RatKingHair");
		ratKidAO->SetMeshMaterialByMeshName("HairLong", "RatKingHair");
		ratKidAO->SetBlendingModeByMeshName("HairL", BlendingMode::HAIR);
		ratKidAO->SetBlendingModeByMeshName("HairR", BlendingMode::HAIR);
		ratKidAO->SetBlendingModeByMeshName("HairLong", BlendingMode::HAIR);

		ratKidAO->SetMeshMaterialByMeshName("Scalp", "RatKingScalp");
		ratKidAO->SetMeshMaterialByMeshName("Scalp2", "RatKingScalp");
		ratKidAO->SetMeshMaterialByMeshName("Scalp3", "RatKingScalp");
		ratKidAO->SetBlendingModeByMeshName("Scalp", BlendingMode::DO_NOT_RENDER);
		ratKidAO->SetBlendingModeByMeshName("Scalp2", BlendingMode::DO_NOT_RENDER);
		ratKidAO->SetBlendingModeByMeshName("Scalp3", BlendingMode::DO_NOT_RENDER);

		ratKidAO->SetMeshMaterialByMeshName("Scalp4", "RatKingScalp2");
		ratKidAO->SetBlendingModeByMeshName("Scalp4", BlendingMode::BLENDED);

		ratKidAO->SetBlendingModeByMeshName("EyeOcclusion", BlendingMode::DO_NOT_RENDER);
		ratKidAO->SetBlendingModeByMeshName("EyeOcclusion2", BlendingMode::DO_NOT_RENDER);

		ratKidAO->SetBlendingModeByMeshName("Lashes", BlendingMode::BLENDED);
		ratKidAO->SetBlendingModeByMeshName("Lashes2", BlendingMode::BLENDED);
		ratKidAO->SetBlendingModeByMeshName("Lashes3", BlendingMode::BLENDED);
		ratKidAO->SetBlendingModeByMeshName("Lashes4", BlendingMode::BLENDED);

		ratKidAO->SetMeshMaterialByMeshName("Lashes", "RatKingLashes");
		ratKidAO->SetMeshMaterialByMeshName("Lashes2", "RatKingLashes");
		ratKidAO->SetMeshMaterialByMeshName("Lashes3", "RatKingLashes");
		ratKidAO->SetMeshMaterialByMeshName("Lashes4", "RatKingLashes");

		ratKidAO->SetPosition(glm::vec3(36.8f, 31.0f, 36.23f));
		//ratKidAO->PlayAndLoopAnimation("Main", "RatKid_PistolWalk2", 1.0f);
		ratKidAO->PrintMeshNames();
    }
    
    static float DegToRad(float degrees) { return degrees * (HELL_PI / 180.0f); }

    void Update(float deltaTime) {

        // FAILED DOOR CHAIN LINK SHIT
        if (false) {
            static Hell::SlotMap<DoorChain> doorchains;
            static bool runOnce = true;

            if (runOnce) {
                runOnce = false;

                SpawnOffset spawnOffset;

                DoorChainCreateInfo createInfo;

                // First chain
                createInfo.position = glm::vec3(36, 32.6, 36);
                const uint64_t id = UniqueID::GetNextObjectId(ObjectType::NO_TYPE);
                doorchains.emplace_with_id(id, id, createInfo, spawnOffset);

                // Second chain
                createInfo.position = glm::vec3(37, 32.6, 37);
                createInfo.rotation.y = HELL_PI * 0.5f;
                const uint64_t id2 = UniqueID::GetNextObjectId(ObjectType::NO_TYPE);
                doorchains.emplace_with_id(id2, id2, createInfo, spawnOffset);
            }

            for (DoorChain& doorChain : doorchains) {
                doorChain.Update(deltaTime);
                doorChain.SubmitRenderItems();
            }
        }

        NavMeshManager::Update();

        //if (Input::KeyPressed(HELL_KEY_LEFT)) {
        //    static MermaidCreateInfo createInfo = GetMermaids()[0].GetCreateInfo();
        //    createInfo.rotation.y += 0.05f;
        //    GetMermaids()[0].Init(createInfo, SpawnOffset());
        //}
        //
        //if (Input::KeyPressed(HELL_KEY_NUMPAD_3)) {
        //
        //    GetGameObjects()[0].SetPosition(Game::GetLocalPlayerByIndex(0)->GetFootPosition());
        //    for (Light& light : GetLights()) {
        //        light.ForceDirty();
        //    }
        //}
        //
        //if (Input::KeyPressed(HELL_KEY_J)) {
        //    PrintObjectCounts();
        //}

        //glm::vec3 rayOrigin = Game::GetLocalPlayerByIndex(0)->GetCameraPosition();
        //glm::vec3 rayDir = Game::GetLocalPlayerByIndex(0)->GetCameraForward();
        //glm::vec3 position = glm::vec3(1.0f);
        //float radius = 0.5f;
        //
        //bool rayHit = Util::RayIntersectsSphere(rayOrigin, rayDir, position, radius);
        //glm::vec4 color = rayHit ? GREEN : YELLOW;
        //Renderer::DrawSphere(position, radius, color);
        //
        //if (rayHit) {
        //    std::cout << "ray origin:      " << rayOrigin << "\n";
        //    std::cout << "ray dir:         " << rayDir << "\n";
        //    std::cout << "sphere position: " << position << "\n";
        //    std::cout << "sphere radius:   " << radius << "\n\n";
        //}

        // Display closest AABB to mesh nodes to player 0
        //for (GenericObject& genericObject : GetGenericObjects()) {
        //    for (const MeshNode& meshNode : genericObject.GetMeshNodes().GetNodes()) {
        //        const AABB& aabb = meshNode.worldspaceAabb;
        //        glm::vec3 closestPoint = aabb.NearestPointTo(Game::GetLocalPlayerByIndex(0)->GetCameraPosition());
        //        Renderer::DrawAABB(aabb, PINK);
        //        Renderer::DrawPoint(closestPoint, YELLOW);
        //    }
        //}

        if (g_trapKingID == 0) {
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
            trapKingAO->PrintMeshNames();
            trapKingAO->SetAnimationModeToBindPose();
            //trapKingAO->PlayAndLoopAnimation("Main", "RatKid_PistolWalk3", 1.0f);

        }

      if (g_ratKidAO == 0) {
		  InitRatKing("RatKing");
	  }
	  if (Input::KeyPressed(HELL_KEY_NUMPAD_7)) {
		  InitRatKing("RatKingSam");
		  Audio::PlayAudio(AUDIO_SELECT, 1.0f);
	  }
	  if (Input::KeyPressed(HELL_KEY_NUMPAD_8)) {
		  InitRatKing("RatKingNew");
		  Audio::PlayAudio(AUDIO_SELECT, 1.0f);
	  }
	  if (Input::KeyPressed(HELL_KEY_NUMPAD_9)) {
		  AnimatedGameObject* ratKidAO = GetRadKidAO();
		  ratKidAO->SetAnimationModeToBindPose();
		  Audio::PlayAudio(AUDIO_SELECT, 1.0f);
	  }
	  if (Input::KeyPressed(HELL_KEY_NUMPAD_0)) {
		  AnimatedGameObject* ratKidAO = GetRadKidAO();
		  ratKidAO->PlayAndLoopAnimation("Main", "RatKingSamTest", 1.0f);
		  Audio::PlayAudio(AUDIO_SELECT, 1.0f);
	  }
     
       
        auto& ragdolls = RagdollManager::GetRagdolls();
        for (auto it = ragdolls.begin(); it != ragdolls.end(); ) {
            RagdollV2& ragdoll = it->second;

            //ragdoll.Update();

            if (Input::KeyPressed(HELL_KEY_Y)) {
                ragdoll.SetToInitialPose();
                ragdoll.DisableSimulation();

                for (Light& light : GetLights()) {
                    AABB aabb = ragdoll.GetWorldSpaceAABB();
                    if (aabb.IntersectsSphere(light.GetPosition(), light.GetRadius())) {
                        light.ForceDirty();
                    }
                }
            }

            if (Input::KeyPressed(HELL_KEY_O)) {
                ragdoll.EnableSimulation();

                for (Light& light : GetLights()) {
                    AABB aabb = ragdoll.GetWorldSpaceAABB();
                    if (aabb.IntersectsSphere(light.GetPosition(), light.GetRadius())) {
                        light.ForceDirty();
                    }
                }
            }
            ++it;
        }


        //auto pos = glm::vec3(31.0f, 30.4f, 38.25f);
        //Renderer::DrawPoint(pos, RED);

        /*
        {
        // Visualize dot product arc
            glm::vec3 forward = Game::GetLocalPlayerByIndex(0)->GetCameraForward();
            forward.y = 0.0f;
            forward = glm::normalize(forward);
            glm::vec3 origin = Game::GetLocalPlayerByIndex(0)->GetFootPosition();
            float dotThreshold = 0.7f;
            float angle = acos(dotThreshold);
            glm::mat4 rotRight = glm::rotate(glm::mat4(1.0f), -angle, glm::vec3(0, 1, 0));
            glm::mat4 rotLeft = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0, 1, 0));
            glm::vec3 rightEdge = glm::vec3(rotRight * glm::vec4(forward, 0.0f));
            glm::vec3 leftEdge = glm::vec3(rotLeft * glm::vec4(forward, 0.0f));
            float length = 2.0f;
            glm::vec4 color = RED;

            AnimatedGameObject* roo = GetAnimatedGameObjectByObjectId(g_rooAnimatedGameObject);
            glm::vec3 target = roo->_transform.position;
            glm::vec3 toTarget = glm::normalize(target - origin);
            float dotValue = glm::dot(forward, toTarget);
            if (dotValue >= dotThreshold) {
                color = GREEN;
            }
            Renderer::DrawLine(origin, origin + forward * length, color);     // center
            Renderer::DrawLine(origin, origin + leftEdge * length, color);    // left limit
            Renderer::DrawLine(origin, origin + rightEdge * length, color);   // right limit
        }*/

        ProcessBullets();
        LazyDebugSpawns();

        for (AnimatedGameObject& object : GetAnimatedGameObjects()) object.Update(deltaTime);
        for (BulletCasing& object : GetBulletCasings())             object.Update(deltaTime);
        for (ChristmasLightSet& object : GetChristmasLightSets())        object.Update(deltaTime);
        for (ChristmasPresent& object : GetChristmasPresents())     object.Update(deltaTime);
        for (ChristmasTree& object : GetChristmasTrees())           object.Update(deltaTime);
        for (Decal& object : GetDecals())                           object.Update();
        for (Dobermann& object : GetDobermanns())                   object.Update(deltaTime);
        for (Door& object : GetDoors())                             object.Update(deltaTime);
        for (Fence& object : GetFences())                           object.Update();
        for (Fireplace& object : GetFireplaces())                   object.Update(deltaTime);
        for (GameObject& object : GetGameObjects())                 object.Update(deltaTime);
        //for (HousePlane& object : GetHousePlanes())               object.Update(deltaTime);
        for (GenericObject& object : GetGenericObjects())           object.Update(deltaTime);
        for (Kangaroo& object : GetKangaroos())                     object.Update(deltaTime);
        for (Ladder& object : GetLadders())                         object.Update(deltaTime);
        for (Mermaid& object : GetMermaids())                       object.Update(deltaTime);
        for (Piano& object : GetPianos())                           object.Update(deltaTime);
        for (PickUp& object : GetPickUps())                         object.Update(deltaTime);
        for (PowerPoleSet& object : GetPowerPoleSets())             object.Update();
        for (Road& object : GetRoads())                             object.Update();
        for (Shark& object : GetSharks())                           object.Update(deltaTime);
        for (Staircase& object : GetStaircases())                   object.Update(deltaTime);
        for (Tree& object : GetTrees())                             object.Update(deltaTime);
        for (TrimSet& object : GetTrimSets())                       object.Update();
        for (Window& object : GetWindows())                         object.Update(deltaTime);

        // These must run in this order otherwise various dirty flags are stale
        for (DDGIVolume& object : GetDDGIVolumes())                 object.Update();
        for (Light& object : GetLights())                           object.Update(deltaTime);

        // Update player weapon attachments. Must happen after AnimatedGameObject updates so that animated transforms are correct
        for (int i = 0; i < Game::GetLocalPlayerCount(); i++) {
            Player* player = Game::GetLocalPlayerByIndex(i);
            if (!player) continue;

            player->UpdateWeaponAttachments();
			player->UpdateSpriteSheets(deltaTime);
        }

        //lights[2].SetColor(DEFAULT_LIGHT_COLOR);
        //GetLights()[2].SetStrength(2.0f);
        //GetLights()[2].SetRadius(3.5f);

        if (Input::KeyPressed(HELL_KEY_BACKSPACE)) {
            for (BulletCasing& bulletCasing : GetBulletCasings()) {
                bulletCasing.CleanUp();
            }

            GetDecals().clear();
            GetScreenSpaceBloodDecals().clear();
            GetBulletCasings().clear();
        }

        CalculateGPULights();
        CalculateDirtyAABBs();

        P90MagManager::SubmitRenderItems();

        // Volumetric blood
        std::vector<VolumetricBloodSplatter>& volumetricBloodSplatters = GetVolumetricBloodSplatters();
        for (int i = 0; i < volumetricBloodSplatters.size(); i++) {
            VolumetricBloodSplatter& volumetricBloodSplatter = volumetricBloodSplatters[i];

            if (volumetricBloodSplatter.GetLifeTime() < 0.9f) {
                volumetricBloodSplatter.Update(deltaTime);
            }
            else {
                volumetricBloodSplatters.erase(volumetricBloodSplatters.begin() + i);
                i--;
            }
        }
    }

    void LazyDebugSpawns() {
        // AKs
        //if (Input::KeyPressed(HELL_KEY_BACKSPACE)) {
        //    PickUpCreateInfo createInfo;
        //    createInfo.position = Game::GetLocalPlayerByIndex(0)->GetCameraPosition();
        //    createInfo.position += Game::GetLocalPlayerByIndex(0)->GetCameraForward();
        //    createInfo.rotation.x = Util::RandomFloat(-HELL_PI, HELL_PI);
        //    createInfo.rotation.y = Util::RandomFloat(-HELL_PI, HELL_PI);
        //    createInfo.rotation.z = Util::RandomFloat(-HELL_PI, HELL_PI);
        //    createInfo.pickUpType = Util::PickUpTypeToString(PickUpType::AKS74U);
        //    AddPickUp(createInfo);
        //}

        // Remingtons
        //if (Input::KeyPressed(HELL_KEY_INSERT)) {
        //    PickUpCreateInfo createInfo;
        //    createInfo.position = Game::GetLocalPlayerByIndex(0)->GetCameraPosition();
        //    createInfo.position += Game::GetLocalPlayerByIndex(0)->GetCameraForward();
        //    createInfo.rotation.x = Util::RandomFloat(-HELL_PI, HELL_PI);
        //    createInfo.rotation.y = Util::RandomFloat(-HELL_PI, HELL_PI);
        //    createInfo.rotation.z = Util::RandomFloat(-HELL_PI, HELL_PI);
        //    createInfo.pickUpType = Util::PickUpTypeToString(PickUpType::REMINGTON_870);
        //    AddPickUp(createInfo);
        //}
    }

    void UpdateDoorAndWindowCubeTransforms() {
        std::vector<Transform>& transforms = GetDoorAndWindowCubeTransforms();

        transforms.clear();
        transforms.reserve(World::GetDoors().size() + GetWindows().size());

        for (Door& door : World::GetDoors()) {
            Transform& transform = transforms.emplace_back();
            transform.position = door.GetPosition();
            transform.position.y += DOOR_HEIGHT / 2;
            transform.rotation.y = door.GetRotation().y;
            transform.scale.x = 0.2f;
            transform.scale.y = DOOR_HEIGHT * 1.0f;
            transform.scale.z = 1.02f;
        }

        for (Window& window : GetWindows()) {
            float windowMidPointFromGround = 1.4f;

            Transform& transform = transforms.emplace_back();
            transform.position = window.GetPosition();
            transform.position.y += windowMidPointFromGround;
            transform.rotation.y = window.GetRotation().y;
            transform.scale.x = 0.2f;
            transform.scale.y = 1.2f;
            transform.scale.z = 0.846f;
        }
    }

    void CalculateGPULights() {
        for (int i = 0; i < GetLights().size(); i++) {
            RenderDataManager::SubmitGPULightHighRes(i);
        }
    }

    void CalculateDirtyAABBs() {
        std::vector<GPUAABB>& aabbs = GetDirtyDoorAABBS();
        aabbs.clear();

        for (Door& door : GetDoors()) {
            if (door.IsDirty()) {
                GPUAABB aabb;
                aabb.boundsMin = glm::vec4(door.GetPhsyicsAABB().GetBoundsMin(), 0.0f);
                aabb.boundsMax = glm::vec4(door.GetPhsyicsAABB().GetBoundsMax(), 0.0f);
                aabbs.push_back(aabb);

                //Renderer::DrawAABB(door.GetPhsyicsAABB(), YELLOW);
            }
        }
    }
}