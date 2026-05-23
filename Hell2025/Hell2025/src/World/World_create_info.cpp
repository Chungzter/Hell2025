#include "World.h"

namespace World {

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
        for (HousePlaneCreateInfo& createInfo : createInfoCollection.housePlanes)           AddHousePlane(createInfo, spawnOffset);
        for (StaircaseCreateInfo& createInfo : createInfoCollection.staircases)             AddStaircase(createInfo, spawnOffset);
        for (TreeCreateInfo& createInfo : createInfoCollection.trees)                       AddTree(createInfo, spawnOffset);
        for (WallCreateInfo& createInfo : createInfoCollection.walls)                       AddWall(createInfo, spawnOffset);
        for (WindowCreateInfo& createInfo : createInfoCollection.windows)                   AddWindow(createInfo, spawnOffset);
    }

    CreateInfoCollection GetCreateInfoCollection() {
        CreateInfoCollection createInfoCollection;

        for (ChristmasLightSet& object : World::GetChristmasLightSets()) createInfoCollection.christmasLights.push_back(object.GetCreateInfo());
        for (Door& object : World::GetDoors())                           createInfoCollection.doors.push_back(object.GetCreateInfo());
        for (Fence& object : World::GetFences())                         createInfoCollection.fences.push_back(object.GetCreateInfo());
        for (Fireplace& object : World::GetFireplaces())                 createInfoCollection.fireplaces.push_back(object.GetCreateInfo());
        for (GenericObject& object : World::GetGenericObjects())         createInfoCollection.genericObjects.push_back(object.GetCreateInfo());
        for (Ladder& object : World::GetLadders())                       createInfoCollection.ladders.push_back(object.GetCreateInfo());
        //for (Light& object : World::GetLights())                       createInfoCollection.lights.push_back(object.GetCreateInfo());
        for (Piano& object : World::GetPianos())                         createInfoCollection.pianos.push_back(object.GetCreateInfo());
        for (PictureFrame& object : World::GetPictureFrames())           createInfoCollection.pictureFrames.push_back(object.GetCreateInfo());
        for (PowerPoleSet& object : World::GetPowerPoleSets())           createInfoCollection.powerPoleSets.push_back(object.GetCreateInfo());
        for (Staircase& object : World::GetStaircases())                 createInfoCollection.staircases.push_back(object.GetCreateInfo());
        for (Tree& object : World::GetTrees())                           createInfoCollection.trees.push_back(object.GetCreateInfo());
        for (Wall& object : World::GetWalls())                           createInfoCollection.walls.push_back(object.GetCreateInfo());
        for (Window& object : World::GetWindows())                       createInfoCollection.windows.push_back(object.GetCreateInfo());

        // Conditionals
        for (DDGIVolume& object : World::GetDDGIVolumes()) {
            if (object.GetCreateInfo().saveToFile) {
                createInfoCollection.ddgiVolumes.push_back(object.GetCreateInfo());
            }
        }

        for (HousePlane& housePlane : World::GetHousePlanes()) {
            if (housePlane.GetParentDoorId() == 0) {
                createInfoCollection.housePlanes.push_back(housePlane.GetCreateInfo());
            }
        }

        for (PickUp& object : World::GetPickUps()) {
            if (object.GetCreateInfo().saveToFile) {
                createInfoCollection.pickUps.push_back(object.GetCreateInfo());
            }
        }

        for (Light& object : World::GetLights()) {
            if (object.GetCreateInfo().saveToFile) {
                createInfoCollection.lights.push_back(object.GetCreateInfo());
            }
        }

        return createInfoCollection;
    }
}