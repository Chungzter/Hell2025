#include "World.h"
#include "AssetManagement/AssetManager.h"
#include "Core/Game.h"
#include "Editor/Editor.h"
#include "Hell/Logging.h"
#include "Input/Input.h"
#include "Renderer/RenderDataManager.h"
#include "Renderer/Renderer.h"

namespace World {

    void SubmitRenderItems() {

        for (int i = 0; i < Game::GetLocalPlayerCount(); i++) {
            Player* player = Game::GetLocalPlayerByIndex(i);
            if (!player) continue;

            player->SubmitP90MagsRenderItems();
        }

        for (HousePlane& housePlane : GetHousePlanes()) {
            housePlane.SubmitRenderItem();
        }

        // Main mesh buffer
        for (Door& object : GetDoors())                   RenderDataManager::SubmitMeshNodes(object.GetMeshNodes());
        for (Fireplace& object : GetFireplaces())         RenderDataManager::SubmitMeshNodes(object.GetMeshNodes());
        for (GameObject& object : GetGameObjects())       RenderDataManager::SubmitMeshNodes(object.GetMeshNodes());
        for (GenericObject& object : GetGenericObjects()) RenderDataManager::SubmitMeshNodes(object.GetMeshNodes());
        for (Mermaid& object : GetMermaids())             RenderDataManager::SubmitMeshNodes(object.GetMeshNodes());
        for (Piano& object : GetPianos())                 RenderDataManager::SubmitMeshNodes(object.GetMeshNodes());
        for (PickUp& object : GetPickUps())               RenderDataManager::SubmitMeshNodes(object.GetMeshNodes());
        for (PictureFrame& object : GetPictureFrames())   RenderDataManager::SubmitMeshNodes(object.GetMeshNodes());
        for (Window& object : GetWindows())               RenderDataManager::SubmitMeshNodes(object.GetMeshNodes());

        // Clean me up
        for (ChristmasTree& object : GetChristmasTrees())          RenderDataManager::SubmitRenderItems(object.GetRenderItems());
        for (ChristmasLightSet& object : GetChristmasLightSets())  RenderDataManager::SubmitRenderItems(object.GetRenderItems());
        for (Fence& object : GetFences())                          RenderDataManager::SubmitRenderItems(object.GetRenderItems());
        for (Ladder& object : GetLadders())                        RenderDataManager::SubmitRenderItems(object.GetRenderItems());
        for (Light& object : GetLights())                          RenderDataManager::SubmitRenderItems(object.GetRenderItems());
        for (Staircase& object : GetStaircases())                  RenderDataManager::SubmitRenderItems(object.GetRenderItems());
        for (TrimSet& object : GetTrimSets())                      RenderDataManager::SubmitRenderItems(object.GetRenderItems());
        for (PowerPoleSet& object : GetPowerPoleSets())            RenderDataManager::SubmitRenderItems(object.GetRenderItems());
        for (Wall& object : GetWalls())                            RenderDataManager::SubmitRenderItems(object.GetWeatherBoardstopRenderItems());

        for (Wall& wall : GetWalls()) {
            wall.SubmitRenderItems();
        }

        for (BulletCasing& bulletCasing : GetBulletCasings()) {
            bulletCasing.SubmitRenderItem();
        }

        // Animated mesh nodes
        for (AnimatedGameObject& animatedGameObject : GetAnimatedGameObjects()) {
            animatedGameObject.UpdateRenderItems();
            RenderDataManager::SubmitAnimatedMeshNodes(animatedGameObject.GetAnimatedMeshNodes());
        }

        // Hack to render door and window cube transforms
        if (true && false) {
            int meshIndex = AssetManager::GetMeshIndexByModelNameMeshName("Primitives", "Cube");
            std::vector<Transform>& transforms = GetDoorAndWindowCubeTransforms();
            for (Transform& transform : transforms) {
                Material* material = AssetManager::GetDefaultMaterial();
                RenderItem renderItem;
                renderItem.meshIndex = meshIndex;
                renderItem.modelMatrix = transform.to_mat4();
                renderItem.baseColorTextureIndex = material->m_basecolor;
                renderItem.normalMapTextureIndex = material->m_normal;
                renderItem.rmaTextureIndex = material->m_rma;
                Util::UpdateRenderItemAABB(renderItem);
                RenderDataManager::SubmitRenderItem(renderItem);
            }
        }

        // Update UI after all else
        for (int i = 0; i < Game::GetLocalPlayerCount(); i++) {
            Player* player = Game::GetLocalPlayerByIndex(i);
            if (!player) continue;

            player->UpdateUI(Game::GetDeltaTime());
        }


        // This prints the name and type of a RenderItem list
        // 
        // if (Input::KeyPressed(HELL_KEY_E)) {
        //     std::cout << "\n";
        // 
        //     for (const RenderItem& renderItem : RenderDataManager::GetRenderItemsAlphaDiscard()) {
        //         uint64_t objectId = 0;
        //         Util::UnpackUint64(renderItem.objectIdLowerBit, renderItem.objectIdUpperBit, objectId);
        // 
        //         std::cout << objectId << " " << Util::EnumToString(UniqueID::GetType(objectId)) << " ";
        // 
        //         Mesh* mesh = AssetManager::GetMeshByIndex(renderItem.meshIndex);
        //         if (!mesh) {
        //             std::cout << "\n";
        //             continue;
        //         }
        // 
        //         std::cout << mesh->GetName() << "\n";
        //     }
        // }
    }
}