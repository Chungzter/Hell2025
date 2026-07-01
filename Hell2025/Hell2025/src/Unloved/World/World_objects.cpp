#include "World.h"

#include "Hell/Common/Constants.h"
#include "Hell/Logging.h"

#include "Unloved/Bible/Bible.h"
#include "Unloved/Characters/Enemies/Dobermann/Dobermann.h"
#include "Unloved/Characters/Enemies/Kangaroo/Kangaroo.h"
#include "Unloved/Characters/Enemies/Shark/Shark.h"
#include "Unloved/Characters/Mermaids/Mermaid/Mermaid.h"
#include "Unloved/Editor/ObjectNames.h"
#include "Unloved/ObjectId.h"
#include "Unloved/Objects/Exterior/Fence.h"
#include "Unloved/Objects/Exterior/PowerPoleSet.h"
#include "Unloved/Objects/Exterior/Wire.h"
#include "Unloved/Objects/Effects/Decal.h"
#include "Unloved/Objects/House/Door.h"
#include "Unloved/Objects/House/Fireplace.h"
#include "Unloved/Objects/House/WorldPlane.h"
#include "Unloved/Objects/House/TrimSet.h"
#include "Unloved/Objects/House/Wall.h"
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
#include "Unloved/Objects/Spawns/SpawnPoint.h"
#include "Unloved/Objects/Traversal/Ladder.h"
#include "Unloved/Objects/Traversal/Staircase.h"
#include "Unloved/Systems/DDGI/DDGIVolume.h"

namespace Unloved::World {

    Hell::SlotMap<AnimatedGameObject> g_animatedGameObjects;
    Hell::SlotMap<BulletCasing> g_bulletCasings;
    Hell::SlotMap<ChristmasLightSet> g_christmasLightSets;
    Hell::SlotMap<ChristmasTree> g_christmasTrees;
    Hell::SlotMap<Decal> g_decals;
    Hell::SlotMap<DDGIVolume> g_ddgiVolumes;
    Hell::SlotMap<Dobermann> g_dobermanns;
    Hell::SlotMap<Door> g_doors;
    Hell::SlotMap<Fence> g_fences;
    Hell::SlotMap<Fireplace> g_fireplaces;
    Hell::SlotMap<GameObject> g_gameObjects;
    Hell::SlotMap<GenericObject> g_genericObjects;
    Hell::SlotMap<WorldPlane> g_worldPlanes;
    Hell::SlotMap<Kangaroo> g_kangaroos;
    Hell::SlotMap<Ladder> g_ladders;
    Hell::SlotMap<Light> g_lights;
    Hell::SlotMap<Mermaid> g_mermaids;
    Hell::SlotMap<Piano> g_pianos;
    Hell::SlotMap<PickUp> g_pickUps;
    Hell::SlotMap<PictureFrame> g_pictureFrames;
    Hell::SlotMap<PowerPoleSet> g_powerPoleSets;
    Hell::SlotMap<Shark> g_sharks;
    Hell::SlotMap<SpawnPoint> g_spawnPointsCampaign;
    Hell::SlotMap<SpawnPoint> g_spawnPointsDeathMatch;
    Hell::SlotMap<Staircase> g_staircases;
    Hell::SlotMap<TrimSet> g_trimSets;
    Hell::SlotMap<Wall> g_walls;
    Hell::SlotMap<Wire> g_wires;
    Hell::SlotMap<Window> g_windows;

    Hell::SlotMap<AnimatedGameObject>& GetAnimatedGameObjects() { return g_animatedGameObjects; }
    Hell::SlotMap<BulletCasing>& GetBulletCasings()             { return g_bulletCasings; }
    Hell::SlotMap<ChristmasLightSet>& GetChristmasLightSets()   { return g_christmasLightSets; }
    Hell::SlotMap<ChristmasTree>& GetChristmasTrees()           { return g_christmasTrees; }
    Hell::SlotMap<Decal>& GetDecals()                           { return g_decals; }
    Hell::SlotMap<DDGIVolume>& GetDDGIVolumes()                 { return g_ddgiVolumes; }
    Hell::SlotMap<Dobermann>& GetDobermanns()                   { return g_dobermanns; }
    Hell::SlotMap<Door>& GetDoors()                             { return g_doors; }
    Hell::SlotMap<Fence>& GetFences()                           { return g_fences; }
    Hell::SlotMap<Fireplace>& GetFireplaces()                   { return g_fireplaces; }
    Hell::SlotMap<GameObject>& GetGameObjects()                 { return g_gameObjects; }
    Hell::SlotMap<GenericObject>& GetGenericObjects()           { return g_genericObjects; }
    Hell::SlotMap<WorldPlane>& GetWorldPlanes()                 { return g_worldPlanes; }
    Hell::SlotMap<Kangaroo>& GetKangaroos()                     { return g_kangaroos; }
    Hell::SlotMap<Ladder>& GetLadders()                         { return g_ladders; }
    Hell::SlotMap<Light>& GetLights()                           { return g_lights; }
    Hell::SlotMap<Mermaid>& GetMermaids()                       { return g_mermaids; }
    Hell::SlotMap<Piano>& GetPianos()                           { return g_pianos; }
    Hell::SlotMap<PickUp>& GetPickUps()                         { return g_pickUps; }
    Hell::SlotMap<PictureFrame>& GetPictureFrames()             { return g_pictureFrames; }
    Hell::SlotMap<PowerPoleSet>& GetPowerPoleSets()             { return g_powerPoleSets; }
    Hell::SlotMap<Shark>& GetSharks()                           { return g_sharks; }
    Hell::SlotMap<SpawnPoint>& GetSpawnPointsCampaign()         { return g_spawnPointsCampaign; }
    Hell::SlotMap<SpawnPoint>& GetSpawnPointsDeathMatch()       { return g_spawnPointsDeathMatch; }
    Hell::SlotMap<Staircase>& GetStaircases()                   { return g_staircases; }
    Hell::SlotMap<TrimSet>& GetTrimSets()                       { return g_trimSets; }
    Hell::SlotMap<Wall>& GetWalls()                             { return g_walls; }
    Hell::SlotMap<Wire>& GetWires()                             { return g_wires; }
    Hell::SlotMap<Window>& GetWindows()                         { return g_windows; }

    // Animated Game Objects

    uint64_t AddAnimatedGameObject() {
        const uint64_t id = GetNextObjectId(ObjectType::ANIMATED_GAME_OBJECT);
        g_animatedGameObjects.emplace_with_id(id, id);
        return id;
    }

    AnimatedGameObject* GetAnimatedGameObjectByObjectId(uint64_t objectId) {
        return GetAnimatedGameObjects().get(objectId);
    }

    // Bullet Casings

    uint64_t AddBulletCasing(BulletCasingCreateInfo createInfo) {
        Editor::AssignEditorName(createInfo, GetBulletCasings());
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::BULLET_CASING);
        GetBulletCasings().emplace_with_id(id, id, createInfo);
        return id;
    }

    BulletCasing* GetBulletCasingByObjectId(uint64_t objectId) {
        return GetBulletCasings().get(objectId);
    }

    // Christmas Lights

    uint64_t AddChristmasLights(ChristmasLightsCreateInfo createInfo, SpawnOffset spawnOffset) {
        Editor::AssignEditorName(createInfo, GetChristmasLightSets());
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::CHRISTMAS_LIGHTS);
        GetChristmasLightSets().emplace_with_id(id, id, createInfo, spawnOffset);
        return id;
    }

    ChristmasLightSet* GetChristmasLightsByObjectId(uint64_t objectId) {
        return GetChristmasLightSets().get(objectId);
    }

    // Christmas Trees

    uint64_t AddChristmasTree(ChristmasTreeCreateInfo createInfo, SpawnOffset spawnOffset) {
        Editor::AssignEditorName(createInfo, GetChristmasTrees());
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::TREE);
        GetChristmasTrees().emplace_with_id(id, id, createInfo, spawnOffset);
        return id;
    }

    ChristmasTree* GetChristmasTreeByObjectId(uint64_t objectId) {
        return GetChristmasTrees().get(objectId);
    }

    // Decals

    uint64_t AddDecal(DecalCreateInfo createInfo) {
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::DECAL);
        GetDecals().emplace_with_id(id, id, createInfo);
        return id;
    }

    Decal* GetDecalByObjectId(uint64_t objectId) {
        return GetDecals().get(objectId);
    }

    // DDGI Volumes

    uint64_t AddDDGIVolume(DDGIVolumeCreateInfo createInfo, SpawnOffset spawnOffset) {
        Editor::AssignEditorName(createInfo, GetDDGIVolumes());
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::DDGI_VOLUME);
        GetDDGIVolumes().emplace_with_id(id, id, createInfo, spawnOffset);
        return id;
    }

    DDGIVolume* GetDDGIVolumeByObjectId(uint64_t objectId) {
        return GetDDGIVolumes().get(objectId);
    }

    // Dobermanns

    uint64_t AddDobermann(DobermannCreateInfo createInfo, SpawnOffset spawnOffset) {
        Editor::AssignEditorName(createInfo, GetDobermanns());
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::DOBERMANN);
        GetDobermanns().emplace_with_id(id, id, createInfo, spawnOffset);
        return id;
    }

    Dobermann* GetDobermannByObjectId(uint64_t objectId) {
        return GetDobermanns().get(objectId);
    }

    // Doors

    uint64_t AddDoor(DoorCreateInfo createInfo, SpawnOffset spawnOffset) {
        Editor::AssignEditorName(createInfo, GetDoors());
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::DOOR);
        GetDoors().emplace_with_id(id, id, createInfo, spawnOffset);
        return id;
    }

    Door* GetDoorByObjectId(uint64_t objectId) {
        return GetDoors().get(objectId);
    }

    // Fences

    uint64_t AddFence(FenceCreateInfo createInfo, SpawnOffset spawnOffset) {
        Editor::AssignEditorName(createInfo, GetFences());
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::FENCE);
        GetFences().emplace_with_id(id, id, createInfo, spawnOffset);
        return id;
    }

    Fence* GetFenceByObjectId(uint64_t objectId) {
        return GetFences().get(objectId);
    }

    // Fireplaces

    uint64_t AddFireplace(FireplaceCreateInfo createInfo, SpawnOffset spawnOffset) {
        Editor::AssignEditorName(createInfo, GetFireplaces());
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::FIREPLACE);
        GetFireplaces().emplace_with_id(id, id, createInfo, spawnOffset);
        return id;
    }

    Fireplace* GetFireplaceById(uint64_t objectId) {
        return GetFireplaces().get(objectId);
    }

    // Game Objects

    uint64_t AddGameObject(GameObjectCreateInfo createInfo, SpawnOffset spawnOffset) {
        Editor::AssignEditorName(createInfo, GetGameObjects());
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::GAME_OBJECT);
        GetGameObjects().emplace_with_id(id, id, createInfo, spawnOffset);
        return id;
    }

    GameObject* GetGameObjectByObjectId(uint64_t objectId) {
        return GetGameObjects().get(objectId);
    }

    GameObject* GetGameObjectByIndex(int32_t index) {
        if (index >= 0 && index < static_cast<int32_t>(GetGameObjects().size())) {
            return &GetGameObjects()[index];
        }
        return nullptr;
    }

    GameObject* GetGameObjectByName(const std::string& name) {
        for (GameObject& gameObject : GetGameObjects()) {
            if (gameObject.m_name == name) {
                return &gameObject;
            }
        }
        return nullptr;
    }

    // Generic Objects

    uint64_t AddGenericObject(GenericObjectCreateInfo createInfo, SpawnOffset spawnOffset) {
        Editor::AssignEditorName(createInfo, GetGenericObjects());
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::GENERIC_OBJECT);

        GetGenericObjects().emplace_with_id(id, id, createInfo, spawnOffset);
        return id;
    }

    GenericObject* GetGenericObjectById(uint64_t objectId) {
        return GetGenericObjects().get(objectId);
    }

    // Kangaroos

    uint64_t AddKangaroo(KangarooCreateInfo createInfo, SpawnOffset spawnOffset) {
        Editor::AssignEditorName(createInfo, GetKangaroos());
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::KANGAROO);
        GetKangaroos().emplace_with_id(id, id, createInfo, spawnOffset);
        return id;
    }

    Kangaroo* GetKangarooByObjectId(uint64_t objectId) {
        return GetKangaroos().get(objectId);
    }

    // Ladders

    uint64_t AddLadder(LadderCreateInfo createInfo, SpawnOffset spawnOffset) {
        Editor::AssignEditorName(createInfo, GetLadders());
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::LADDER);
        GetLadders().emplace_with_id(id, id, createInfo, spawnOffset);
        return id;
    }

    Ladder* GetLadderByObjectId(uint64_t objectId) {
        return GetLadders().get(objectId);
    }

    // Lights

    uint64_t AddLight(LightCreateInfo createInfo, SpawnOffset spawnOffset) {
        Editor::AssignEditorName(createInfo, GetLights());
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::LIGHT);
        GetLights().emplace_with_id(id, id, createInfo, spawnOffset);
        return id;
    }

    Light* GetLightByObjectId(uint64_t objectId) {
        return GetLights().get(objectId);
    }

    Light* GetLightByIndex(int32_t index) {
        if (index >= 0 && index < static_cast<int32_t>(GetLights().size())) {
            return &GetLights()[index];
        }

        Logging::Warning() << "World::GetLightByIndex() failed: index " << index << " out of range of size " << GetLights().size();
        return nullptr;
    }

    uint32_t GetLightCount() {
        return static_cast<uint32_t>(GetLights().size());
    }

    std::vector<uint64_t> GetLightIds() {
        std::vector<uint64_t> ids;
        ids.reserve(GetLights().size());

        for (Light& light : GetLights()) {
            if (light.GetType() != LightType::FIREPLACE_FIRE) {
                ids.push_back(light.GetObjectId());
            }
        }

        return ids;
    }

    // Mermaids

    uint64_t AddMermaid(MermaidCreateInfo createInfo, SpawnOffset spawnOffset) {
        Editor::AssignEditorName(createInfo, GetMermaids());
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::MERMAID);
        GetMermaids().emplace_with_id(id, id, createInfo, spawnOffset);
        return id;
    }

    Mermaid* GetMermaidByObjectId(uint64_t objectId) {
        return GetMermaids().get(objectId);
    }

    // Pianos

    uint64_t AddPiano(PianoCreateInfo createInfo, SpawnOffset spawnOffset) {
        if (createInfo.soundFontName == UNDEFINED_STRING) {
            createInfo.soundFontName = "YamahaGrandLiteV2";
        }

        Editor::AssignEditorName(createInfo, GetPianos());
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::PIANO);
        GetPianos().emplace_with_id(id, id, createInfo, spawnOffset);
        return id;
    }

    Piano* GetPianoByObjectId(uint64_t objectId) {
        return GetPianos().get(objectId);
    }

    Piano* GetPianoByMeshNodeObjectId(uint64_t objectId) {
        for (Piano& piano : GetPianos()) {
            MeshNodes& meshNodes = piano.GetMeshNodes();
            if (meshNodes.HasNodeWithObjectId(objectId)) {
                return &piano;
            }
        }
        return nullptr;
    }

    PianoKey* GetPianoKeyByObjectId(uint64_t objectId) {
        for (Piano& piano : GetPianos()) {
            if (piano.PianoKeyExists(objectId)) {
                return piano.GetPianoKey(objectId);
            }
        }
        return nullptr;
    }

    // Pick Ups

    uint64_t AddPickUp(PickUpCreateInfo createInfo, SpawnOffset spawnOffset) {
        if (!Bible::GetItemInfoByName(createInfo.name)) {
            Logging::Warning() << "World::AddPickUp(..) failed: '" << createInfo.name << "' ItemInfo not found in bible";
            return 0;
        }

        Editor::AssignEditorName(createInfo, GetPickUps());
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::PICK_UP);
        GetPickUps().emplace_with_id(id, id, createInfo, spawnOffset);
        return id;
    }

    PickUp* GetPickUpByObjectId(uint64_t objectId) {
        return GetPickUps().get(objectId);
    }

    // Picture Frames

    uint64_t AddPictureFrame(PictureFrameCreateInfo createInfo, SpawnOffset spawnOffset) {
        Editor::AssignEditorName(createInfo, GetPictureFrames());
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::PICTURE_FRAME);
        GetPictureFrames().emplace_with_id(id, id, createInfo, spawnOffset);
        return id;
    }

    PictureFrame* GetPictureFrameByObjectId(uint64_t objectId) {
        return GetPictureFrames().get(objectId);
    }

    // Power Pole Sets

    uint64_t AddPowerPoleSet(PowerPoleSetCreateInfo createInfo, SpawnOffset spawnOffset) {
        Editor::AssignEditorName(createInfo, GetPowerPoleSets());
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::POWER_POLE_SET);
        GetPowerPoleSets().emplace_with_id(id, id, createInfo, spawnOffset);
        return id;
    }

    PowerPoleSet* GetPowerPoleSetByObjectId(uint64_t objectId) {
        return GetPowerPoleSets().get(objectId);
    }

    // Sharks

    uint64_t AddShark(SharkCreateInfo createInfo, SpawnOffset spawnOffset) {
        Editor::AssignEditorName(createInfo, g_sharks);
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::SHARK);
        g_sharks.emplace_with_id(id, id, createInfo, spawnOffset);
        return id;
    }

    Shark* GetSharkByObjectId(uint64_t objectId) {
        return g_sharks.get(objectId);
    }

    // Spawn Points

    uint64_t AddSpawnPointCampaign(SpawnPointCreateInfo createInfo, SpawnOffset spawnOffset) {
        Editor::AssignEditorName(createInfo, g_spawnPointsCampaign);
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::SPAWN_POINT);
        g_spawnPointsCampaign.emplace_with_id(id, id, createInfo, spawnOffset);
        return id;
    }

    uint64_t AddSpawnPointDeathMatch(SpawnPointCreateInfo createInfo, SpawnOffset spawnOffset) {
        Editor::AssignEditorName(createInfo, g_spawnPointsDeathMatch);
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::SPAWN_POINT);
        g_spawnPointsDeathMatch.emplace_with_id(id, id, createInfo, spawnOffset);
        return id;
    }

    SpawnPoint* GetSpawnPointCampaignByObjectId(uint64_t objectId) {
        return g_spawnPointsCampaign.get(objectId);
    }

    SpawnPoint* GetSpawnPointDeathMatchByObjectId(uint64_t objectId) {
        return g_spawnPointsDeathMatch.get(objectId);
    }

    // Staircases

    uint64_t AddStaircase(StaircaseCreateInfo createInfo, SpawnOffset spawnOffset) {
        Editor::AssignEditorName(createInfo, GetStaircases());
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::STAIRCASE);
        GetStaircases().emplace_with_id(id, id, createInfo, spawnOffset);
        return id;
    }

    Staircase* GetStaircaseByObjectId(uint64_t objectId) {
        return GetStaircases().get(objectId);
    }

    // Trim Sets

    uint64_t AddTrimSet(TrimSetCreateInfo createInfo, SpawnOffset spawnOffset) {
        Editor::AssignEditorName(createInfo, GetTrimSets());
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::TRIM_SET);
        GetTrimSets().emplace_with_id(id, id, createInfo, spawnOffset);
        return id;
    }

    TrimSet* GetTrimSetByObjectId(uint64_t objectId) {
        return GetTrimSets().get(objectId);
    }

    // Walls

    uint64_t AddWall(WallCreateInfo createInfo, SpawnOffset spawnOffset) {
        if (createInfo.points.empty()) {
            Logging::Warning() << "World::AddWall() failed: createInfo has zero points!";
            return 0;
        }

        Editor::AssignEditorName(createInfo, GetWalls());
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::WALL);

        GetWalls().emplace_with_id(id, id, createInfo, spawnOffset);
        return id;
    }

    Wall* GetWallByObjectId(uint64_t objectId) {
        return GetWalls().get(objectId);
    }

    Wall* GetWallByWallSegmentObjectId(uint64_t objectId) {
        for (Wall& wall : GetWalls()) {
            for (WallSegment& wallSegment : wall.GetWallSegments()) {
                if (wallSegment.GetObjectId() == objectId) {
                    return &wall;
                }
            }
        }
        return nullptr;
    }

    // Wires

    uint64_t AddWire(WireCreateInfo createInfo) {
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::WIRE);
        GetWires().emplace_with_id(id, id, createInfo);
        return id;
    }

    Wire* GetWireByObjectId(uint64_t objectId) {
        return GetWires().get(objectId);
    }

    // Windows

    uint64_t AddWindow(WindowCreateInfo createInfo, SpawnOffset spawnOffset) {
        Editor::AssignEditorName(createInfo, GetWindows());
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::WINDOW);
        GetWindows().emplace_with_id(id, id, createInfo, spawnOffset);
        return id;
    }

    Window* GetWindowByObjectId(uint64_t objectId) {
        return GetWindows().get(objectId);
    }

    // World Planes

    uint64_t AddWorldPlane(WorldPlaneCreateInfo createInfo, SpawnOffset spawnOffset) {
        Editor::AssignEditorName(createInfo, GetWorldPlanes());
        const uint64_t id = Unloved::GetNextObjectId(ObjectType::WORLD_PLANE);

        GetWorldPlanes().emplace_with_id(id, id, createInfo, spawnOffset);
        return id;
    }

    WorldPlane* GetWorldPlaneByObjectId(uint64_t objectId) {
        return GetWorldPlanes().get(objectId);
    }

    // Remove Object

    template<typename Container>
    bool RemoveFromSlotMap(Container& objects, uint64_t objectId) {
        if (!objects.contains(objectId)) return false;

        objects.get(objectId)->CleanUp();
        objects.erase(objectId);
        return true;
    }

    bool RemoveObjectById(uint64_t objectId) {
        if (objectId == 0) return false;

        switch (GetObjectIdType(objectId)) {
        case ObjectType::ANIMATED_GAME_OBJECT: return RemoveFromSlotMap(GetAnimatedGameObjects(), objectId);
        case ObjectType::BULLET_CASING:        return RemoveFromSlotMap(GetBulletCasings(), objectId);
        case ObjectType::CHRISTMAS_LIGHTS:     return RemoveFromSlotMap(GetChristmasLightSets(), objectId);
        case ObjectType::TREE:                 return RemoveFromSlotMap(GetChristmasTrees(), objectId);
        case ObjectType::DECAL:                return RemoveFromSlotMap(GetDecals(), objectId);
        case ObjectType::DDGI_VOLUME:          return RemoveFromSlotMap(GetDDGIVolumes(), objectId);
        case ObjectType::DOBERMANN:            return RemoveFromSlotMap(GetDobermanns(), objectId);
        case ObjectType::DOOR:                 return RemoveFromSlotMap(GetDoors(), objectId);
        case ObjectType::FENCE:                return RemoveFromSlotMap(GetFences(), objectId);
        case ObjectType::FIREPLACE:            return RemoveFromSlotMap(GetFireplaces(), objectId);
        case ObjectType::GAME_OBJECT:          return RemoveFromSlotMap(GetGameObjects(), objectId);
        case ObjectType::GENERIC_OBJECT:       return RemoveFromSlotMap(GetGenericObjects(), objectId);
        case ObjectType::WORLD_PLANE:          return RemoveFromSlotMap(GetWorldPlanes(), objectId);
        case ObjectType::KANGAROO:             return RemoveFromSlotMap(GetKangaroos(), objectId);
        case ObjectType::LADDER:               return RemoveFromSlotMap(GetLadders(), objectId);
        case ObjectType::LIGHT:                return RemoveFromSlotMap(GetLights(), objectId);
        case ObjectType::MERMAID:              return RemoveFromSlotMap(GetMermaids(), objectId);
        case ObjectType::PIANO:                return RemoveFromSlotMap(GetPianos(), objectId);
        case ObjectType::PICK_UP:              return RemoveFromSlotMap(GetPickUps(), objectId);
        case ObjectType::PICTURE_FRAME:        return RemoveFromSlotMap(GetPictureFrames(), objectId);
        case ObjectType::POWER_POLE_SET:       return RemoveFromSlotMap(GetPowerPoleSets(), objectId);
        case ObjectType::SHARK:                return RemoveFromSlotMap(GetSharks(), objectId);
        case ObjectType::STAIRCASE:            return RemoveFromSlotMap(GetStaircases(), objectId);
        case ObjectType::TRIM_SET:             return RemoveFromSlotMap(GetTrimSets(), objectId);
        case ObjectType::WALL:                 return RemoveFromSlotMap(GetWalls(), objectId);
        case ObjectType::WIRE:                 return RemoveFromSlotMap(GetWires(), objectId);
        case ObjectType::WINDOW:               return RemoveFromSlotMap(GetWindows(), objectId);

        case ObjectType::SPAWN_POINT:
            if (RemoveFromSlotMap(GetSpawnPointsCampaign(), objectId)) return true;
            return RemoveFromSlotMap(GetSpawnPointsDeathMatch(), objectId);

        default:
            Logging::Error() << "World::RemoveObjectById() failed: unsupported object type '" << Hell::Enum::ToString(GetObjectIdType(objectId)) << "'\n";
            return false;
        }
    }

    MeshNode* GetMeshNodeByObjectIdAndLocalNodeIndex(uint64_t objectId, int32_t meshNodeLocalIndex) {
        if (meshNodeLocalIndex < 0) return nullptr;

        switch (GetObjectIdType(objectId)) {
        case ObjectType::DOOR:
            if (Door* object = GetDoorByObjectId(objectId)) return object->GetMeshNodes().GetMeshNodeByLocalIndex(meshNodeLocalIndex);
            return nullptr;
        case ObjectType::FIREPLACE:
            if (Fireplace* object = GetFireplaceById(objectId)) return object->GetMeshNodes().GetMeshNodeByLocalIndex(meshNodeLocalIndex);
            return nullptr;
        case ObjectType::GAME_OBJECT:
            if (GameObject* object = GetGameObjectByObjectId(objectId)) return object->GetMeshNodes().GetMeshNodeByLocalIndex(meshNodeLocalIndex);
            return nullptr;
        case ObjectType::GENERIC_OBJECT:
            if (GenericObject* object = GetGenericObjectById(objectId)) return object->GetMeshNodes().GetMeshNodeByLocalIndex(meshNodeLocalIndex);
            return nullptr;
        case ObjectType::LADDER:
            if (Ladder* object = GetLadderByObjectId(objectId)) return object->GetMeshNodes().GetMeshNodeByLocalIndex(meshNodeLocalIndex);
            return nullptr;
        case ObjectType::LIGHT:
            if (Light* object = GetLightByObjectId(objectId)) return object->GetMeshNodes().GetMeshNodeByLocalIndex(meshNodeLocalIndex);
            return nullptr;
        case ObjectType::MERMAID:
            if (Mermaid* object = GetMermaidByObjectId(objectId)) return object->GetMeshNodes().GetMeshNodeByLocalIndex(meshNodeLocalIndex);
            return nullptr;
        case ObjectType::PIANO:
            if (Piano* object = GetPianoByObjectId(objectId)) return object->GetMeshNodes().GetMeshNodeByLocalIndex(meshNodeLocalIndex);
            return nullptr;
        case ObjectType::PICK_UP:
            if (PickUp* object = GetPickUpByObjectId(objectId)) return object->GetMeshNodes().GetMeshNodeByLocalIndex(meshNodeLocalIndex);
            return nullptr;
        case ObjectType::PICTURE_FRAME:
            if (PictureFrame* object = GetPictureFrameByObjectId(objectId)) return object->GetMeshNodes().GetMeshNodeByLocalIndex(meshNodeLocalIndex);
            return nullptr;
        case ObjectType::WINDOW:
            if (Window* object = GetWindowByObjectId(objectId)) return object->GetMeshNodes().GetMeshNodeByLocalIndex(meshNodeLocalIndex);
            return nullptr;
        default:
            return nullptr;
        }
    }

    // Clean Up

    template<typename Container>
    void CleanUpSlotMap(Container& objects) {
        for (auto& object : objects) {
            object.CleanUp();
        }
        objects.clear();
    }

    void CleanUpCasings() {
        CleanUpSlotMap(g_bulletCasings);
    }

    void CleanUpDecals() {
        CleanUpSlotMap(g_decals);
    }

    void CleanUpAll() {
        CleanUpSlotMap(g_bulletCasings);
        CleanUpSlotMap(g_christmasLightSets);
        CleanUpSlotMap(g_christmasTrees);
        CleanUpSlotMap(g_decals);
        CleanUpSlotMap(g_ddgiVolumes);
        CleanUpSlotMap(g_dobermanns);
        CleanUpSlotMap(g_doors);
        CleanUpSlotMap(g_fences);
        CleanUpSlotMap(g_fireplaces);
        CleanUpSlotMap(g_gameObjects);
        CleanUpSlotMap(g_genericObjects);
        CleanUpSlotMap(g_worldPlanes);
        CleanUpSlotMap(g_kangaroos);
        CleanUpSlotMap(g_ladders);
        CleanUpSlotMap(g_lights);
        CleanUpSlotMap(g_mermaids);
        CleanUpSlotMap(g_pianos);
        CleanUpSlotMap(g_pickUps);
        CleanUpSlotMap(g_pictureFrames);
        CleanUpSlotMap(g_powerPoleSets);
        CleanUpSlotMap(g_sharks);
        CleanUpSlotMap(g_spawnPointsCampaign);
        CleanUpSlotMap(g_spawnPointsDeathMatch);
        CleanUpSlotMap(g_staircases);
        CleanUpSlotMap(g_trimSets);
        CleanUpSlotMap(g_walls);
        CleanUpSlotMap(g_wires);
        CleanUpSlotMap(g_windows);
    }

}
