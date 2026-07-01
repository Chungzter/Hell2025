#include "LegacyWorld.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Editor/Editor.h"
#include "Unloved/World/World.h"
#include "Hell/Logging.h"
#include "Renderer/RenderDataManager.h"
#include "Renderer/Renderer.h"
#include "Hell/Time.h"


namespace Unloved::LegacyWorld {

    void SubmitRenderItems() {

        for (int i = 0; i < Unloved::Session::GetLocalPlayerCount(); i++) {
            Unloved::Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(i);
            if (!player) continue;

            player->SubmitP90MagsRenderItems();
        }

        for (WorldPlane& housePlane : Unloved::World::GetWorldPlanes()) {
            housePlane.SubmitRenderItem();
        }

        // Main mesh buffer
        for (Door& object : Unloved::World::GetDoors())                   RenderDataManager::SubmitMeshNodes(object.GetMeshNodes());
        for (Fireplace& object : Unloved::World::GetFireplaces())         RenderDataManager::SubmitMeshNodes(object.GetMeshNodes());
        for (GameObject& object : Unloved::World::GetGameObjects())     RenderDataManager::SubmitMeshNodes(object.GetMeshNodes());
        for (GenericObject& object : Unloved::World::GetGenericObjects()) RenderDataManager::SubmitMeshNodes(object.GetMeshNodes());
        for (Mermaid& object : Unloved::World::GetMermaids())           RenderDataManager::SubmitMeshNodes(object.GetMeshNodes());
        for (Piano& object : Unloved::World::GetPianos())               RenderDataManager::SubmitMeshNodes(object.GetMeshNodes());
        for (PickUp& object : Unloved::World::GetPickUps())               RenderDataManager::SubmitMeshNodes(object.GetMeshNodes());
        for (PictureFrame& object : Unloved::World::GetPictureFrames())   RenderDataManager::SubmitMeshNodes(object.GetMeshNodes());
        for (Window& object : Unloved::World::GetWindows())               RenderDataManager::SubmitMeshNodes(object.GetMeshNodes());

        // Clean me up
        for (ChristmasTree& object : Unloved::World::GetChristmasTrees())        RenderDataManager::SubmitRenderItems(object.GetRenderItems());
        for (ChristmasLightSet& object : Unloved::World::GetChristmasLightSets())  RenderDataManager::SubmitRenderItems(object.GetRenderItems());
        for (Fence& object : Unloved::World::GetFences())                          RenderDataManager::SubmitRenderItems(object.GetRenderItems());
        for (Ladder& object : Unloved::World::GetLadders())                        RenderDataManager::SubmitRenderItems(object.GetRenderItems());
        for (Light& object : Unloved::World::GetLights())                         RenderDataManager::SubmitRenderItems(object.GetRenderItems());
        for (Staircase& object : Unloved::World::GetStaircases())                  RenderDataManager::SubmitRenderItems(object.GetRenderItems());
        for (TrimSet& object : Unloved::World::GetTrimSets())                      RenderDataManager::SubmitRenderItems(object.GetRenderItems());
        for (PowerPoleSet& object : Unloved::World::GetPowerPoleSets())            RenderDataManager::SubmitRenderItems(object.GetRenderItems());
        for (Wall& object : Unloved::World::GetWalls())                            RenderDataManager::SubmitRenderItems(object.GetWeatherBoardstopRenderItems());

        for (Wall& wall : Unloved::World::GetWalls()) {
            wall.SubmitRenderItems();
        }

        for (BulletCasing& bulletCasing : Unloved::World::GetBulletCasings()) {
            bulletCasing.SubmitRenderItem();
        }

        // Animated mesh nodes
        for (AnimatedGameObject& animatedGameObject : Unloved::World::GetAnimatedGameObjects()) {
            animatedGameObject.UpdateRenderItems();
            RenderDataManager::SubmitAnimatedMeshNodes(animatedGameObject.GetAnimatedMeshNodes());
        }

        // Update UI after all else
        for (int i = 0; i < Unloved::Session::GetLocalPlayerCount(); i++) {
            Unloved::Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(i);
            if (!player) continue;

            player->UpdateUI(Hell::Time::DeltaTime());
        }


        // This prints the name and type of a RenderItem list
        // 
        // if (Input::KeyPressed(HELL_KEY_E)) {
        //     std::cout << "\n";
        // 
        //     for (const RenderItem& renderItem : RenderDataManager::GetRenderItemsAlphaDiscard()) {
        //         uint64_t objectId = 0;
        //         Hell::Bit::UnpackUint64(renderItem.objectIdLowerBit, renderItem.objectIdUpperBit, objectId);
        // 
        //         std::cout << objectId << " " << Hell::Enum::ToString(Unloved::GetObjectIdType(objectId)) << " ";
        // 
        //         Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(renderItem.meshId);
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
