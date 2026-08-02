#include "EditorInspector.h"
#include "Unloved/EditorSession/Inspector/EditorInspectorInternal.h"

#include "Unloved/EditorSession/UI/EditorDialogs.h"
#include "Unloved/EditorSession/HeightMap/EditorHeightMap.h"
#include "EditorHierarchy.h"
#include "Unloved/EditorSession/UI/EditorInputElements.h"
#include "EditorMapTools.h"
#include "EditorObjectOptions.h"
#include "Unloved/EditorSession/Interaction/EditorPointSequences.h"
#include "Unloved/EditorSession/Interaction/EditorSelection.h"
#include "EditorSession.h"
#include "Unloved/EditorSession/Core/EditorWorkspace.h"

#include "Hell/Common/Enum.h"

#include "Unloved/Editor/Gizmo.h"
#include "Unloved/ObjectId.h"
#include "Unloved/Objects/Exterior/Jetty.h"
#include "Unloved/Objects/House/Door.h"
#include "Unloved/Objects/House/Fireplace.h"
#include "Unloved/Objects/House/Wall.h"
#include "Unloved/Objects/House/WorldPlane.h"
#include "Unloved/Objects/Interior/PictureFrame.h"
#include "Unloved/Objects/Lighting/Light.h"
#include "Unloved/Objects/Props/GenericAnimatedObject.h"
#include "Unloved/Objects/Props/GenericObject.h"
#include "Unloved/Objects/Props/Christmas/ChristmasLights.h"
#include "Unloved/Objects/Props/PickUp.h"
#include "Unloved/Objects/Spawns/HouseLocation.h"
#include "Unloved/Objects/Spawns/SpawnPoint.h"
#include "Unloved/Objects/Traversal/Ladder.h"
#include "Unloved/Objects/Traversal/Staircase.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Systems/DDGI/DDGIVolume.h"
#include "Unloved/Systems/House/HouseBuilder.h"
#include "Unloved/World/World.h"

#include <algorithm>
#include <string>
#include <vector>

namespace Unloved::EditorSession::Inspector {
    namespace {
        void SetEditorName(uint64_t objectId, const std::string& editorName) {
            if (!World::SetEditorNameById(objectId, editorName)) {
                Dialog::Open("Name '" + editorName + "' Taken");
            }
        }

        void SetEditorPosition(uint64_t objectId, const glm::vec3& position) {
            if (World::SetPositionById(objectId, position)) {
                Gizmo::SetPosition(position);
            }
        }

        void SetEditorYaw(uint64_t objectId, float rotation) {
            const glm::vec3 fullRotation = glm::vec3(0.0f, rotation, 0.0f);
            if (World::SetRotationById(objectId, fullRotation)) {
                Gizmo::SetRotation(fullRotation);
            }
        }

        void SetEditorRotation(uint64_t objectId, const glm::vec3& rotation) {
            if (World::SetRotationById(objectId, rotation)) {
                Gizmo::SetRotation(rotation);
            }
        }

        void SetSpawnPointRotation(uint64_t objectId, const glm::vec2& rotation) {
            SetEditorRotation(objectId, glm::vec3(rotation, 0.0f));
        }

        void SetSpawnPointFromPlayerCamera(uint64_t objectId) {
            Player* player = Session::GetLocalPlayerByViewportIndex(0);
            if (!player) return;

            SetEditorPosition(objectId, player->GetCamera().GetPosition());
            SetEditorRotation(objectId, player->GetCamera().GetEulerRotation());
        }

        void TestSpawnPoint(SpawnPoint* spawnPoint) {
            Player* player = Session::GetLocalPlayerByViewportIndex(0);
            if (!player || !spawnPoint) return;

            player->SetFootPosition(spawnPoint->GetPosition() - glm::vec3(0.0f, 1.6f, 0.0f));
            player->GetCamera().SetEulerRotation(spawnPoint->GetCameraEuler());
            Unloved::EditorSession::Close();
        }

        void SetWallType(Wall* wall, const std::string& wallType, const std::string& materialName) {
            const WallType selectedType = Hell::Enum::FromString(wallType, WallType::UNDEFINED);
            wall->SetWallType(selectedType);

            const std::vector<std::string>& interiorMaterials = ObjectOptions::GetInteriorMaterials();
            const std::vector<std::string>& weatherBoardMaterials = ObjectOptions::GetWeatherBoardMaterials();

            if (selectedType == WallType::WEATHER_BOARDS && std::find(weatherBoardMaterials.begin(), weatherBoardMaterials.end(), materialName) == weatherBoardMaterials.end()) {
                Internal::ApplyWeatherBoardMaterialDefaults(wall, weatherBoardMaterials.front());
            }
            if (selectedType == WallType::INTERIOR && std::find(interiorMaterials.begin(), interiorMaterials.end(), materialName) == interiorMaterials.end()) {
                wall->SetMaterial(interiorMaterials.front());
            }
        }

        void SetWorkspaceMapName(const std::string& name) {
            if (!Workspace::SetMapName(name)) {
                Dialog::Open("Name '" + name + "' Taken");
            }
        }

        void SetWorkspaceHouseName(const std::string& name) {
            if (!Workspace::SetHouseName(name)) {
                Dialog::Open("Name '" + name + "' Taken");
            }
        }

        void RevertMapFromDisk() {
            if (Workspace::RevertMap()) {
                Hierarchy::Refresh();
            }
        }

        void RevertHouseFromDisk() {
            if (Workspace::RevertHouse()) {
                Hierarchy::Refresh();
            }
        }

        void SetWorldPlaneRotation(WorldPlane* worldPlane, const glm::vec3& rotation) {
            if (worldPlane->SetRotation(rotation)) {
                Gizmo::SetRotation(worldPlane->GetRotation());
            }
        }

        void SetWorldPlanePoint(WorldPlane* worldPlane, int32_t pointIndex, const glm::vec3& position) {
            if (worldPlane->SetPointPosition(pointIndex, position)) {
                Gizmo::SetPosition(worldPlane->GetWorldSpaceCenter());
            }
        }

        void SetSequencePointPosition(uint64_t objectId, int32_t pointIndex, PointSequences::PointHandleType handleType, const glm::vec3& position) {
            if (PointSequences::SetPointPosition(objectId, pointIndex, handleType, position)) {
                Gizmo::SetPosition(position);
            }
        }

        void SetWallSegmentPoint(Wall* wall, int32_t pointIndex, const glm::vec3& position, const glm::vec3& otherPosition) {
            if (wall->UpdatePointPosition(pointIndex, position)) {
                Gizmo::SetPosition((position + otherPosition) * 0.5f);
            }
        }

        void AddNameProperty(InputElements::PropertyList& properties, uint64_t objectId, std::string& editorName) {
            properties.String(objectId, "Name", editorName, [objectId, editorName = &editorName] { SetEditorName(objectId, *editorName); });
        }

        void AddPositionProperty(InputElements::PropertyList& properties, uint64_t objectId, glm::vec3& position) {
            properties.Vec3(objectId, "Position", position, [objectId, position = &position] { SetEditorPosition(objectId, *position); });
        }

        void AddYawProperty(InputElements::PropertyList& properties, uint64_t objectId, float& rotation) {
            properties.Float(objectId, "Rotation", rotation, [objectId, rotation = &rotation] { SetEditorYaw(objectId, *rotation); });
        }

        void AddEulerRotationProperty(InputElements::PropertyList& properties, uint64_t objectId, glm::vec3& rotation) {
            properties.Vec3(objectId, "Rotation", rotation, [objectId, rotation = &rotation] { SetEditorRotation(objectId, *rotation); });
        }

        void RenderWorkspaceProperties(const EditorRect& rect) {
            constexpr uint64_t WORKSPACE_PROPERTY_ID = UINT64_MAX;
            InputElements::PropertyList properties;
            std::string name = Workspace::GetName();
            if (Workspace::GetMode() == EditorSessionMode::MAP) {
                properties.String(WORKSPACE_PROPERTY_ID, "Name", name, [name = &name] { SetWorkspaceMapName(*name); });
                uint32_t chunkWidth = Workspace::GetMapChunkWidth();
                uint32_t chunkDepth = Workspace::GetMapChunkDepth();
                properties.UInt(WORKSPACE_PROPERTY_ID, "Chunk Width", chunkWidth, [&] { Workspace::ResizeMap(chunkWidth, chunkDepth); });
                properties.UInt(WORKSPACE_PROPERTY_ID, "Chunk Depth", chunkDepth, [&] { Workspace::ResizeMap(chunkWidth, chunkDepth); });
                properties.Button("Reset height map", [] { Workspace::ResetHeightMap(); });
                properties.Button("Revert from disk", RevertMapFromDisk);
            }
            else {
                properties.String(WORKSPACE_PROPERTY_ID, "Name", name, [name = &name] { SetWorkspaceHouseName(*name); });
                properties.Button("Revert from disk", RevertHouseFromDisk);
            }
            properties.Render(rect);
        }

        void RenderWorldPlaneProperties(const EditorRect& rect, uint64_t objectId) {
            WorldPlane* worldPlane = World::GetWorldPlaneByObjectId(objectId);
            InputElements::PropertyList properties;
            if (!worldPlane) {
                properties.Render(rect);
                return;
            }

            WorldPlaneCreateInfo& createInfo = worldPlane->GetCreateInfo();
            std::string editorName = World::GetEditorNameById(objectId);
            glm::vec3 position = World::GetPositionById(objectId);
            glm::vec3 rotation = worldPlane->GetRotation();
            glm::vec3 p0 = worldPlane->GetPlanarQuad().GetPositionP0();
            glm::vec3 p1 = worldPlane->GetPlanarQuad().GetPositionP1();
            glm::vec3 p2 = worldPlane->GetPlanarQuad().GetPositionP2();
            glm::vec3 p3 = worldPlane->GetPlanarQuad().GetPositionP3();

            AddNameProperty(properties, objectId, editorName);
            AddPositionProperty(properties, objectId, position);
            properties.Vec3(objectId, "Rotation", rotation, [worldPlane, rotation = &rotation] { SetWorldPlaneRotation(worldPlane, *rotation); });
            properties.Vec3(objectId, "P0", p0, [worldPlane, p0 = &p0] { SetWorldPlanePoint(worldPlane, 0, *p0); });
            properties.Vec3(objectId, "P1", p1, [worldPlane, p1 = &p1] { SetWorldPlanePoint(worldPlane, 1, *p1); });
            properties.Vec3(objectId, "P2", p2, [worldPlane, p2 = &p2] { SetWorldPlanePoint(worldPlane, 2, *p2); });
            properties.Vec3(objectId, "P3", p3, [worldPlane, p3 = &p3] { SetWorldPlanePoint(worldPlane, 3, *p3); });
            properties.Float(objectId, "Tex Scale", createInfo.textureScale, [&] { worldPlane->SetTextureScale(createInfo.textureScale); });
            properties.Float(objectId, "Tex Offset U", createInfo.textureOffsetU, [&] { worldPlane->SetTextureOffsetU(createInfo.textureOffsetU); });
            properties.Float(objectId, "Tex Offset V", createInfo.textureOffsetV, [&] { worldPlane->SetTextureOffsetV(createInfo.textureOffsetV); });
            properties.CheckBox("Tex Rotate", createInfo.rotateTexture90, [&] { worldPlane->SetRotateTexture90(createInfo.rotateTexture90); });
            properties.Float(objectId, "Roughness Factor", createInfo.roughnessFactor, [&] { worldPlane->SetRoughnessFactor(createInfo.roughnessFactor); });
            properties.Float(objectId, "Metallic Factor", createInfo.metallicFactor, [&] { worldPlane->SetMetallicFactor(createInfo.metallicFactor); });
            properties.Render(rect);
        }

        void RenderChristmasLightsProperties(const EditorRect& rect, uint64_t objectId) {
            ChristmasLightSet* christmasLights = World::GetChristmasLightsByObjectId(objectId);
            InputElements::PropertyList properties;
            if (!christmasLights) {
                properties.Render(rect);
                return;
            }

            std::string editorName = World::GetEditorNameById(objectId);
            glm::vec3 position = World::GetPositionById(objectId);
            float spacing = christmasLights->GetCreateInfo().spacing;
            float wireRadius = christmasLights->GetCreateInfo().wireRadius;

            AddNameProperty(properties, objectId, editorName);
            AddPositionProperty(properties, objectId, position);
            properties.Float(objectId, "Spacing", spacing, [&] { christmasLights->SetSpacing(spacing); });
            properties.Float(objectId, "Wire Radius", wireRadius, [&] { christmasLights->SetWireRadius(wireRadius); });
            properties.Render(rect);
        }

        void RenderChristmasLightPointProperties(const EditorRect& rect, uint64_t objectId, int32_t pointIndex) {
            ChristmasLightSet* christmasLights = World::GetChristmasLightsByObjectId(objectId);
            InputElements::PropertyList properties;
            if (!christmasLights || pointIndex < 0 || pointIndex >= static_cast<int32_t>(christmasLights->GetCreateInfo().sequencePoints.size())) {
                properties.Render(rect);
                return;
            }

            std::vector<SequencePoint> sequencePoints = christmasLights->GetCreateInfo().sequencePoints;
            SequencePoint& sequencePoint = sequencePoints[pointIndex];
            bool changed = false;

            properties.Vec3(objectId, "Position", sequencePoint.position, [&] { changed = true; });
            if (pointIndex > 0) {
                properties.Float(objectId, "Sag", sequencePoint.customFloat, [&] { changed = true; });
            }
            properties.Render(rect);

            if (changed) {
                christmasLights->UpdateSequencePoints(sequencePoints);
                Gizmo::SetPosition(sequencePoint.position);
            }
        }

        void RenderSequencePointProperties(const EditorRect& rect, uint64_t objectId, int32_t pointIndex, PointSequences::PointHandleType handleType) {
            InputElements::PropertyList properties;
            glm::vec3 position;
            if (!PointSequences::GetPointPosition(objectId, pointIndex, handleType, position)) {
                properties.Render(rect);
                return;
            }

            properties.Vec3(objectId, "Position", position, [objectId, pointIndex, handleType, position = &position] { SetSequencePointPosition(objectId, pointIndex, handleType, *position); });
            properties.Render(rect);
        }

        void SetGizmoToSelectedPoint(uint64_t objectId) {
            glm::vec3 position;
            if (PointSequences::GetPointPosition(objectId, Selection::GetSelectedPointIndex(), Selection::GetSelectedPointHandleType(), position)) {
                Gizmo::SetPosition(position);
            }
        }

        void SetDDGIPosition(uint64_t objectId, const glm::vec3& position) {
            if (World::SetPositionById(objectId, position)) {
                SetGizmoToSelectedPoint(objectId);
            }
        }

        void SetDDGIExtents(DDGIVolume* volume, uint64_t objectId, const glm::vec3& extents) {
            volume->SetExtents(extents);
            SetGizmoToSelectedPoint(objectId);
        }

        void RenderDDGIVolumeProperties(const EditorRect& rect, uint64_t objectId) {
            DDGIVolume* volume = World::GetDDGIVolumeByObjectId(objectId);
            InputElements::PropertyList properties;
            if (!volume) {
                properties.Render(rect);
                return;
            }

            std::string editorName = World::GetEditorNameById(objectId);
            glm::vec3 position = World::GetPositionById(objectId);
            glm::vec3 extents = volume->GetExtents();
            float probeSpacing = volume->GetProbeSpacing();
            float pointCloudSpacing = volume->GetPointCloudSpacing();

            AddNameProperty(properties, objectId, editorName);
            properties.Vec3(objectId, "Position", position, [objectId, position = &position] { SetDDGIPosition(objectId, *position); });
            properties.Vec3(objectId, "Extents", extents, [volume, objectId, extents = &extents] { SetDDGIExtents(volume, objectId, *extents); });
            properties.Float(objectId, "Probe Spacing", probeSpacing, [&] { volume->SetProbeSpacing(probeSpacing); });
            properties.Float(objectId, "Point Cloud Spacing", pointCloudSpacing, [&] { volume->SetPointCloudSpacing(pointCloudSpacing); });
            properties.Render(rect);
        }

        void RenderDobermannProperties(const EditorRect& rect, uint64_t objectId) {
            std::string editorName = World::GetEditorNameById(objectId);
            glm::vec3 position = World::GetPositionById(objectId);
            float rotation = World::GetRotationById(objectId).y;
            InputElements::PropertyList properties;

            AddNameProperty(properties, objectId, editorName);
            AddPositionProperty(properties, objectId, position);
            AddYawProperty(properties, objectId, rotation);
            properties.Render(rect);
        }

        void RenderDoorProperties(const EditorRect& rect, uint64_t objectId) {
            Door* door = World::GetDoorByObjectId(objectId);
            InputElements::PropertyList properties;
            if (!door) {
                properties.Render(rect);
                return;
            }

            static const std::vector<std::string> doorTypes = {
                Hell::Enum::ToString(DoorType::STANDARD_A),
                Hell::Enum::ToString(DoorType::STANDARD_B),
                Hell::Enum::ToString(DoorType::STAINED_GLASS),
                Hell::Enum::ToString(DoorType::STAINED_GLASS2)
            };
            static const std::vector<std::string> materialTypes = {
                Hell::Enum::ToString(DoorMaterialType::RESIDENT_EVIL),
                Hell::Enum::ToString(DoorMaterialType::WHITE_PAINT)
            };

            std::string editorName = World::GetEditorNameById(objectId);
            glm::vec3 position = World::GetPositionById(objectId);
            float rotation = World::GetRotationById(objectId).y;
            std::string type = Hell::Enum::ToString(door->GetType());
            std::string frontMaterial = Hell::Enum::ToString(door->GetMaterialTypeFront());
            std::string backMaterial = Hell::Enum::ToString(door->GetMaterialTypeBack());
            std::string frameFrontMaterial = Hell::Enum::ToString(door->GetMaterialTypeFrameFront());
            std::string frameBackMaterial = Hell::Enum::ToString(door->GetMaterialTypeFrameBack());
            bool hasDeadLock = door->GetDeadLockState();
            bool deadLockedAtStart = door->GetDeadLockedAtInitState();
            bool openAtStart = door->GetOpenAtStartState();
            float maxOpenValue = door->GetCreateInfo().maxOpenValue;
            float floorPlaneTextureScale = door->GetCreateInfo().floorPlaneTextureScale;
            float floorPlaneTextureOffsetU = door->GetCreateInfo().floorPlaneTextureOffsetU;
            float floorPlaneTextureOffsetV = door->GetCreateInfo().floorPlaneTextureOffsetV;
            bool floorPlaneRotateTexture90 = door->GetCreateInfo().floorPlaneRotateTexture90;
            float floorPlaneRoughnessFactor = door->GetCreateInfo().floorPlaneRoughnessFactor;
            float floorPlaneMetallicFactor = door->GetCreateInfo().floorPlaneMetallicFactor;

            AddNameProperty(properties, objectId, editorName);
            AddPositionProperty(properties, objectId, position);
            AddYawProperty(properties, objectId, rotation);
            properties.DropDown(objectId, "Type", doorTypes, type, [&] { door->SetType(Hell::Enum::FromString(type, DoorType::UNDEFINED)); });
            properties.DropDown(objectId, "Front Material", materialTypes, frontMaterial, [&] { door->SetFrontMaterial(Hell::Enum::FromString(frontMaterial, DoorMaterialType::UNDEFINED)); });
            properties.DropDown(objectId, "Back Material", materialTypes, backMaterial, [&] { door->SetBackMaterial(Hell::Enum::FromString(backMaterial, DoorMaterialType::UNDEFINED)); });
            properties.DropDown(objectId, "Frame Front Material", materialTypes, frameFrontMaterial, [&] { door->SetFrameFrontMaterial(Hell::Enum::FromString(frameFrontMaterial, DoorMaterialType::UNDEFINED)); });
            properties.DropDown(objectId, "Frame Back Material", materialTypes, frameBackMaterial, [&] { door->SetFrameBackMaterial(Hell::Enum::FromString(frameBackMaterial, DoorMaterialType::UNDEFINED)); });
            properties.Float(objectId, "Floor Tex Scale", floorPlaneTextureScale, [&] { door->SetFloorPlaneTextureScale(floorPlaneTextureScale); });
            properties.Float(objectId, "Floor Tex Offset U", floorPlaneTextureOffsetU, [&] { door->SetFloorPlaneTextureOffsetU(floorPlaneTextureOffsetU); });
            properties.Float(objectId, "Floor Tex Offset V", floorPlaneTextureOffsetV, [&] { door->SetFloorPlaneTextureOffsetV(floorPlaneTextureOffsetV); });
            properties.CheckBox("Floor Tex Rotate", floorPlaneRotateTexture90, [&] { door->SetFloorPlaneRotateTexture90(floorPlaneRotateTexture90); });
            properties.Float(objectId, "Floor Tex Roughness Factor", floorPlaneRoughnessFactor, [&] { door->SetFloorPlaneRoughnessFactor(floorPlaneRoughnessFactor); });
            properties.Float(objectId, "Floor Tex Metallic Factor", floorPlaneMetallicFactor, [&] { door->SetFloorPlaneMetallicFactor(floorPlaneMetallicFactor); });
            properties.CheckBox("Has Deadlock", hasDeadLock, [&] { door->SetDeadLockState(hasDeadLock); });
            properties.CheckBox("Deadlocked At Start", deadLockedAtStart, [&] { door->SetDeadLockedAtInitState(deadLockedAtStart); });
            properties.CheckBox("Open At Start", openAtStart, [&] { door->SetOpenAtStartState(openAtStart); });
            properties.Float(objectId, "Max Open", maxOpenValue, [&] { door->SetMaxOpenValue(maxOpenValue); });
            properties.Render(rect);
        }

        void RenderFireplaceProperties(const EditorRect& rect, uint64_t objectId) {
            Fireplace* fireplace = World::GetFireplaceById(objectId);
            InputElements::PropertyList properties;
            if (!fireplace) {
                properties.Render(rect);
                return;
            }

            static const std::vector<std::string> fireplaceTypes = {
                Hell::Enum::ToString(FireplaceType::DEFAULT),
                Hell::Enum::ToString(FireplaceType::WOOD_STOVE)
            };

            std::string editorName = World::GetEditorNameById(objectId);
            glm::vec3 position = World::GetPositionById(objectId);
            float rotation = World::GetRotationById(objectId).y;
            std::string type = Hell::Enum::ToString(fireplace->GetCreateInfo().type);

            AddNameProperty(properties, objectId, editorName);
            AddPositionProperty(properties, objectId, position);
            AddYawProperty(properties, objectId, rotation);
            properties.DropDown(objectId, "Type", fireplaceTypes, type, [&] { fireplace->SetType(Hell::Enum::FromString(type, FireplaceType::UNDEFINED)); });
            properties.Render(rect);
        }

        void RenderGenericObjectProperties(const EditorRect& rect, uint64_t objectId) {
            GenericObject* genericObject = World::GetGenericObjectById(objectId);
            InputElements::PropertyList properties;
            if (!genericObject) {
                properties.Render(rect);
                return;
            }

            static const std::vector<std::string> genericObjectTypes = {
                Hell::Enum::ToString(GenericObjectType::CHRISTMAS_TREE),
                Hell::Enum::ToString(GenericObjectType::CHRISTMAS_PRESENT_SMALL),
                Hell::Enum::ToString(GenericObjectType::CHRISTMAS_PRESENT_LARGE),
                Hell::Enum::ToString(GenericObjectType::DRAWERS_SMALL),
                Hell::Enum::ToString(GenericObjectType::DRAWERS_LARGE),
                Hell::Enum::ToString(GenericObjectType::TOILET),
                Hell::Enum::ToString(GenericObjectType::COUCH),
                Hell::Enum::ToString(GenericObjectType::BATHROOM_BASIN),
                Hell::Enum::ToString(GenericObjectType::BATHROOM_CABINET),
                Hell::Enum::ToString(GenericObjectType::CHAIR_RE),
                Hell::Enum::ToString(GenericObjectType::CHAIR_SPINDLE_BACK),
                Hell::Enum::ToString(GenericObjectType::DEER_HEAD),
                Hell::Enum::ToString(GenericObjectType::MERMAID_ROCK),
                Hell::Enum::ToString(GenericObjectType::PLANT_BLACKBERRIES),
                Hell::Enum::ToString(GenericObjectType::PLANT_TREE),
                Hell::Enum::ToString(GenericObjectType::TEST_MODEL),
                Hell::Enum::ToString(GenericObjectType::TEST_MODEL2),
                Hell::Enum::ToString(GenericObjectType::TEST_MODEL3),
                Hell::Enum::ToString(GenericObjectType::TEST_MODEL4)
            };

            std::string editorName = World::GetEditorNameById(objectId);
            glm::vec3 position = World::GetPositionById(objectId);
            glm::vec3 rotation = World::GetRotationById(objectId);
            glm::vec3 scale = genericObject->GetScale();
            std::string type = Hell::Enum::ToString(genericObject->GetType());

            AddNameProperty(properties, objectId, editorName);
            AddPositionProperty(properties, objectId, position);
            AddEulerRotationProperty(properties, objectId, rotation);
            properties.Vec3(objectId, "Scale", scale, [&] { genericObject->SetScale(scale); });
            properties.DropDown(objectId, "Type", genericObjectTypes, type, [&] { genericObject->SetType(Hell::Enum::FromString(type, GenericObjectType::UNDEFINED)); });
            properties.Render(rect);
        }

        void RenderGenericAnimatedObjectProperties(const EditorRect& rect, uint64_t objectId) {
            GenericAnimatedObject* genericAnimatedObject = World::GetGenericAnimatedObjectById(objectId);
            InputElements::PropertyList properties;
            if (!genericAnimatedObject) {
                properties.Render(rect);
                return;
            }

            static const std::vector<std::string> genericAnimatedObjectTypes = {
                Hell::Enum::ToString(GenericAnimatedObjectType::RAT_KING)
            };

            std::string editorName = World::GetEditorNameById(objectId);
            glm::vec3 position = World::GetPositionById(objectId);
            glm::vec3 rotation = World::GetRotationById(objectId);
            float scale = genericAnimatedObject->GetScale();
            std::string type = Hell::Enum::ToString(genericAnimatedObject->GetType());
            std::string animationName = genericAnimatedObject->GetCreateInfo().animationName;
            float animationSpeed = genericAnimatedObject->GetCreateInfo().animationSpeed;

            AddNameProperty(properties, objectId, editorName);
            AddPositionProperty(properties, objectId, position);
            AddEulerRotationProperty(properties, objectId, rotation);
            properties.Float(objectId, "Scale", scale, [&] { genericAnimatedObject->SetScale(scale); });
            properties.DropDown(objectId, "Type", genericAnimatedObjectTypes, type, [&] { genericAnimatedObject->SetType(Hell::Enum::FromString(type, GenericAnimatedObjectType::UNDEFINED)); });
            properties.String(objectId, "Animation", animationName, [&] { genericAnimatedObject->SetAnimationName(animationName); });
            properties.Float(objectId, "Animation Speed", animationSpeed, [&] { genericAnimatedObject->SetAnimationSpeed(animationSpeed); });
            properties.Render(rect);
        }

        void RenderJettyProperties(const EditorRect& rect, uint64_t objectId) {
            Jetty* jetty = World::GetJettyById(objectId);
            InputElements::PropertyList properties;
            if (!jetty) {
                properties.Render(rect);
                return;
            }

            std::string editorName = World::GetEditorNameById(objectId);
            glm::vec3 position = World::GetPositionById(objectId);
            glm::vec3 rotation = World::GetRotationById(objectId);
            glm::vec3 scale = jetty->GetScale();
            uint32_t boardCount = jetty->GetBoardCount();

            AddNameProperty(properties, objectId, editorName);
            AddPositionProperty(properties, objectId, position);
            AddEulerRotationProperty(properties, objectId, rotation);
            properties.Vec3(objectId, "Scale", scale, [&] { jetty->SetScale(scale); });
            properties.UInt(objectId, "Board Count", boardCount, [&] { jetty->SetBoardCount(boardCount); });
            properties.Render(rect);
        }

        void RenderHouseLocationProperties(const EditorRect& rect, uint64_t objectId) {
            HouseLocation* houseLocation = World::GetHouseLocationByObjectId(objectId);
            InputElements::PropertyList properties;
            if (!houseLocation) {
                properties.Render(rect);
                return;
            }

            const HouseLocationCreateInfo& createInfo = houseLocation->GetCreateInfo();
            const std::vector<std::string>& houseNames = ObjectOptions::GetHouseNames();
            std::string editorName = World::GetEditorNameById(objectId);
            glm::vec3 position = World::GetPositionById(objectId);
            float rotation = createInfo.rotation;
            bool randomHouse = createInfo.randomHouse;
            std::string houseName = createInfo.houseName;

            AddNameProperty(properties, objectId, editorName);
            AddPositionProperty(properties, objectId, position);
            AddYawProperty(properties, objectId, rotation);
            properties.CheckBox("Random House", randomHouse, [&] { houseLocation->SetRandomHouse(randomHouse); });
            if (!randomHouse) {
                properties.DropDown(objectId, "House", houseNames, houseName, [&] { houseLocation->SetHouseName(houseName); });
            }
            properties.Render(rect);
        }

        void RenderLadderProperties(const EditorRect& rect, uint64_t objectId) {
            Ladder* ladder = World::GetLadderByObjectId(objectId);
            InputElements::PropertyList properties;
            if (!ladder) {
                properties.Render(rect);
                return;
            }

            std::string editorName = World::GetEditorNameById(objectId);
            glm::vec3 position = World::GetPositionById(objectId);
            float rotation = World::GetRotationById(objectId).y;

            AddNameProperty(properties, objectId, editorName);
            AddPositionProperty(properties, objectId, position);
            AddYawProperty(properties, objectId, rotation);
            properties.Render(rect);
        }

        void RenderLightProperties(const EditorRect& rect, uint64_t objectId) {
            Light* light = World::GetLightByObjectId(objectId);
            InputElements::PropertyList properties;
            if (!light) {
                properties.Render(rect);
                return;
            }

            static const std::vector<std::string> lightTypes = {
                Hell::Enum::ToString(LightType::HANGING_LIGHT),
                Hell::Enum::ToString(LightType::WALL_LAMP)
            };
            static const std::vector<std::string> iesProfileTypes = Hell::Enum::GetNames<IESProfileType>();

            std::string editorName = World::GetEditorNameById(objectId);
            glm::vec3 position = World::GetPositionById(objectId);
            glm::vec3 rotation = World::GetRotationById(objectId);
            glm::vec3 color = light->GetColor();
            glm::vec3 forward = light->GetForward();
            float radius = light->GetRadius();
            float strength = light->GetStrength();
            float iesExposure = light->GetIESExposure();
            float twist = light->GetTwist();
            std::string type = Hell::Enum::ToString(light->GetType());
            std::string iesProfileType = Hell::Enum::ToString(light->GetIESProfileType());

            AddNameProperty(properties, objectId, editorName);
            AddPositionProperty(properties, objectId, position);
            AddEulerRotationProperty(properties, objectId, rotation);
            properties.DropDown(objectId, "Type", lightTypes, type, [&] { light->SetType(Hell::Enum::FromString(type, LightType::HANGING_LIGHT)); });
            properties.Vec3(objectId, "Color", color, [&] { light->SetColor(color); });
            properties.Float(objectId, "Radius", radius, [&] { light->SetRadius(radius); });
            properties.Float(objectId, "Strength", strength, [&] { light->SetStrength(strength); });
            properties.DropDown(objectId, "IES Profile", iesProfileTypes, iesProfileType, [&] { light->SetIESProfileType(Hell::Enum::FromString(iesProfileType, IESProfileType::NONE)); });
            if (light->GetIESProfileType() != IESProfileType::NONE) {
                properties.Float(objectId, "IES Exposure", iesExposure, [&] { light->SetIESExposure(iesExposure); });
                properties.Vec3(objectId, "Forward", forward, [&] { light->SetForward(forward); });
                properties.Float(objectId, "Twist", twist, [&] { light->SetTwist(twist); });
            }
            properties.Render(rect);
        }

        void RenderMermaidProperties(const EditorRect& rect, uint64_t objectId) {
            std::string editorName = World::GetEditorNameById(objectId);
            glm::vec3 position = World::GetPositionById(objectId);
            float rotation = World::GetRotationById(objectId).y;
            InputElements::PropertyList properties;

            AddNameProperty(properties, objectId, editorName);
            AddPositionProperty(properties, objectId, position);
            AddYawProperty(properties, objectId, rotation);
            properties.Render(rect);
        }

        void RenderPickUpProperties(const EditorRect& rect, uint64_t objectId) {
            PickUp* pickUp = World::GetPickUpByObjectId(objectId);
            InputElements::PropertyList properties;
            if (!pickUp) {
                properties.Render(rect);
                return;
            }

            std::string editorName = World::GetEditorNameById(objectId);
            glm::vec3 position = World::GetPositionById(objectId);
            glm::vec3 rotation = World::GetRotationById(objectId);
            bool respawn = pickUp->GetRespawnState();
            bool disablePhysicsAtSpawn = pickUp->GetDisabledPhysicsAtSpawnState();

            AddNameProperty(properties, objectId, editorName);
            AddPositionProperty(properties, objectId, position);
            AddEulerRotationProperty(properties, objectId, rotation);
            properties.CheckBox("Respawn", respawn, [&] { pickUp->SetRespawnState(respawn); });
            properties.CheckBox("Starts Frozen", disablePhysicsAtSpawn, [&] { pickUp->SetDisabledPhysicsAtSpawnState(disablePhysicsAtSpawn); });
            properties.Render(rect);
        }

        void RenderPictureFrameProperties(const EditorRect& rect, uint64_t objectId) {
            PictureFrame* pictureFrame = World::GetPictureFrameByObjectId(objectId);
            InputElements::PropertyList properties;
            if (!pictureFrame) {
                properties.Render(rect);
                return;
            }

            static const std::vector<std::string> pictureFrameTypes = {
                Hell::Enum::ToString(PictureFrameType::BIG_LANDSCAPE),
                Hell::Enum::ToString(PictureFrameType::TALL_THIN),
                Hell::Enum::ToString(PictureFrameType::REGULAR_PORTRAIT),
                Hell::Enum::ToString(PictureFrameType::REGULAR_LANDSCAPE)
            };

            std::string editorName = World::GetEditorNameById(objectId);
            glm::vec3 position = World::GetPositionById(objectId);
            glm::vec3 rotation = World::GetRotationById(objectId);
            glm::vec3 scale = pictureFrame->GetScale();
            std::string type = Hell::Enum::ToString(pictureFrame->GetType());
            bool useRandom = pictureFrame->GetCreateInfo().useRandom;
            std::string materialName = pictureFrame->GetCreateInfo().materialName;
            const std::vector<std::string> materialNames = HouseBuilder::GetLargePictureFrameMaterialNames();

            AddNameProperty(properties, objectId, editorName);
            AddPositionProperty(properties, objectId, position);
            AddEulerRotationProperty(properties, objectId, rotation);
            properties.Vec3(objectId, "Scale", scale, [&] { pictureFrame->SetScale(scale); });
            properties.DropDown(objectId, "Type", pictureFrameTypes, type, [&] { pictureFrame->SetType(Hell::Enum::FromString(type, PictureFrameType::UNDEFINED)); });
            if (pictureFrame->GetType() == PictureFrameType::BIG_LANDSCAPE) {
                properties.CheckBox("Random Material", useRandom, [&] { pictureFrame->SetUseRandom(useRandom); });
                if (!useRandom) {
                    properties.DropDown(objectId, "Material", materialNames, materialName, [&] { pictureFrame->SetMaterialName(materialName); });
                }
            }
            properties.Render(rect);
        }

        void RenderPianoProperties(const EditorRect& rect, uint64_t objectId) {
            std::string editorName = World::GetEditorNameById(objectId);
            glm::vec3 position = World::GetPositionById(objectId);
            float rotation = World::GetRotationById(objectId).y;
            InputElements::PropertyList properties;

            AddNameProperty(properties, objectId, editorName);
            AddPositionProperty(properties, objectId, position);
            AddYawProperty(properties, objectId, rotation);
            properties.Render(rect);
        }

        void RenderSpawnPointProperties(const EditorRect& rect, uint64_t objectId) {
            const ObjectType objectType = GetObjectIdType(objectId);
            SpawnPoint* spawnPoint = objectType == ObjectType::SPAWN_POINT_CAMPAIGN ? World::GetSpawnPointCampaignByObjectId(objectId) : World::GetSpawnPointDeathMatchByObjectId(objectId);
            if (!spawnPoint) return;

            std::string editorName = World::GetEditorNameById(objectId);
            glm::vec3 position = spawnPoint->GetPosition();
            glm::vec2 rotation = glm::vec2(spawnPoint->GetRotation());
            InputElements::PropertyList properties;

            AddNameProperty(properties, objectId, editorName);
            AddPositionProperty(properties, objectId, position);
            properties.Vec2(objectId, "Rotation", rotation, [objectId, rotation = &rotation] { SetSpawnPointRotation(objectId, *rotation); });
            properties.Button("Set from player camera", [objectId] { SetSpawnPointFromPlayerCamera(objectId); });
            properties.Button("Test Spawn", [spawnPoint] { TestSpawnPoint(spawnPoint); });
            properties.Render(rect);
        }

        void RenderStaircaseProperties(const EditorRect& rect, uint64_t objectId) {
            Staircase* staircase = World::GetStaircaseByObjectId(objectId);
            InputElements::PropertyList properties;
            if (!staircase) {
                properties.Render(rect);
                return;
            }

            std::string editorName = World::GetEditorNameById(objectId);
            glm::vec3 position = World::GetPositionById(objectId);
            float rotation = World::GetRotationById(objectId).y;
            glm::vec3 scale = staircase->GetScale();
            uint32_t stepCount = staircase->GetStepCount();

            AddNameProperty(properties, objectId, editorName);
            AddPositionProperty(properties, objectId, position);
            AddYawProperty(properties, objectId, rotation);
            properties.Vec3(objectId, "Scale", scale, [&] { staircase->SetScale(scale); });
            properties.UInt(objectId, "Step Count", stepCount, [&] { staircase->SetStepCount(stepCount); });
            properties.Render(rect);
        }

        void RenderWallProperties(const EditorRect& rect, uint64_t objectId) {
            Wall* wall = World::GetWallByObjectId(objectId);
            InputElements::PropertyList properties;
            if (!wall) {
                properties.Render(rect);
                return;
            }

            static const std::vector<std::string> wallTypes = {
                Hell::Enum::ToString(WallType::INTERIOR),
                Hell::Enum::ToString(WallType::WEATHER_BOARDS)
            };

            const WallCreateInfo& createInfo = wall->GetCreateInfo();
            const std::vector<std::string>& interiorMaterials = ObjectOptions::GetInteriorMaterials();
            const std::vector<std::string>& weatherBoardMaterials = ObjectOptions::GetWeatherBoardMaterials();
            std::string editorName = World::GetEditorNameById(objectId);
            glm::vec3 position = World::GetPositionById(objectId);
            std::string materialName = createInfo.materialName;
            std::string wallType = Hell::Enum::ToString(createInfo.wallType);
            float textureScale = createInfo.textureScale;
            float textureOffsetU = createInfo.textureOffsetU;
            float textureOffsetV = createInfo.textureOffsetV;
            float roughnessFactor = createInfo.roughnessFactor;
            float metallicFactor = createInfo.metallicFactor;
            std::string weatherBoardStopMaterialName = createInfo.weatherBoardStopMaterialName;
            uint32_t weatherBoardTextureBoardCount = createInfo.weatherBoardTextureBoardCount;
            uint32_t weatherBoardStartIndex = createInfo.weatherBoardStartIndex;
            uint32_t weatherBoardEndIndex = createInfo.weatherBoardEndIndex;

            AddNameProperty(properties, objectId, editorName);
            AddPositionProperty(properties, objectId, position);
            properties.DropDown(objectId, "Type", wallTypes, wallType, [wall, wallType = &wallType, materialName = &materialName] { SetWallType(wall, *wallType, *materialName); });

            if (createInfo.wallType == WallType::WEATHER_BOARDS) {
                properties.UInt(objectId, "Texture Boards", weatherBoardTextureBoardCount, [&] { wall->SetWeatherBoardTextureBoardCount(weatherBoardTextureBoardCount); });
                properties.UInt(objectId, "Start Index", weatherBoardStartIndex, [&] { wall->SetWeatherBoardStartIndex(weatherBoardStartIndex); });
                properties.UInt(objectId, "End Index", weatherBoardEndIndex, [&] { wall->SetWeatherBoardEndIndex(weatherBoardEndIndex); });
                properties.Float(objectId, "Tex Offset U", textureOffsetU, [&] { wall->SetTextureOffsetU(textureOffsetU); });
                properties.Float(objectId, "Tex Offset V", textureOffsetV, [&] { wall->SetTextureOffsetV(textureOffsetV); });
                properties.DropDown(objectId, "Stop Material", weatherBoardMaterials, weatherBoardStopMaterialName, [&] { wall->SetWeatherBoardStopMaterial(weatherBoardStopMaterialName); });
            }
            else {
                properties.Float(objectId, "Tex Scale", textureScale, [&] { wall->SetTextureScale(textureScale); });
                properties.Float(objectId, "Tex Offset U", textureOffsetU, [&] { wall->SetTextureOffsetU(textureOffsetU); });
                properties.Float(objectId, "Tex Offset V", textureOffsetV, [&] { wall->SetTextureOffsetV(textureOffsetV); });
            }
            properties.Float(objectId, "Roughness Factor", roughnessFactor, [&] { wall->SetRoughnessFactor(roughnessFactor); });
            properties.Float(objectId, "Metallic Factor", metallicFactor, [&] { wall->SetMetallicFactor(metallicFactor); });
            properties.Render(rect);
        }

        void RenderWallSegmentProperties(const EditorRect& rect, uint64_t objectId, int32_t segmentIndex) {
            Wall* wall = World::GetWallByObjectId(objectId);
            InputElements::PropertyList properties;
            if (!wall || segmentIndex < 0 || segmentIndex + 1 >= static_cast<int32_t>(wall->GetCreateInfo().sequencePoints.size())) {
                properties.Render(rect);
                return;
            }

            const int32_t startIndex = wall->GetCreateInfo().useReversePointOrder ? segmentIndex + 1 : segmentIndex;
            const int32_t endIndex = wall->GetCreateInfo().useReversePointOrder ? segmentIndex : segmentIndex + 1;
            glm::vec3 startPosition = wall->GetPointByIndex(startIndex);
            glm::vec3 endPosition = wall->GetPointByIndex(endIndex);
            float startHeight = wall->GetPointHeightByIndex(startIndex);
            float endHeight = wall->GetPointHeightByIndex(endIndex);
            bool weatherBoardStop = wall->GetCreateInfo().sequencePoints[segmentIndex].customBool;

            properties.Vec3(objectId, "Start", startPosition, [wall, startIndex, startPosition = &startPosition, endPosition = &endPosition] { SetWallSegmentPoint(wall, startIndex, *startPosition, *endPosition); });
            properties.Float(objectId, "Start Height", startHeight, [&] { wall->SetPointHeight(startIndex, startHeight); });
            properties.Vec3(objectId, "End", endPosition, [wall, endIndex, startPosition = &startPosition, endPosition = &endPosition] { SetWallSegmentPoint(wall, endIndex, *endPosition, *startPosition); });
            properties.Float(objectId, "End Height", endHeight, [&] { wall->SetPointHeight(endIndex, endHeight); });
            if (wall->IsWeatherBoards()) {
                properties.CheckBox("Weatherboard Stop", weatherBoardStop, [&] { wall->SetPointCustomBool(segmentIndex, weatherBoardStop); });
            }
            properties.Render(rect);
        }

        void RenderWindowProperties(const EditorRect& rect, uint64_t objectId) {
            std::string editorName = World::GetEditorNameById(objectId);
            glm::vec3 position = World::GetPositionById(objectId);
            float rotation = World::GetRotationById(objectId).y;
            InputElements::PropertyList properties;

            AddNameProperty(properties, objectId, editorName);
            AddPositionProperty(properties, objectId, position);
            AddYawProperty(properties, objectId, rotation);
            properties.Render(rect);
        }

        void RenderNameOnlyProperties(const EditorRect& rect, uint64_t objectId) {
            std::string editorName = World::GetEditorNameById(objectId);
            InputElements::PropertyList properties;
            AddNameProperty(properties, objectId, editorName);
            properties.Render(rect);
        }

        void RenderPositionOnlyProperties(const EditorRect& rect, uint64_t objectId) {
            std::string editorName = World::GetEditorNameById(objectId);
            glm::vec3 position = World::GetPositionById(objectId);
            InputElements::PropertyList properties;

            AddNameProperty(properties, objectId, editorName);
            AddPositionProperty(properties, objectId, position);
            properties.Render(rect);
        }

        void RenderDefaultProperties(const EditorRect& rect, uint64_t objectId) {
            std::string editorName = World::GetEditorNameById(objectId);
            glm::vec3 position = World::GetPositionById(objectId);
            glm::vec3 rotation = World::GetRotationById(objectId);
            static glm::vec2 dummyVec2(0.0f);
            static float dummyFloat = 0.0f;
            InputElements::PropertyList properties;

            AddNameProperty(properties, objectId, editorName);
            AddPositionProperty(properties, objectId, position);
            AddEulerRotationProperty(properties, objectId, rotation);

            // Delete these when Vec2 and float have real properties
            properties.Vec2(objectId, "Vec2", dummyVec2);
            properties.Float(objectId, "Float", dummyFloat);
            properties.Render(rect);
        }
    }

    void RenderProperties(const EditorRect& rect) {
        if (!Workspace::HasMode()) {
            InputElements::PropertyList properties;
            properties.Render(rect);
            return;
        }

        if (Selection::HasWorkspaceSelection()) {
            RenderWorkspaceProperties(rect);
            return;
        }

        if (!Selection::HasSelection()) {
            InputElements::PropertyList properties;
            properties.Render(rect);
            return;
        }

        const uint64_t objectId = Selection::GetSelectedObjectId();
        const ObjectType objectType = GetObjectIdType(objectId);

        if (objectType == ObjectType::DDGI_VOLUME) {
            RenderDDGIVolumeProperties(rect, objectId);
            return;
        }

        if (objectType == ObjectType::PLANAR_QUAD_OBJECT) {
            Internal::RenderPlanarQuadProperties(rect, objectId);
            return;
        }

        if (objectType == ObjectType::POINT_PAIR_OBJECT) {
            Internal::RenderPointPairProperties(rect, objectId);
            return;
        }

        if (Selection::HasSelectedPoint()) {
            if (objectType == ObjectType::CHRISTMAS_LIGHTS) {
                RenderChristmasLightPointProperties(rect, objectId, Selection::GetSelectedPointIndex());
            }
            else {
                RenderSequencePointProperties(rect, objectId, Selection::GetSelectedPointIndex(), Selection::GetSelectedPointHandleType());
            }
            return;
        }

        if (Selection::HasSelectedWallSegment()) {
            RenderWallSegmentProperties(rect, objectId, Selection::GetSelectedWallSegmentIndex());
            return;
        }

        switch (objectType) {
            case ObjectType::CHRISTMAS_LIGHTS:        RenderChristmasLightsProperties(rect, objectId);       break;
            case ObjectType::DOBERMANN:               RenderDobermannProperties(rect, objectId);             break;
            case ObjectType::DOOR:                    RenderDoorProperties(rect, objectId);                  break;
            case ObjectType::FIREPLACE:               RenderFireplaceProperties(rect, objectId);             break;
            case ObjectType::GENERIC_ANIMATED_OBJECT: RenderGenericAnimatedObjectProperties(rect, objectId); break;
            case ObjectType::GENERIC_OBJECT:          RenderGenericObjectProperties(rect, objectId);         break;
            case ObjectType::HOUSE_LOCATION:          RenderHouseLocationProperties(rect, objectId);         break;
            case ObjectType::JETTY:                   RenderJettyProperties(rect, objectId);                 break;
            case ObjectType::LADDER:                  RenderLadderProperties(rect, objectId);                break;
            case ObjectType::LIGHT:                   RenderLightProperties(rect, objectId);                 break;
            case ObjectType::MERMAID:                 RenderMermaidProperties(rect, objectId);               break;
            case ObjectType::PIANO:                   RenderPianoProperties(rect, objectId);                 break;
            case ObjectType::PICK_UP:                 RenderPickUpProperties(rect, objectId);                break;
            case ObjectType::PICTURE_FRAME:           RenderPictureFrameProperties(rect, objectId);          break;
            case ObjectType::SPAWN_POINT_CAMPAIGN:    RenderSpawnPointProperties(rect, objectId);            break;
            case ObjectType::SPAWN_POINT_DEATHMATCH:  RenderSpawnPointProperties(rect, objectId);            break;
            case ObjectType::STAIRCASE:               RenderStaircaseProperties(rect, objectId);             break;
            case ObjectType::WALL:                    RenderWallProperties(rect, objectId);                  break;
            case ObjectType::WINDOW:                  RenderWindowProperties(rect, objectId);                break;
            case ObjectType::WORLD_PLANE:             RenderWorldPlaneProperties(rect, objectId);            break;
            case ObjectType::ANIMATED_GAME_OBJECT:    RenderPositionOnlyProperties(rect, objectId);          break;
            case ObjectType::FENCE:                   RenderPositionOnlyProperties(rect, objectId);          break;
            case ObjectType::POWER_POLE_SET:          RenderPositionOnlyProperties(rect, objectId);          break;
            case ObjectType::SHARK:                   RenderPositionOnlyProperties(rect, objectId);          break;
            default:                                  RenderDefaultProperties(rect, objectId);               break;
        }
    }

    bool HasTools() {
        if (!Workspace::HasMode()) return false;
        return Workspace::GetMode() == EditorSessionMode::MAP && MapTools::GetMode() == MapTools::Mode::HEIGHT_MAP;
    }

    void RenderTools(const EditorRect& rect) {
        if (!HasTools()) return;

        constexpr uint64_t TOOLS_PROPERTY_ID = UINT64_MAX - 1;
        float brushSize = HeightMapEditor::GetBrushSize();
        float brushStrength = HeightMapEditor::GetBrushStrength();
        float targetHeight = HeightMapEditor::GetTargetHeight();
        float brushRotation = HeightMapEditor::GetBrushRotation();
        float brushGamma = HeightMapEditor::GetBrushGamma();

        InputElements::PropertyList properties;
        properties.FloatSlider(TOOLS_PROPERTY_ID, "Brush Size", brushSize, 0.5f, 32.0f, [&] { HeightMapEditor::SetBrushSize(brushSize); });
        properties.FloatSlider(TOOLS_PROPERTY_ID, "Strength", brushStrength, 1.0f, 100.0f, [&] { HeightMapEditor::SetBrushStrength(brushStrength); });
        properties.FloatSlider(TOOLS_PROPERTY_ID, "Target Height", targetHeight, 0.0f, 40.0f, [&] { HeightMapEditor::SetTargetHeight(targetHeight); });
        properties.FloatSlider(TOOLS_PROPERTY_ID, "Rotation", brushRotation, -180.0f, 180.0f, [&] { HeightMapEditor::SetBrushRotation(brushRotation); });
        properties.FloatSlider(TOOLS_PROPERTY_ID, "Gamma", brushGamma, 0.1f, 2.0f, [&] { HeightMapEditor::SetBrushGamma(brushGamma); });
        properties.Render(rect);
    }
}
