#include "EditorObjectProperties.h"

#include "EditorInputElements.h"
#include "EditorSelection.h"

#include "Hell/Common/Enum.h"
#include "Hell/ResourceManagement/ResourceManager.h"

#include "Unloved/Editor/Gizmo.h"
#include "Unloved/ObjectId.h"
#include "Unloved/Objects/Exterior/Jetty.h"
#include "Unloved/Objects/House/Door.h"
#include "Unloved/Objects/House/Fireplace.h"
#include "Unloved/Objects/House/Wall.h"
#include "Unloved/Objects/House/WorldPlane.h"
#include "Unloved/Objects/Interior/PictureFrame.h"
#include "Unloved/Objects/Lighting/Light.h"
#include "Unloved/Objects/Props/GenericObject.h"
#include "Unloved/Objects/Props/Christmas/ChristmasLights.h"
#include "Unloved/Objects/Props/PickUp.h"
#include "Unloved/Objects/Traversal/Ladder.h"
#include "Unloved/Objects/Traversal/Staircase.h"
#include "Unloved/Systems/DDGI/DDGIVolume.h"
#include "Unloved/World/World.h"

#include <string>
#include <vector>

namespace Unloved::EditorSession::ObjectProperties {
    namespace {
        void RenderWorldPlaneProperties(const EditorRect& rect, uint64_t objectId) {
            WorldPlane* worldPlane = World::GetWorldPlaneByObjectId(objectId);
            InputElements::PropertyList properties;
            if (!worldPlane) {
                properties.Render(rect);
                return;
            }

            static const std::vector<std::string> materialNames = Hell::ResourceManager::GetMaterialNames();
            WorldPlaneCreateInfo& createInfo = worldPlane->GetCreateInfo();
            std::string editorName = World::GetEditorNameById(objectId);
            glm::vec3 position = World::GetPositionById(objectId);
            bool pointsChanged = false;

            properties.String(objectId, "Name", editorName, [&] { World::SetEditorNameById(objectId, editorName); });
            properties.Vec3(objectId, "Position", position, [&] { if (World::SetPositionById(objectId, position)) Gizmo::SetPosition(position); });
            properties.DropDown(objectId, "Material", materialNames, createInfo.materialName, [&] { worldPlane->SetMaterial(createInfo.materialName); });
            properties.Vec3(objectId, "P0", createInfo.p0, [&] { pointsChanged = true; });
            properties.Vec3(objectId, "P1", createInfo.p1, [&] { pointsChanged = true; });
            properties.Vec3(objectId, "P2", createInfo.p2, [&] { pointsChanged = true; });
            properties.Vec3(objectId, "P3", createInfo.p3, [&] { pointsChanged = true; });
            properties.Float(objectId, "Tex Scale", createInfo.textureScale, [&] { worldPlane->SetTextureScale(createInfo.textureScale); });
            properties.Float(objectId, "Tex Offset U", createInfo.textureOffsetU, [&] { worldPlane->SetTextureOffsetU(createInfo.textureOffsetU); });
            properties.Float(objectId, "Tex Offset V", createInfo.textureOffsetV, [&] { worldPlane->SetTextureOffsetV(createInfo.textureOffsetV); });
            properties.Render(rect);

            if (pointsChanged) worldPlane->UpdateVertexDataFromCreateInfo();
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

            properties.String(objectId, "Name", editorName, [&] { World::SetEditorNameById(objectId, editorName); });
            properties.Vec3(objectId, "Position", position, [&] { if (World::SetPositionById(objectId, position)) Gizmo::SetPosition(position); });
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
            if (pointIndex > 0) properties.Float(objectId, "Sag", sequencePoint.value, [&] { changed = true; });
            properties.Render(rect);

            if (changed) {
                christmasLights->UpdateSequencePoints(sequencePoints);
                Gizmo::SetPosition(sequencePoint.position);
            }
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

            properties.String(objectId, "Name", editorName, [&] { World::SetEditorNameById(objectId, editorName); });
            properties.Vec3(objectId, "Position", position, [&] { if (World::SetPositionById(objectId, position)) Gizmo::SetPosition(position); });
            properties.Vec3(objectId, "Extents", extents, [&] { volume->SetExtents(extents); });
            properties.Float(objectId, "Probe Spacing", probeSpacing, [&] { volume->SetProbeSpacing(probeSpacing); });
            properties.Float(objectId, "Point Cloud Spacing", pointCloudSpacing, [&] { volume->SetPointCloudSpacing(pointCloudSpacing); });
            properties.Render(rect);
        }

        void RenderDobermannProperties(const EditorRect& rect, uint64_t objectId) {
            std::string editorName = World::GetEditorNameById(objectId);
            glm::vec3 position = World::GetPositionById(objectId);
            float rotation = World::GetRotationById(objectId).y;
            InputElements::PropertyList properties;

            properties.String(objectId, "Name", editorName, [&] { World::SetEditorNameById(objectId, editorName); });
            properties.Vec3(objectId, "Position", position, [&] { if (World::SetPositionById(objectId, position)) Gizmo::SetPosition(position); });
            properties.Float(objectId, "Rotation", rotation, [&] { const glm::vec3 fullRotation = glm::vec3(0.0f, rotation, 0.0f); if (World::SetRotationById(objectId, fullRotation)) Gizmo::SetRotation(fullRotation); });
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

            properties.String(objectId, "Name", editorName, [&] { World::SetEditorNameById(objectId, editorName); });
            properties.Vec3(objectId, "Position", position, [&] { if (World::SetPositionById(objectId, position)) Gizmo::SetPosition(position); });
            properties.Float(objectId, "Rotation", rotation, [&] { const glm::vec3 fullRotation = glm::vec3(0.0f, rotation, 0.0f); if (World::SetRotationById(objectId, fullRotation)) Gizmo::SetRotation(fullRotation); });
            properties.DropDown(objectId, "Type", doorTypes, type, [&] { door->SetType(Hell::Enum::FromString(type, DoorType::UNDEFINED)); });
            properties.DropDown(objectId, "Front Material", materialTypes, frontMaterial, [&] { door->SetFrontMaterial(Hell::Enum::FromString(frontMaterial, DoorMaterialType::UNDEFINED)); });
            properties.DropDown(objectId, "Back Material", materialTypes, backMaterial, [&] { door->SetBackMaterial(Hell::Enum::FromString(backMaterial, DoorMaterialType::UNDEFINED)); });
            properties.DropDown(objectId, "Frame Front Material", materialTypes, frameFrontMaterial, [&] { door->SetFrameFrontMaterial(Hell::Enum::FromString(frameFrontMaterial, DoorMaterialType::UNDEFINED)); });
            properties.DropDown(objectId, "Frame Back Material", materialTypes, frameBackMaterial, [&] { door->SetFrameBackMaterial(Hell::Enum::FromString(frameBackMaterial, DoorMaterialType::UNDEFINED)); });
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

            properties.String(objectId, "Name", editorName, [&] { World::SetEditorNameById(objectId, editorName); });
            properties.Vec3(objectId, "Position", position, [&] { if (World::SetPositionById(objectId, position)) Gizmo::SetPosition(position); });
            properties.Float(objectId, "Rotation", rotation, [&] { const glm::vec3 fullRotation = glm::vec3(0.0f, rotation, 0.0f); if (World::SetRotationById(objectId, fullRotation)) Gizmo::SetRotation(fullRotation); });
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

            properties.String(objectId, "Name", editorName, [&] { World::SetEditorNameById(objectId, editorName); });
            properties.Vec3(objectId, "Position", position, [&] { if (World::SetPositionById(objectId, position)) Gizmo::SetPosition(position); });
            properties.Vec3(objectId, "Rotation", rotation, [&] { if (World::SetRotationById(objectId, rotation)) Gizmo::SetRotation(rotation); });
            properties.Vec3(objectId, "Scale", scale, [&] { genericObject->SetScale(scale); });
            properties.DropDown(objectId, "Type", genericObjectTypes, type, [&] { genericObject->SetType(Hell::Enum::FromString(type, GenericObjectType::UNDEFINED)); });
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

            properties.String(objectId, "Name", editorName, [&] { World::SetEditorNameById(objectId, editorName); });
            properties.Vec3(objectId, "Position", position, [&] { if (World::SetPositionById(objectId, position)) Gizmo::SetPosition(position); });
            properties.Vec3(objectId, "Rotation", rotation, [&] { if (World::SetRotationById(objectId, rotation)) Gizmo::SetRotation(rotation); });
            properties.Vec3(objectId, "Scale", scale, [&] { jetty->SetScale(scale); });
            properties.UInt(objectId, "Board Count", boardCount, [&] { jetty->SetBoardCount(boardCount); });
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

            properties.String(objectId, "Name", editorName, [&] { World::SetEditorNameById(objectId, editorName); });
            properties.Vec3(objectId, "Position", position, [&] { if (World::SetPositionById(objectId, position)) Gizmo::SetPosition(position); });
            properties.Float(objectId, "Rotation", rotation, [&] { const glm::vec3 fullRotation = glm::vec3(0.0f, rotation, 0.0f); if (World::SetRotationById(objectId, fullRotation)) Gizmo::SetRotation(fullRotation); });
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

            properties.String(objectId, "Name", editorName, [&] { World::SetEditorNameById(objectId, editorName); });
            properties.Vec3(objectId, "Position", position, [&] { if (World::SetPositionById(objectId, position)) Gizmo::SetPosition(position); });
            properties.Vec3(objectId, "Rotation", rotation, [&] { if (World::SetRotationById(objectId, rotation)) Gizmo::SetRotation(rotation); });
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

            properties.String(objectId, "Name", editorName, [&] { World::SetEditorNameById(objectId, editorName); });
            properties.Vec3(objectId, "Position", position, [&] { if (World::SetPositionById(objectId, position)) Gizmo::SetPosition(position); });
            properties.Float(objectId, "Rotation", rotation, [&] { const glm::vec3 fullRotation = glm::vec3(0.0f, rotation, 0.0f); if (World::SetRotationById(objectId, fullRotation)) Gizmo::SetRotation(fullRotation); });
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

            properties.String(objectId, "Name", editorName, [&] { World::SetEditorNameById(objectId, editorName); });
            properties.Vec3(objectId, "Position", position, [&] { if (World::SetPositionById(objectId, position)) Gizmo::SetPosition(position); });
            properties.Vec3(objectId, "Rotation", rotation, [&] { if (World::SetRotationById(objectId, rotation)) Gizmo::SetRotation(rotation); });
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

            std::string editorName = World::GetEditorNameById(objectId);
            glm::vec3 position = World::GetPositionById(objectId);
            glm::vec3 rotation = World::GetRotationById(objectId);
            glm::vec3 scale = pictureFrame->GetScale();

            properties.String(objectId, "Name", editorName, [&] { World::SetEditorNameById(objectId, editorName); });
            properties.Vec3(objectId, "Position", position, [&] { if (World::SetPositionById(objectId, position)) Gizmo::SetPosition(position); });
            properties.Vec3(objectId, "Rotation", rotation, [&] { if (World::SetRotationById(objectId, rotation)) Gizmo::SetRotation(rotation); });
            properties.Vec3(objectId, "Scale", scale, [&] { pictureFrame->SetScale(scale); });
            properties.Render(rect);
        }

        void RenderPianoProperties(const EditorRect& rect, uint64_t objectId) {
            std::string editorName = World::GetEditorNameById(objectId);
            glm::vec3 position = World::GetPositionById(objectId);
            float rotation = World::GetRotationById(objectId).y;
            InputElements::PropertyList properties;

            properties.String(objectId, "Name", editorName, [&] { World::SetEditorNameById(objectId, editorName); });
            properties.Vec3(objectId, "Position", position, [&] { if (World::SetPositionById(objectId, position)) Gizmo::SetPosition(position); });
            properties.Float(objectId, "Rotation", rotation, [&] { const glm::vec3 fullRotation = glm::vec3(0.0f, rotation, 0.0f); if (World::SetRotationById(objectId, fullRotation)) Gizmo::SetRotation(fullRotation); });
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

            properties.String(objectId, "Name", editorName, [&] { World::SetEditorNameById(objectId, editorName); });
            properties.Vec3(objectId, "Position", position, [&] { if (World::SetPositionById(objectId, position)) Gizmo::SetPosition(position); });
            properties.Float(objectId, "Rotation", rotation, [&] { const glm::vec3 fullRotation = glm::vec3(0.0f, rotation, 0.0f); if (World::SetRotationById(objectId, fullRotation)) Gizmo::SetRotation(fullRotation); });
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

            static const std::vector<std::string> materialNames = Hell::ResourceManager::GetMaterialNames();
            static const std::vector<std::string> wallTypes = {
                Hell::Enum::ToString(WallType::INTERIOR),
                Hell::Enum::ToString(WallType::WEATHER_BOARDS)
            };

            const WallCreateInfo& createInfo = wall->GetCreateInfo();
            std::string editorName = World::GetEditorNameById(objectId);
            glm::vec3 position = World::GetPositionById(objectId);
            std::vector<glm::vec3> points = createInfo.points;
            std::string materialName = createInfo.materialName;
            std::string wallType = Hell::Enum::ToString(createInfo.wallType);
            float height = createInfo.height;
            float textureScale = createInfo.textureScale;
            float textureOffsetU = createInfo.textureOffsetU;
            float textureOffsetV = createInfo.textureOffsetV;

            properties.String(objectId, "Name", editorName, [&] { World::SetEditorNameById(objectId, editorName); });
            properties.Vec3(objectId, "Position", position, [&] { if (World::SetPositionById(objectId, position)) Gizmo::SetPosition(position); });
            for (size_t i = 0; i < points.size(); i++) {
                const std::string pointLabel = "P" + std::to_string(i);
                properties.Vec3(objectId, pointLabel.c_str(), points[i], [&, i] { if (wall->UpdatePointPosition(static_cast<int>(i), points[i])) Gizmo::SetPosition(wall->GetWorldSpaceCenter()); });
            }
            properties.DropDown(objectId, "Material", materialNames, materialName, [&] { wall->SetMaterial(materialName); });
            properties.DropDown(objectId, "Type", wallTypes, wallType, [&] { wall->SetWallType(Hell::Enum::FromString(wallType, WallType::UNDEFINED)); });
            properties.Float(objectId, "Height", height, [&] { wall->SetHeight(height); });
            properties.Float(objectId, "Tex Scale", textureScale, [&] { wall->SetTextureScale(textureScale); });
            properties.Float(objectId, "Tex Offset U", textureOffsetU, [&] { wall->SetTextureOffsetU(textureOffsetU); });
            properties.Float(objectId, "Tex Offset V", textureOffsetV, [&] { wall->SetTextureOffsetV(textureOffsetV); });
            properties.Render(rect);
        }

        void RenderWindowProperties(const EditorRect& rect, uint64_t objectId) {
            std::string editorName = World::GetEditorNameById(objectId);
            glm::vec3 position = World::GetPositionById(objectId);
            float rotation = World::GetRotationById(objectId).y;
            InputElements::PropertyList properties;

            properties.String(objectId, "Name", editorName, [&] { World::SetEditorNameById(objectId, editorName); });
            properties.Vec3(objectId, "Position", position, [&] { if (World::SetPositionById(objectId, position)) Gizmo::SetPosition(position); });
            properties.Float(objectId, "Rotation", rotation, [&] { const glm::vec3 fullRotation = glm::vec3(0.0f, rotation, 0.0f); if (World::SetRotationById(objectId, fullRotation)) Gizmo::SetRotation(fullRotation); });
            properties.Render(rect);
        }

        void RenderNameOnlyProperties(const EditorRect& rect, uint64_t objectId) {
            std::string editorName = World::GetEditorNameById(objectId);
            InputElements::PropertyList properties;
            properties.String(objectId, "Name", editorName, [&] { World::SetEditorNameById(objectId, editorName); });
            properties.Render(rect);
        }

        void RenderPositionOnlyProperties(const EditorRect& rect, uint64_t objectId) {
            std::string editorName = World::GetEditorNameById(objectId);
            glm::vec3 position = World::GetPositionById(objectId);
            InputElements::PropertyList properties;

            properties.String(objectId, "Name", editorName, [&] { World::SetEditorNameById(objectId, editorName); });
            properties.Vec3(objectId, "Position", position, [&] { if (World::SetPositionById(objectId, position)) Gizmo::SetPosition(position); });
            properties.Render(rect);
        }

        void RenderDefaultProperties(const EditorRect& rect, uint64_t objectId) {
            std::string editorName = World::GetEditorNameById(objectId);
            glm::vec3 position = World::GetPositionById(objectId);
            glm::vec3 rotation = World::GetRotationById(objectId);
            static glm::vec2 dummyVec2(0.0f);
            static float dummyFloat = 0.0f;
            InputElements::PropertyList properties;

            properties.String(objectId, "Name", editorName, [&] { World::SetEditorNameById(objectId, editorName); });
            properties.Vec3(objectId, "Position", position, [&] { if (World::SetPositionById(objectId, position)) Gizmo::SetPosition(position); });
            properties.Vec3(objectId, "Rotation", rotation, [&] { if (World::SetRotationById(objectId, rotation)) Gizmo::SetRotation(rotation); });

            // Delete these when Vec2 and float have real properties
            properties.Vec2(objectId, "Vec2", dummyVec2);
            properties.Float(objectId, "Float", dummyFloat);
            properties.Render(rect);
        }
    }

    void Render(const EditorRect& rect) {
        if (!Selection::HasSelection()) {
            InputElements::PropertyList properties;
            properties.Render(rect);
            return;
        }

        const uint64_t objectId = Selection::GetSelectedObjectId();
        if (Selection::HasSelectedChristmasLightPoint()) {
            RenderChristmasLightPointProperties(rect, objectId, Selection::GetSelectedChristmasLightPointIndex());
            return;
        }
        if (Selection::HasSelectedWallSegment()) {
            InputElements::PropertyList properties;
            properties.Render(rect);
            return;
        }

        switch (GetObjectIdType(objectId)) {
            case ObjectType::CHRISTMAS_LIGHTS: RenderChristmasLightsProperties(rect, objectId); break;
            case ObjectType::DDGI_VOLUME:      RenderDDGIVolumeProperties(rect, objectId);      break;
            case ObjectType::DOBERMANN:        RenderDobermannProperties(rect, objectId);       break;
            case ObjectType::DOOR:             RenderDoorProperties(rect, objectId);            break;
            case ObjectType::FIREPLACE:        RenderFireplaceProperties(rect, objectId);       break;
            case ObjectType::GENERIC_OBJECT:   RenderGenericObjectProperties(rect, objectId);   break;
            case ObjectType::JETTY:            RenderJettyProperties(rect, objectId);           break;
            case ObjectType::LADDER:           RenderLadderProperties(rect, objectId);          break;
            case ObjectType::LIGHT:            RenderLightProperties(rect, objectId);           break;
            case ObjectType::MERMAID:          RenderMermaidProperties(rect, objectId);         break;
            case ObjectType::PIANO:            RenderPianoProperties(rect, objectId);           break;
            case ObjectType::PICK_UP:          RenderPickUpProperties(rect, objectId);          break;
            case ObjectType::PICTURE_FRAME:    RenderPictureFrameProperties(rect, objectId);    break;
            case ObjectType::STAIRCASE:        RenderStaircaseProperties(rect, objectId);       break;
            case ObjectType::WALL:             RenderWallProperties(rect, objectId);            break;
            case ObjectType::WINDOW:           RenderWindowProperties(rect, objectId);          break;
            case ObjectType::WORLD_PLANE:      RenderWorldPlaneProperties(rect, objectId);      break;
            case ObjectType::FENCE:
            case ObjectType::POWER_POLE_SET:   RenderNameOnlyProperties(rect, objectId);        break;
            case ObjectType::ANIMATED_GAME_OBJECT:
            case ObjectType::SHARK:            RenderPositionOnlyProperties(rect, objectId);    break;
            default:                           RenderDefaultProperties(rect, objectId);         break;
        }
    }
}
