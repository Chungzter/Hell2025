#include "World.h"

#include "Hell/Time.h"

#include "Unloved/Characters/Enemies/Dobermann/Dobermann.h"
#include "Unloved/Characters/Enemies/Kangaroo/Kangaroo.h"
#include "Unloved/Characters/Enemies/Shark/Shark.h"
#include "Unloved/Characters/Mermaids/Mermaid/Mermaid.h"
#include "Unloved/Editor/Editor.h"
#include "Unloved/Objects/Effects/Decal.h"
#include "Unloved/Objects/Exterior/Fence.h"
#include "Unloved/Objects/Exterior/PowerPoleSet.h"
#include "Unloved/Objects/Exterior/Road.h"
#include "Unloved/Objects/House/Door.h"
#include "Unloved/Objects/House/Fireplace.h"
#include "Unloved/Objects/House/TrimSet.h"
#include "Unloved/Objects/House/Window.h"
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
#include "Unloved/Objects/Traversal/Ladder.h"
#include "Unloved/Objects/Traversal/Staircase.h"
#include "Unloved/UI/Imgui/ImguiBackEnd.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Systems/DDGI/DDGIVolume.h"
#include "Unloved/Systems/P90Mag/P90MagManager.h"
#include "Unloved/Systems/WorldBVH/WorldBVH.h"

#include "Legacy/World/LegacyWorld.h"

namespace Unloved::World {
    void UpdateBvhs() {
        Unloved::WorldBVH::UpdateBvhs();
    }

    void UpdateObjects() {
        const float deltaTime = Hell::Time::DeltaTime();

        for (AnimatedGameObject& object : GetAnimatedGameObjects()) object.Update(deltaTime);
        for (BulletCasing& object : GetBulletCasings())             object.Update(deltaTime);
        for (ChristmasLightSet& object : GetChristmasLightSets())   object.Update(deltaTime);
        for (ChristmasTree& object : GetChristmasTrees())           object.Update(deltaTime);
        for (Dobermann& object : GetDobermanns())                   object.Update(deltaTime);
        for (Door& object : GetDoors())                             object.Update(deltaTime);
        for (Fence& object : GetFences())                           object.Update();
        for (Fireplace& object : GetFireplaces())                   object.Update(deltaTime);
        for (GameObject& object : GetGameObjects())                 object.Update(deltaTime);
        for (GenericObject& object : GetGenericObjects())           object.Update(deltaTime);
        for (Kangaroo& object : GetKangaroos())                     object.Update(deltaTime);
        for (Ladder& object : GetLadders())                         object.Update(deltaTime);
        for (Mermaid& object : GetMermaids())                       object.Update(deltaTime);
        for (Piano& object : GetPianos())                           object.Update(deltaTime);
        for (PickUp& object : GetPickUps())                         object.Update(deltaTime);
        for (PictureFrame& object : GetPictureFrames())             object.Update();
        for (PowerPoleSet& object : GetPowerPoleSets())             object.Update();
        for (Road& object : LegacyWorld::GetRoads())                object.Update();
        for (Shark& object : GetSharks())                           object.Update(deltaTime);
        for (Staircase& object : GetStaircases())                   object.Update(deltaTime);
        for (TrimSet& object : GetTrimSets())                       object.Update();
        for (Window& object : GetWindows())                         object.Update(deltaTime);

        // These must run in this order otherwise various dirty flags are stale
        for (DDGIVolume& object : GetDDGIVolumes())                 object.Update();
        for (Light& object : GetLights())                           object.Update(deltaTime);
        for (Decal& object : GetDecals())                           object.Update();

        P90MagManager::SubmitRenderItems();
    }

    void UpdatePlayers() {
        const float deltaTime = Hell::Time::DeltaTime();
        const bool disableControl = Editor::IsOpen() || ImGuiBackEnd::OwnsMouse();

        for (uint64_t playerId : Session::GetLocalPlayerIds()) {
            Unloved::Player* player = Session::GetPlayerById(playerId);
            if (!player) continue;

            if (disableControl) {
                player->DisableControl();
            }
            else {
                player->EnableControl();
            }
        }

        for (uint64_t playerId : Session::GetLocalPlayerIds()) {
            Unloved::Player* player = Session::GetPlayerById(playerId);
            if (!player) continue;

            player->Update(deltaTime);
        }
    }
}
