#include "Hell/Logging.h"
#include "Hell/Common/Enum.h"
#include "Hell/ResourceManagement/ResourceManager.h"

#include "Unloved/Systems/Map/MapManager.h"
#include "Legacy/World/LegacyWorld.h"
#include "Unloved/World/World.h"

#include "Unloved/Debug/DebugDraw.h"
#include "Unloved/Editor/Editor.h"
#include "Unloved/Editor/ObjectNames.h"
#include "Unloved/UI/Imgui/ImguiBackEnd.h"
#include "Unloved/UI/Imgui/Types/Types.h"

#include "Unloved/Objects/Exterior/Jetty.h"

#include <imgui/imgui.h>

namespace Unloved::Editor {

    EditorUI::LeftPanel g_leftPanel;

    EditorUI::CollapsingHeader g_settingsHeader;
    EditorUI::CheckBox g_backfaceCulling;

    EditorUI::CollapsingHeader g_mapPropertiesHeader;
    EditorUI::CollapsingHeader g_objectPropertiesHeader;
    EditorUI::CollapsingHeader g_outlinerHeader;
    EditorUI::StringInput g_mapNameInput;
    EditorUI::StringInput g_objectNameInput;
    EditorUI::FloatInput g_positionX;
    EditorUI::FloatInput g_positionY;
    EditorUI::FloatInput g_positionZ;
    EditorUI::FloatSliderInput g_rotationX;
    EditorUI::FloatSliderInput g_rotationY;
    EditorUI::FloatSliderInput g_rotationZ;
    EditorUI::FloatInput g_extentsX;
    EditorUI::FloatInput g_extentsY;
    EditorUI::FloatInput g_extentsZ;
    EditorUI::Outliner g_outliner;
    EditorUI::DropDown g_materialDropDown;
    EditorUI::FloatInput g_probeSpacing;

    EditorUI::FloatInput g_textureScale;
    EditorUI::FloatSliderInput g_textureOffsetU;
    EditorUI::FloatSliderInput g_textureOffsetV;

    EditorUI::FloatInput g_worldPlaneP0X;
    EditorUI::FloatInput g_worldPlaneP0Y;
    EditorUI::FloatInput g_worldPlaneP0Z;
    EditorUI::FloatInput g_worldPlaneP1X;
    EditorUI::FloatInput g_worldPlaneP1Y;
    EditorUI::FloatInput g_worldPlaneP1Z;
    EditorUI::FloatInput g_worldPlaneP2X;
    EditorUI::FloatInput g_worldPlaneP2Y;
    EditorUI::FloatInput g_worldPlaneP2Z;
    EditorUI::FloatInput g_worldPlaneP3X;
    EditorUI::FloatInput g_worldPlaneP3Y;
    EditorUI::FloatInput g_worldPlaneP3Z;

    // Door stuff
    EditorUI::DropDown g_doorType;
    EditorUI::DropDown g_doorFrontMaterial;
    EditorUI::DropDown g_doorBackMaterial;
    EditorUI::DropDown g_doorFrameFrontMaterial;
    EditorUI::DropDown g_doorFrameBackMaterial;
    EditorUI::CheckBox g_doorHasDeadLock;
    EditorUI::CheckBox g_doorDeadLockedAtStart;

    // Pickup stuff
    EditorUI::CheckBox g_pickUpDisablePhysicsAtSpawn;
    EditorUI::CheckBox g_pickUpRespawn;

    EditorUI::IntegerInput g_jettyBoardCount;

    void InitLeftPanel() {
        g_mapPropertiesHeader.SetTitle("Map Editor");
        g_objectPropertiesHeader.SetTitle("Properties");

        g_settingsHeader.SetTitle("Settings");
        g_backfaceCulling.SetText("Backface culling");
        g_backfaceCulling.SetState(BackfaceCullingEnabled());

        g_materialDropDown.SetText("Material");
        g_materialDropDown.SetOptions(Hell::ResourceManager::GetMaterialNames());

        g_textureScale.SetText("Tex Scale");
        g_textureScale.SetRange(0.00f, 100.0f);
        g_textureOffsetU.SetText("Tex Offset U");
        g_textureOffsetU.SetRange(-1.0f, 1.0f);
        g_textureOffsetV.SetText("Tex Offset V");
        g_textureOffsetV.SetRange(-1.0f, 1.0f);

        g_worldPlaneP0X.SetText("P0 X");
        g_worldPlaneP0Y.SetText("P0 Y");
        g_worldPlaneP0Z.SetText("P0 Z");
        g_worldPlaneP1X.SetText("P1 X");
        g_worldPlaneP1Y.SetText("P1 Y");
        g_worldPlaneP1Z.SetText("P1 Z");
        g_worldPlaneP2X.SetText("P2 X");
        g_worldPlaneP2Y.SetText("P2 Y");
        g_worldPlaneP2Z.SetText("P2 Z");
        g_worldPlaneP3X.SetText("P3 X");
        g_worldPlaneP3Y.SetText("P3 Y");
        g_worldPlaneP3Z.SetText("P3 Z");

        g_probeSpacing.SetText("Probe Spacing");
        g_probeSpacing.SetRange(0.1f, 2.0f);

        g_doorType.SetText("Type");
        g_doorFrontMaterial.SetText("Front Material");
        g_doorBackMaterial.SetText("Back Material");
        g_doorFrameFrontMaterial.SetText("Frame Front Material");
        g_doorFrameBackMaterial.SetText("Frame Back Material");
        g_doorHasDeadLock.SetText("Has Deadlock");
        g_doorDeadLockedAtStart.SetText("Deadlocked at start");

        g_pickUpDisablePhysicsAtSpawn.SetText("No PhysX at Start");
        g_pickUpRespawn.SetText("Respawn");

        g_jettyBoardCount.SetText("Board Count");
        g_jettyBoardCount.SetRange(4, 666);
    }


    void UpdateOutliner() {

        if (GetEditorMode() == EditorMode::HOUSE_EDITOR ||
            GetEditorMode() == EditorMode::MAP_OBJECT_EDITOR) {
            MapData* mapData = MapManager::GetMapDataByName(GetEditorMapName());
            if (mapData) {
                g_mapNameInput.SetLabel("Map Name");
                g_mapNameInput.SetText(GetEditorMapName());

                g_outlinerHeader.SetTitle("Outliner");

                for (const EditorObjectNameGroup& group : GetEditorObjectNameGroups()) {
                    g_outliner.AddEditorObjectNameGroup(group);
                }

                g_objectNameInput.SetLabel("Name");

                g_positionX.SetText("Position X");
                g_positionY.SetText("Position Y");
                g_positionZ.SetText("Position Z");

                g_rotationX.SetText("Rotation X");
                g_rotationY.SetText("Rotation Y");
                g_rotationZ.SetText("Rotation Z");

                g_rotationX.SetRange(-HELL_PI, HELL_PI);
                g_rotationY.SetRange(-HELL_PI, HELL_PI);
                g_rotationZ.SetRange(-HELL_PI, HELL_PI);

                g_extentsX.SetText("Extent X");
                g_extentsY.SetText("Extent Y");
                g_extentsZ.SetText("Extent Z");
                g_extentsX.SetRange(-1000, 1000);
                g_extentsY.SetRange(-1000, 1000);
                g_extentsZ.SetRange(-1000, 1000);
            }
        }
    }

    void BeginLeftPanel() {
        g_leftPanel.BeginImGuiElement();

        // Settings
        if (g_settingsHeader.CreateImGuiElement()) {
            if (g_backfaceCulling.CreateImGuiElements()) {
                SetBackfaceCulling(g_backfaceCulling.GetState());
            }
            ImGui::Dummy(ImVec2(0.0f, 10.0f));
        }

        // Map properties
        if (GetEditorMode() == EditorMode::MAP_HEIGHT_EDITOR ||
            GetEditorMode() == EditorMode::MAP_OBJECT_EDITOR) {
            if (g_mapPropertiesHeader.CreateImGuiElement()) {
                g_mapNameInput.CreateImGuiElement();
                ImGui::Dummy(ImVec2(0.0f, 10.0f));
            }
        }

        // Outliner
        if (GetEditorMode() == EditorMode::MAP_OBJECT_EDITOR ||
            GetEditorMode() == EditorMode::HOUSE_EDITOR) {
            if (g_outlinerHeader.CreateImGuiElement()) {
                float outlinerHeight = Hell::BackEnd::GetCurrentWindowHeight() * 0.1f;
                g_outliner.CreateImGuiElements(outlinerHeight);
                ImGui::Dummy(ImVec2(0.0f, 20.0f));
            }
        }

        // Object properties
        if (GetEditorMode() == EditorMode::MAP_OBJECT_EDITOR || GetEditorMode() == EditorMode::HOUSE_EDITOR) {
            if (g_objectPropertiesHeader.CreateImGuiElement()) {

                // Begin scrollable child thing
                float currentY = ImGui::GetCursorScreenPos().y;
                float remainingHeight = Hell::BackEnd::GetCurrentWindowHeight() - currentY - 10.0f;
                ImGui::BeginChild("ObjectPropertiesScrollRegion", ImVec2(0.0f, remainingHeight), false);

                // DDGI Volume
                if (Unloved::DDGIVolume* object = Unloved::World::GetDDGIVolumeByObjectId(GetSelectedObjectId())) {
                    g_extentsX.SetValue(object->GetExtents().x);
                    g_extentsY.SetValue(object->GetExtents().y);
                    g_extentsZ.SetValue(object->GetExtents().z);
                    g_probeSpacing.SetValue(object->GetProbeSpacing());

                    if (g_extentsX.CreateImGuiElements()) object->SetExtents(glm::vec3(g_extentsX.GetValue(), g_extentsY.GetValue(), g_extentsZ.GetValue()));
                    if (g_extentsY.CreateImGuiElements()) object->SetExtents(glm::vec3(g_extentsX.GetValue(), g_extentsY.GetValue(), g_extentsZ.GetValue()));
                    if (g_extentsZ.CreateImGuiElements()) object->SetExtents(glm::vec3(g_extentsX.GetValue(), g_extentsY.GetValue(), g_extentsZ.GetValue()));
                    if (g_probeSpacing.CreateImGuiElements()) object->SetProbeSpacing(g_probeSpacing.GetValue());
                }

                // Fireplace
                if (Fireplace* fireplace = Unloved::World::GetFireplaceById(GetSelectedObjectId())) {
                    EditorUI::FloatInput("Position X", fireplace->GetPosition().x, fireplace, &Fireplace::SetPositionX);
                    EditorUI::FloatInput("Position Y", fireplace->GetPosition().y, fireplace, &Fireplace::SetPositionY);
                    EditorUI::FloatInput("Position Z", fireplace->GetPosition().z, fireplace, &Fireplace::SetPositionZ);
                }

                // Lights
                if (Light* light = Unloved::World::GetLightByObjectId(GetSelectedObjectId())) {
                    AABB aabb(light->GetWorldBoundsMin(), light->GetWorldBoundsMax());
                    DebugDraw::DrawAABB(aabb, YELLOW);

                    EditorUI::DropDown type;
                    type.SetText("Type");
                    type.SetOptions(Hell::Enum::GetNames<LightType>());
                    type.SetCurrentOption(Hell::Enum::ToString(light->GetType()));
                    if (type.CreateImGuiElements()) {
                        LightType newType = Hell::Enum::FromString(type.GetSelectedOptionText(), LightType::HANGING_LIGHT);
                        light->SetType(newType);
                    }

                    EditorUI::DropDown iesType;
                    iesType.SetText("IES Profile");
                    iesType.SetOptions(Hell::Enum::GetNames<IESProfileType>());
                    iesType.SetCurrentOption(Hell::Enum::ToString(light->GetIESProfileType()));
                    if (iesType.CreateImGuiElements()) {
                        IESProfileType newType = Hell::Enum::FromString(iesType.GetSelectedOptionText(), IESProfileType::NONE);
                        light->SetIESProfileType(newType);
                    }

                    EditorUI::FloatInput("Position X",   light->GetPosition().x,  light, &Light::SetPositionX);
                    EditorUI::FloatInput("Position Y",   light->GetPosition().y,  light, &Light::SetPositionY);
                    EditorUI::FloatInput("Position Z",   light->GetPosition().z,  light, &Light::SetPositionZ);
                    EditorUI::FloatInput("Rotation X",   light->GetRotation().x,  light, &Light::SetRotationX);
                    EditorUI::FloatInput("Rotation Y",   light->GetRotation().y,  light, &Light::SetRotationY);
                    EditorUI::FloatInput("Rotation Z",   light->GetRotation().z,  light, &Light::SetRotationZ);
                    EditorUI::FloatInput("Color R",      light->GetColor().x,     light, &Light::SetColorR);
                    EditorUI::FloatInput("Color G",      light->GetColor().y,     light, &Light::SetColorG);
                    EditorUI::FloatInput("Color B",      light->GetColor().z,     light, &Light::SetColorB);
                    EditorUI::FloatInput("Radius",       light->GetRadius(),      light, &Light::SetRadius);
                    EditorUI::FloatInput("Strength",     light->GetStrength(),    light, &Light::SetStrength);
                    EditorUI::FloatInput("IES Exposure", light->GetIESExposure(), light, &Light::SetIESExposure);
                    EditorUI::FloatInput("Forward X",    light->GetForward().x,   light, &Light::SetForwardX);
                    EditorUI::FloatInput("Forward Y",    light->GetForward().y,   light, &Light::SetForwardY);
                    EditorUI::FloatInput("Forward Z",    light->GetForward().z,   light, &Light::SetForwardZ);
                    EditorUI::FloatInput("Twist",        light->GetTwist(),       light, &Light::SetTwist);
                }


                // Trees (LIKELY BROKEN)
                //if (GetSelectedObjectType() == ObjectType::TREE) {
                //    Tree* tree = LegacyWorld::GetTreeByObjectId(GetSelectedObjectId());
                //    if (tree) {
                //        if (g_positionX.CreateImGuiElements())  tree->SetPosition(glm::vec3(g_positionX.GetValue(), g_positionY.GetValue(), g_positionZ.GetValue()));
                //        if (g_positionY.CreateImGuiElements())  tree->SetPosition(glm::vec3(g_positionX.GetValue(), g_positionY.GetValue(), g_positionZ.GetValue()));
                //        if (g_positionZ.CreateImGuiElements())  tree->SetPosition(glm::vec3(g_positionX.GetValue(), g_positionY.GetValue(), g_positionZ.GetValue()));
                //
                //        if (g_rotationX.CreateImGuiElements())  tree->SetRotation(glm::vec3(g_rotationX.GetValue(), g_rotationY.GetValue(), g_rotationZ.GetValue()));
                //        if (g_rotationY.CreateImGuiElements())  tree->SetRotation(glm::vec3(g_rotationX.GetValue(), g_rotationY.GetValue(), g_rotationZ.GetValue()));
                //        if (g_rotationZ.CreateImGuiElements())  tree->SetRotation(glm::vec3(g_rotationX.GetValue(), g_rotationY.GetValue(), g_rotationZ.GetValue()));
                //    }
                //}

                // Windows (BARELY FUNCITONAL)
                if (Window* window = Unloved::World::GetWindowByObjectId(GetSelectedObjectId())) {
                    g_positionX.SetValue(window->GetPosition().x);
                    g_positionY.SetValue(window->GetPosition().y);
                    g_positionZ.SetValue(window->GetPosition().z);
                    g_rotationY.SetValue(window->GetRotation().y);

                    if (g_positionX.CreateImGuiElements())  window->SetPosition(glm::vec3(g_positionX.GetValue(), g_positionY.GetValue(), g_positionZ.GetValue()));
                    if (g_positionY.CreateImGuiElements())  window->SetPosition(glm::vec3(g_positionX.GetValue(), g_positionY.GetValue(), g_positionZ.GetValue()));
                    if (g_positionZ.CreateImGuiElements())  window->SetPosition(glm::vec3(g_positionX.GetValue(), g_positionY.GetValue(), g_positionZ.GetValue()));
                    if (g_rotationY.CreateImGuiElements())  window->SetRotationY(g_rotationY.GetValue());
                }

                // Doors (BARELY FUNCITONAL)
                if (Door* door = Unloved::World::GetDoorByObjectId(GetSelectedObjectId())) {
                    g_positionX.SetValue(door->GetPosition().x);
                    g_positionY.SetValue(door->GetPosition().y);
                    g_positionZ.SetValue(door->GetPosition().z);
                    g_rotationY.SetValue(door->GetRotation().y);

                    std::vector<std::string> types;
                    types.push_back(Hell::Enum::ToString(DoorType::STANDARD_A));
                    types.push_back(Hell::Enum::ToString(DoorType::STANDARD_B));
                    types.push_back(Hell::Enum::ToString(DoorType::STAINED_GLASS));
                    types.push_back(Hell::Enum::ToString(DoorType::STAINED_GLASS2));

                    std::vector<std::string> materialTypes;
                    materialTypes.push_back(Hell::Enum::ToString(DoorMaterialType::RESIDENT_EVIL));
                    materialTypes.push_back(Hell::Enum::ToString(DoorMaterialType::WHITE_PAINT));

                    g_doorType.SetOptions(types);
                    g_doorFrontMaterial.SetOptions(materialTypes);
                    g_doorBackMaterial.SetOptions(materialTypes);
                    g_doorFrameFrontMaterial.SetOptions(materialTypes);
                    g_doorFrameBackMaterial.SetOptions(materialTypes);

                    g_doorType.SetCurrentOption(Hell::Enum::ToString(door->GetType()));
                    g_doorFrontMaterial.SetCurrentOption(Hell::Enum::ToString(door->GetMaterialTypeFront()));
                    g_doorBackMaterial.SetCurrentOption(Hell::Enum::ToString(door->GetMaterialTypeBack()));
                    g_doorFrameFrontMaterial.SetCurrentOption(Hell::Enum::ToString(door->GetMaterialTypeFrameFront()));
                    g_doorFrameBackMaterial.SetCurrentOption(Hell::Enum::ToString(door->GetMaterialTypeFrameBack()));

                    g_doorHasDeadLock.SetState(door->GetDeadLockState());
                    g_doorDeadLockedAtStart.SetState(door->GetDeadLockedAtInitState());

                    if (g_positionX.CreateImGuiElements())  door->SetPosition(glm::vec3(g_positionX.GetValue(), g_positionY.GetValue(), g_positionZ.GetValue()));
                    if (g_positionY.CreateImGuiElements())  door->SetPosition(glm::vec3(g_positionX.GetValue(), g_positionY.GetValue(), g_positionZ.GetValue()));
                    if (g_positionZ.CreateImGuiElements())  door->SetPosition(glm::vec3(g_positionX.GetValue(), g_positionY.GetValue(), g_positionZ.GetValue()));
                    if (g_rotationY.CreateImGuiElements())  door->SetRotationY(g_rotationY.GetValue());

                    if (g_doorType.CreateImGuiElements()) {
                        door->SetType(Hell::Enum::FromString(g_doorType.GetSelectedOptionText(), DoorType::UNDEFINED));
                    }
                    if (g_doorFrontMaterial.CreateImGuiElements()) {
                        door->SetFrontMaterial(Hell::Enum::FromString(g_doorFrontMaterial.GetSelectedOptionText(), DoorMaterialType::UNDEFINED));
                    }
                    if (g_doorBackMaterial.CreateImGuiElements()) {
                        door->SetBackMaterial(Hell::Enum::FromString(g_doorBackMaterial.GetSelectedOptionText(), DoorMaterialType::UNDEFINED));
                    }
                    if (g_doorFrameFrontMaterial.CreateImGuiElements()) {
                        door->SetFrameFrontMaterial(Hell::Enum::FromString(g_doorFrameFrontMaterial.GetSelectedOptionText(), DoorMaterialType::UNDEFINED));
                    }
                    if (g_doorFrameBackMaterial.CreateImGuiElements()) {
                        door->SetFrameBackMaterial(Hell::Enum::FromString(g_doorFrameBackMaterial.GetSelectedOptionText(), DoorMaterialType::UNDEFINED));
                    }
                    if (g_doorHasDeadLock.CreateImGuiElements()) {
                        door->SetDeadLockState(g_doorHasDeadLock.GetState());
                    }
                    if (g_doorDeadLockedAtStart.CreateImGuiElements()) {
                        door->SetDeadLockedAtInitState(g_doorDeadLockedAtStart.GetState());
                    }
                }

                // Jetties
                if (GetSelectedObjectType() == ObjectType::JETTY) {
                    if (Jetty* jetty = Unloved::World::GetJettyById(GetSelectedObjectId())) {
                        g_jettyBoardCount.SetValue(jetty->GetBoardCount());


                        if (g_jettyBoardCount.CreateImGuiElements()) {
                            jetty->SetBoardCount(g_jettyBoardCount.GetValue());
                        }

                    }
                }

                // Pick Ups
                if (GetSelectedObjectType() == ObjectType::PICK_UP) {
                    if (PickUp* pickUp = Unloved::World::GetPickUpByObjectId(GetSelectedObjectId())) {
                        // Retrieve state
                        g_positionX.SetValue(pickUp->GetPosition().x);
                        g_positionY.SetValue(pickUp->GetPosition().y);
                        g_positionZ.SetValue(pickUp->GetPosition().z);
                        g_rotationX.SetValue(pickUp->GetRotation().x);
                        g_rotationY.SetValue(pickUp->GetRotation().y);
                        g_rotationZ.SetValue(pickUp->GetRotation().y);
                        g_pickUpDisablePhysicsAtSpawn.SetState(pickUp->GetDisabledPhysicsAtSpawnState());
                        g_pickUpRespawn.SetState(pickUp->GetRespawnState());

                        // Render and set state
                        if (g_positionX.CreateImGuiElements())                      pickUp->SetPosition(glm::vec3(g_positionX.GetValue(), g_positionY.GetValue(), g_positionZ.GetValue()));
                        if (g_positionY.CreateImGuiElements())                      pickUp->SetPosition(glm::vec3(g_positionX.GetValue(), g_positionY.GetValue(), g_positionZ.GetValue()));
                        if (g_positionZ.CreateImGuiElements())                      pickUp->SetPosition(glm::vec3(g_positionX.GetValue(), g_positionY.GetValue(), g_positionZ.GetValue()));
                        if (g_rotationX.CreateImGuiElements())                      pickUp->SetRotation(glm::vec3(g_rotationX.GetValue(), g_rotationY.GetValue(), g_rotationZ.GetValue()));
                        if (g_rotationY.CreateImGuiElements())                      pickUp->SetRotation(glm::vec3(g_rotationX.GetValue(), g_rotationY.GetValue(), g_rotationZ.GetValue()));
                        if (g_rotationZ.CreateImGuiElements())                      pickUp->SetRotation(glm::vec3(g_rotationX.GetValue(), g_rotationY.GetValue(), g_rotationZ.GetValue()));
                        if (g_pickUpDisablePhysicsAtSpawn.CreateImGuiElements())    pickUp->SetDisabledPhysicsAtSpawnState(g_pickUpDisablePhysicsAtSpawn.GetState());
                        if (g_pickUpRespawn.CreateImGuiElements())                  pickUp->SetRespawnState(g_pickUpRespawn.GetState());
                    }
                }


                // World planes (aka floors and ceilings)
                if (WorldPlane* worldPlane = Unloved::World::GetWorldPlaneByObjectId(GetSelectedObjectId())) {
                    bool worldPlaneUpdated = false;

                    g_materialDropDown.SetCurrentOption(worldPlane->GetCreateInfo().materialName);
                    g_worldPlaneP0X.SetValue(worldPlane->GetCreateInfo().p0.x);
                    g_worldPlaneP0Y.SetValue(worldPlane->GetCreateInfo().p0.y);
                    g_worldPlaneP0Z.SetValue(worldPlane->GetCreateInfo().p0.z);
                    g_worldPlaneP1X.SetValue(worldPlane->GetCreateInfo().p1.x);
                    g_worldPlaneP1Y.SetValue(worldPlane->GetCreateInfo().p1.y);
                    g_worldPlaneP1Z.SetValue(worldPlane->GetCreateInfo().p1.z);
                    g_worldPlaneP2X.SetValue(worldPlane->GetCreateInfo().p2.x);
                    g_worldPlaneP2Y.SetValue(worldPlane->GetCreateInfo().p2.y);
                    g_worldPlaneP2Z.SetValue(worldPlane->GetCreateInfo().p2.z);
                    g_worldPlaneP3X.SetValue(worldPlane->GetCreateInfo().p3.x);
                    g_worldPlaneP3Y.SetValue(worldPlane->GetCreateInfo().p3.y);
                    g_worldPlaneP3Z.SetValue(worldPlane->GetCreateInfo().p3.z);
                    g_textureOffsetU.SetValue(worldPlane->GetCreateInfo().textureOffsetU);
                    g_textureOffsetV.SetValue(worldPlane->GetCreateInfo().textureOffsetV);
                    g_textureScale.SetValue(worldPlane->GetCreateInfo().textureScale);

                    if (g_materialDropDown.CreateImGuiElements()) {
                        worldPlane->SetMaterial(g_materialDropDown.GetSelectedOptionText());
                        worldPlaneUpdated = true;
                    }

                    if (g_worldPlaneP0X.CreateImGuiElements()) worldPlaneUpdated = true;
                    if (g_worldPlaneP0Y.CreateImGuiElements()) worldPlaneUpdated = true;
                    if (g_worldPlaneP0Z.CreateImGuiElements()) worldPlaneUpdated = true;
                    if (g_worldPlaneP1X.CreateImGuiElements()) worldPlaneUpdated = true;
                    if (g_worldPlaneP1Y.CreateImGuiElements()) worldPlaneUpdated = true;
                    if (g_worldPlaneP1Z.CreateImGuiElements()) worldPlaneUpdated = true;
                    if (g_worldPlaneP2X.CreateImGuiElements()) worldPlaneUpdated = true;
                    if (g_worldPlaneP2Y.CreateImGuiElements()) worldPlaneUpdated = true;
                    if (g_worldPlaneP2Z.CreateImGuiElements()) worldPlaneUpdated = true;
                    if (g_worldPlaneP3X.CreateImGuiElements()) worldPlaneUpdated = true;
                    if (g_worldPlaneP3Y.CreateImGuiElements()) worldPlaneUpdated = true;
                    if (g_worldPlaneP3Z.CreateImGuiElements()) worldPlaneUpdated = true;

                    if (g_textureScale.CreateImGuiElements()) {
                        worldPlane->SetTextureScale(g_textureScale.GetValue());
                    }
                    if (g_textureOffsetU.CreateImGuiElements()) {
                        worldPlane->SetTextureOffsetU(g_textureOffsetU.GetValue());
                    }
                    if (g_textureOffsetV.CreateImGuiElements()) {
                        worldPlane->SetTextureOffsetV(g_textureOffsetV.GetValue());
                    }

                    if (worldPlaneUpdated) {
                        WorldPlaneCreateInfo& createInfo = worldPlane->GetCreateInfo();
                        createInfo.p0.x = g_worldPlaneP0X.GetValue();
                        createInfo.p0.y = g_worldPlaneP0Y.GetValue();
                        createInfo.p0.z = g_worldPlaneP0Z.GetValue();
                        createInfo.p1.x = g_worldPlaneP1X.GetValue();
                        createInfo.p1.y = g_worldPlaneP1Y.GetValue();
                        createInfo.p1.z = g_worldPlaneP1Z.GetValue();
                        createInfo.p2.x = g_worldPlaneP2X.GetValue();
                        createInfo.p2.y = g_worldPlaneP2Y.GetValue();
                        createInfo.p2.z = g_worldPlaneP2Z.GetValue();
                        createInfo.p3.x = g_worldPlaneP3X.GetValue();
                        createInfo.p3.y = g_worldPlaneP3Y.GetValue();
                        createInfo.p3.z = g_worldPlaneP3Z.GetValue();
                        worldPlane->UpdateVertexDataFromCreateInfo();
                    }
                }

                // Walls
                if (Wall* wall = Unloved::World::GetWallByObjectId(GetSelectedObjectId())) {
                    g_materialDropDown.SetCurrentOption(wall->GetCreateInfo().materialName);
                    g_textureOffsetU.SetValue(wall->GetCreateInfo().textureOffsetU);
                    g_textureOffsetV.SetValue(wall->GetCreateInfo().textureOffsetV);
                    g_textureScale.SetValue(wall->GetCreateInfo().textureScale);

                    // Material
                    if (g_materialDropDown.CreateImGuiElements()) {
                        wall->SetMaterial(g_materialDropDown.GetSelectedOptionText());
                    }

                    // Texture settings
                    if (g_textureScale.CreateImGuiElements()) {
                        wall->SetTextureScale(g_textureScale.GetValue());
                    }
                    if (g_textureOffsetU.CreateImGuiElements()) {
                        wall->SetTextureOffsetU(g_textureOffsetU.GetValue());
                    }
                    if (g_textureOffsetV.CreateImGuiElements()) {
                        wall->SetTextureOffsetV(g_textureOffsetV.GetValue());
                    }
                }

                ImGui::Dummy(ImVec2(0.0f, 20.0f));

                // End scrollable region
                ImGui::EndChild();
            }
        }

    }

    void EndLeftPanel() {
        g_leftPanel.EndImGuiElement();
    }
}


