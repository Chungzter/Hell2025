#include "World.h"

#include "Hell/Logging.h"
#include "Hell/Profiling/CPUProfiler.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/Time.h"

#include "Unloved/Characters/Enemies/Dobermann/Dobermann.h"
#include "Unloved/Characters/Enemies/Kangaroo/Kangaroo.h"
#include "Unloved/Characters/Enemies/Shark/Shark.h"
#include "Unloved/Characters/Mermaids/Mermaid/Mermaid.h"
#include "Unloved/Editor/Editor.h"
#include "Unloved/Objects/Effects/Decal.h"
#include "Unloved/Objects/Exterior/Fence.h"
#include "Unloved/Objects/Exterior/Jetty.h"
#include "Unloved/Objects/Exterior/PowerPoleSet.h"
#include "Unloved/Objects/Exterior/Road.h"
#include "Unloved/Objects/Exterior/Wire.h"
#include "Unloved/Objects/House/Door.h"
#include "Unloved/Objects/House/Fireplace.h"
#include "Unloved/Objects/House/PlanarQuadObject.h"
#include "Unloved/Objects/House/PointPairObject.h"
#include "Unloved/Objects/House/TrimSet.h"
#include "Unloved/Objects/House/Wall.h"
#include "Unloved/Objects/House/Window.h"
#include "Unloved/Objects/House/WorldPlane.h"
#include "Unloved/Objects/Interior/Piano.h"
#include "Unloved/Objects/Interior/PictureFrame.h"
#include "Unloved/Objects/Lighting/Light.h"
#include "Unloved/Objects/Props/BulletCasing.h"
#include "Unloved/Objects/Props/Christmas/ChristmasLights.h"
#include "Unloved/Objects/Props/Christmas/ChristmasTree.h"
#include "Unloved/Objects/Props/GameObject.h"
#include "Unloved/Objects/Props/GenericObject.h"
#include "Unloved/Objects/Props/PickUp.h"
#include "Unloved/Objects/Renderables/AnimatedGameObject.h"
#include "Unloved/Objects/Spawns/HouseLocation.h"
#include "Unloved/Objects/Spawns/SpawnPoint.h"
#include "Unloved/Objects/Traversal/Ladder.h"
#include "Unloved/Objects/Traversal/Staircase.h"
#include "Unloved/EditorSession/EditorSession.h"
#include "Unloved/Session/Session.h"

#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Render/Renderer.h"

namespace Unloved::World {

// TODO: This whole file is pretty fucked. Clean it up.

void SubmitRenderItems() {
    ProfilerCPUZoneFunction();

    for (int i = 0; i < Unloved::Session::GetLocalPlayerCount(); i++) {
        Unloved::Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(i);
        if (!player) continue;

        player->SubmitP90MagsRenderItems();
    }

    // Main mesh buffer
    for (Door& object : Unloved::World::GetDoors())                   RenderDataManager::SubmitMeshNodes(object.GetMeshNodes());
    for (Fireplace& object : Unloved::World::GetFireplaces())         RenderDataManager::SubmitMeshNodes(object.GetMeshNodes());
    for (GameObject& object : Unloved::World::GetGameObjects())       RenderDataManager::SubmitMeshNodes(object.GetMeshNodes());
    for (GenericObject& object : Unloved::World::GetGenericObjects()) RenderDataManager::SubmitMeshNodes(object.GetMeshNodes());
    for (Mermaid& object : Unloved::World::GetMermaids())             RenderDataManager::SubmitMeshNodes(object.GetMeshNodes());
    for (Piano& object : Unloved::World::GetPianos())                 RenderDataManager::SubmitMeshNodes(object.GetMeshNodes());
    for (PictureFrame& object : Unloved::World::GetPictureFrames())   RenderDataManager::SubmitMeshNodes(object.GetMeshNodes());
    for (Window& object : Unloved::World::GetWindows())               RenderDataManager::SubmitMeshNodes(object.GetMeshNodes());

    // PickUps only if they are not despawned
    for (PickUp& pickUp : Unloved::World::GetPickUps()) {
        if (pickUp.IsDespawned()) continue;

        RenderDataManager::SubmitMeshNodes(pickUp.GetMeshNodes());
    }

    // Clean me up
    for (ChristmasTree& object : Unloved::World::GetChristmasTrees())          RenderDataManager::SubmitRenderItems(object.GetRenderItems());
    for (ChristmasLightSet& object : Unloved::World::GetChristmasLightSets())  RenderDataManager::SubmitRenderItems(object.GetRenderItems());
    for (Fence& object : Unloved::World::GetFences())                          RenderDataManager::SubmitRenderItems(object.GetRenderItems());
    for (Jetty& object : Unloved::World::GetJetties())                         RenderDataManager::SubmitRenderItems(object.GetRenderItems());
    for (Ladder& object : Unloved::World::GetLadders())                        RenderDataManager::SubmitRenderItems(object.GetRenderItems());
    for (Light& object : Unloved::World::GetLights())                          RenderDataManager::SubmitRenderItems(object.GetRenderItems());
    for (Staircase& object : Unloved::World::GetStaircases())                  RenderDataManager::SubmitRenderItems(object.GetRenderItems());
    for (TrimSet& object : Unloved::World::GetTrimSets())                      RenderDataManager::SubmitRenderItems(object.GetRenderItems());
    for (PowerPoleSet& object : Unloved::World::GetPowerPoleSets())            RenderDataManager::SubmitRenderItems(object.GetRenderItems());
    for (PlanarQuadObject& object : Unloved::World::GetPlanarQuadObjects())   object.SubmitRenderItems();
    for (PointPairObject& object : Unloved::World::GetPointPairObjects())     object.SubmitRenderItems();
    for (Wall& object : Unloved::World::GetWalls())                            RenderDataManager::SubmitRenderItems(object.GetWeatherBoardstopRenderItems());

    if (Unloved::EditorSession::IsActive()) {
        for (HouseLocation& object : Unloved::World::GetHouseLocations())              RenderDataManager::SubmitRenderItems(object.GetRenderItems());
        for (SpawnPoint& object : Unloved::World::GetSpawnPointsCampaign())   RenderDataManager::SubmitRenderItems(object.GetRenderItems());
        for (SpawnPoint& object : Unloved::World::GetSpawnPointsDeathMatch()) RenderDataManager::SubmitRenderItems(object.GetRenderItems());
    }

    for (Wall& wall : Unloved::World::GetWalls()) {
        wall.SubmitRenderItems();
    }

    for (Wire& wire : Unloved::World::GetWires()) {
        wire.SubmitRenderItem();
    }

    for (WorldPlane& worldPlane : Unloved::World::GetWorldPlanes()) {
        worldPlane.SubmitRenderItem();
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
}

}
