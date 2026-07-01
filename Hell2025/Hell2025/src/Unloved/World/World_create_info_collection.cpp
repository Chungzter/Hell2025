#include "World.h"

#include "Unloved/Objects/Exterior/Fence.h"
#include "Unloved/Objects/Exterior/PowerPoleSet.h"
#include "Unloved/Objects/House/Door.h"
#include "Unloved/Objects/House/Fireplace.h"
#include "Unloved/Objects/House/HousePlane.h"
#include "Unloved/Objects/House/Wall.h"
#include "Unloved/Objects/House/Window.h"
#include "Unloved/Objects/Interior/Piano.h"
#include "Unloved/Objects/Interior/PictureFrame.h"
#include "Unloved/Objects/Lighting/Light.h"
#include "Unloved/Objects/Props/Christmas/ChristmasLights.h"
#include "Unloved/Objects/Props/GenericObject.h"
#include "Unloved/Objects/Props/PickUp.h"
#include "Unloved/Objects/Traversal/Ladder.h"
#include "Unloved/Objects/Traversal/Staircase.h"
#include "Unloved/Systems/DDGI/DDGIVolume.h"

namespace Unloved::World {

    // Adds all objects within a CreateInfoCollection to the world
    void AddCreateInfoCollection(CreateInfoCollection& createInfoCollection, SpawnOffset spawnOffset) {
        for (ChristmasLightsCreateInfo& createInfo : createInfoCollection.christmasLights)  AddChristmasLights(createInfo, spawnOffset);
        for (DDGIVolumeCreateInfo& createInfo : createInfoCollection.ddgiVolumes)           AddDDGIVolume(createInfo, spawnOffset);
        for (DoorCreateInfo& createInfo : createInfoCollection.doors)                       AddDoor(createInfo, spawnOffset);
        for (FenceCreateInfo& createInfo : createInfoCollection.fences)                     AddFence(createInfo, spawnOffset);
        for (FireplaceCreateInfo& createInfo : createInfoCollection.fireplaces)             AddFireplace(createInfo, spawnOffset);
        for (GenericObjectCreateInfo& createInfo : createInfoCollection.genericObjects)     AddGenericObject(createInfo, spawnOffset);
        for (LightCreateInfo& createInfo : createInfoCollection.lights)                     AddLight(createInfo, spawnOffset);
        for (LadderCreateInfo& createInfo : createInfoCollection.ladders)                   AddLadder(createInfo, spawnOffset);
        for (PianoCreateInfo& createInfo : createInfoCollection.pianos)                     AddPiano(createInfo, spawnOffset);
        for (PickUpCreateInfo& createInfo : createInfoCollection.pickUps)                   AddPickUp(createInfo, spawnOffset);
        for (PictureFrameCreateInfo& createInfo : createInfoCollection.pictureFrames)       AddPictureFrame(createInfo, spawnOffset);
        for (PowerPoleSetCreateInfo& createInfo : createInfoCollection.powerPoleSets)       AddPowerPoleSet(createInfo, spawnOffset);
        for (HousePlaneCreateInfo& createInfo : createInfoCollection.worldPlanes)           AddHousePlane(createInfo, spawnOffset);
        for (StaircaseCreateInfo& createInfo : createInfoCollection.staircases)             AddStaircase(createInfo, spawnOffset);
        for (WallCreateInfo& createInfo : createInfoCollection.walls)                       AddWall(createInfo, spawnOffset);
        for (WindowCreateInfo& createInfo : createInfoCollection.windows)                   AddWindow(createInfo, spawnOffset);
    }

    template<typename Object, typename Container>
    void AddObject(Object& object, Container& container) {
            container.push_back(object.GetCreateInfo());
    }

    template<typename Object, typename Container>
    void AddObjectIfMarkedForSaving(Object& object, Container& container) {
        if (object.GetCreateInfo().saveToFile) {
            container.push_back(object.GetCreateInfo());
        }
    }

    template<typename Container>
    void AddWorldPlaneIfNotDoorChild(WorldPlane& object, Container& container) {
        if (object.GetParentDoorId() == 0) {
            container.push_back(object.GetCreateInfo());
        }
    }

    // Creates a CreateInfoCollection from all objects in the world
    CreateInfoCollection GetCreateInfoCollection() {
        CreateInfoCollection createInfoCollection;

        for (ChristmasLightSet& object : GetChristmasLightSets()) AddObject(object, createInfoCollection.christmasLights);
        for (Door& object : GetDoors())                           AddObject(object, createInfoCollection.doors);
        for (Fence& object : GetFences())                         AddObject(object, createInfoCollection.fences);
        for (Fireplace& object : GetFireplaces())                 AddObject(object, createInfoCollection.fireplaces);
        for (GenericObject& object : GetGenericObjects())         AddObject(object, createInfoCollection.genericObjects);
        for (Ladder& object : GetLadders())                       AddObject(object, createInfoCollection.ladders);
        for (Piano& object : GetPianos())                         AddObject(object, createInfoCollection.pianos);
        for (PictureFrame& object : GetPictureFrames())           AddObject(object, createInfoCollection.pictureFrames);
        for (PowerPoleSet& object : GetPowerPoleSets())           AddObject(object, createInfoCollection.powerPoleSets);
        for (Staircase& object : GetStaircases())                 AddObject(object, createInfoCollection.staircases);
        for (Wall& object : GetWalls())                           AddObject(object, createInfoCollection.walls);
        for (Window& object : GetWindows())                       AddObject(object, createInfoCollection.windows);

        // Conditionals
        for (DDGIVolume& object : GetDDGIVolumes())               AddObjectIfMarkedForSaving(object, createInfoCollection.ddgiVolumes);
        for (Light& object : GetLights())                         AddObjectIfMarkedForSaving(object, createInfoCollection.lights);
        for (PickUp& object : GetPickUps())                       AddObjectIfMarkedForSaving(object, createInfoCollection.pickUps);
        for (WorldPlane& object : GetWorldPlanes())               AddWorldPlaneIfNotDoorChild(object, createInfoCollection.worldPlanes);

        return createInfoCollection;
    }
}
