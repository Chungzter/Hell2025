#include "Hell/Logging.h"
#include "Hell/Common/Enum.h"
#include "Hell/ResourceManagement/ResourceManager.h"

#include "Unloved/Maps/MapManager.h"
#include "Legacy/Util/Util.h"
#include "Legacy/World/LegacyWorld.h"

#include "Unloved/Debug/DebugDraw.h"
#include "Unloved/Editor/Editor.h"
#include "Unloved/UI/Imgui/ImguiBackEnd.h"
#include "Unloved/UI/Imgui/Types/Types.h"

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

    EditorUI::FloatInput g_heightFloatInput;

    EditorUI::FloatInput g_housePlaneP0X;
    EditorUI::FloatInput g_housePlaneP0Y;
    EditorUI::FloatInput g_housePlaneP0Z;
    EditorUI::FloatInput g_housePlaneP1X;
    EditorUI::FloatInput g_housePlaneP1Y;
    EditorUI::FloatInput g_housePlaneP1Z;
    EditorUI::FloatInput g_housePlaneP2X;
    EditorUI::FloatInput g_housePlaneP2Y;
    EditorUI::FloatInput g_housePlaneP2Z;
    EditorUI::FloatInput g_housePlaneP3X;
    EditorUI::FloatInput g_housePlaneP3Y;
    EditorUI::FloatInput g_housePlaneP3Z;

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

    void InitLeftPanel() {
        g_mapPropertiesHeader.SetTitle("Map Editor");
        g_objectPropertiesHeader.SetTitle("Properties");

        g_settingsHeader.SetTitle("Settings");
        g_backfaceCulling.SetText("Backface culling");
        g_backfaceCulling.SetState(BackfaceCullingEnabled());

        g_materialDropDown.SetText("Material");
        g_materialDropDown.SetOptions(Hell::ResourceManager::GetMaterialNames());

        g_heightFloatInput.SetText("Height");
        g_heightFloatInput.SetRange(0.1f, 100.0f);

        g_textureScale.SetText("Tex Scale");
        g_textureScale.SetRange(0.00f, 100.0f);
        g_textureOffsetU.SetText("Tex Offset U");
        g_textureOffsetU.SetRange(-1.0f, 1.0f);
        g_textureOffsetV.SetText("Tex Offset V");
        g_textureOffsetV.SetRange(-1.0f, 1.0f);

        g_housePlaneP0X.SetText("P0 X");
        g_housePlaneP0Y.SetText("P0 Y");
        g_housePlaneP0Z.SetText("P0 Z");
        g_housePlaneP1X.SetText("P1 X");
        g_housePlaneP1Y.SetText("P1 Y");
        g_housePlaneP1Z.SetText("P1 Z");
        g_housePlaneP2X.SetText("P2 X");
        g_housePlaneP2Y.SetText("P2 Y");
        g_housePlaneP2Z.SetText("P2 Z");
        g_housePlaneP3X.SetText("P3 X");
        g_housePlaneP3Y.SetText("P3 Y");
        g_housePlaneP3Z.SetText("P3 Z");

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
    }

    void UpdateOutliner() {
        if (GetEditorMode() == EditorMode::HOUSE_EDITOR ||
            GetEditorMode() == EditorMode::MAP_OBJECT_EDITOR) {
            Map* map = MapManager::GetMapByName(GetEditorMapName());
            if (map) {
                g_mapNameInput.SetLabel("Map Name");
                g_mapNameInput.SetText(GetEditorMapName());

                g_outlinerHeader.SetTitle("Outliner");

                g_outliner.SetItems("Ceilings", GetCeilingNames());
                g_outliner.AddItems("DDGI Volumes", LegacyWorld::GetDDGIVolumes().ids());
                g_outliner.AddItems("Lights", LegacyWorld::GetLightIds());
                g_outliner.SetItems("Doors", GetDoorNames());
                g_outliner.SetItems("Floors", GetFloorNames());
                g_outliner.SetItems("Generic Objects", GetGenericObjectNames());
                g_outliner.SetItems("House Planes", GetUndefinedHousePlaneNames());
                g_outliner.SetItems("Walls", GetWallNames());

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

        // move to UpdateObjectProperties()
        //if (GetEditorMode() == EditorMode::MAP_HEIGHT_EDITOR || GetEditorMode() == EditorMode::MAP_OBJECT_EDITOR) {
        //    if (GetSelectedObjectType() == ObjectType::TREE) {
        //        Tree* tree = LegacyWorld::GetTreeByObjectId(GetSelectedObjectId());
        //        if (tree) {
        //            g_objectNameInput.SetText(tree->GetEditorName());
        //            g_positionX.SetValue(tree->GetPosition().x);
        //            g_positionY.SetValue(tree->GetPosition().y);
        //            g_positionZ.SetValue(tree->GetPosition().z);
        //            g_rotationX.SetValue(tree->GetRotation().x);
        //            g_rotationY.SetValue(tree->GetRotation().y);
        //            g_rotationZ.SetValue(tree->GetRotation().z);
        //        }
        //    }
        //}
    }

    void BeginLeftPanel() {
        bool houseMeshUpdateRequired = false;

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
                if (Unloved::DDGIVolume* object = LegacyWorld::GetDDGIVolumeByObjectId(GetSelectedObjectId())) {
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
                if (Fireplace* fireplace = LegacyWorld::GetFireplaceById(GetSelectedObjectId())) {
                    EditorUI::FloatInput("Position X", fireplace->GetPosition().x, fireplace, &Fireplace::SetPositionX);
                    EditorUI::FloatInput("Position Y", fireplace->GetPosition().y, fireplace, &Fireplace::SetPositionY);
                    EditorUI::FloatInput("Position Z", fireplace->GetPosition().z, fireplace, &Fireplace::SetPositionZ);
                }

                // Lights
                if (Light* light = LegacyWorld::GetLightByObjectId(GetSelectedObjectId())) {
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
                if (Window* window = LegacyWorld::GetWindowByObjectId(GetSelectedObjectId())) {
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
                if (Door* door = LegacyWorld::GetDoorByObjectId(GetSelectedObjectId())) {
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


                // Pick Ups
                if (GetSelectedObjectType() == ObjectType::PICK_UP) {
                    if (PickUp* pickUp = LegacyWorld::GetPickUpByObjectId(GetSelectedObjectId())) {
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


                // House planes (aka floors and ceilings)
                if (HousePlane* housePlane = LegacyWorld::GetHousePlaneByObjectId(GetSelectedObjectId())) {
                    bool housePlaneUpdated = false;

                    g_materialDropDown.SetCurrentOption(housePlane->GetCreateInfo().materialName);
                    g_housePlaneP0X.SetValue(housePlane->GetCreateInfo().p0.x);
                    g_housePlaneP0Y.SetValue(housePlane->GetCreateInfo().p0.y);
                    g_housePlaneP0Z.SetValue(housePlane->GetCreateInfo().p0.z);
                    g_housePlaneP1X.SetValue(housePlane->GetCreateInfo().p1.x);
                    g_housePlaneP1Y.SetValue(housePlane->GetCreateInfo().p1.y);
                    g_housePlaneP1Z.SetValue(housePlane->GetCreateInfo().p1.z);
                    g_housePlaneP2X.SetValue(housePlane->GetCreateInfo().p2.x);
                    g_housePlaneP2Y.SetValue(housePlane->GetCreateInfo().p2.y);
                    g_housePlaneP2Z.SetValue(housePlane->GetCreateInfo().p2.z);
                    g_housePlaneP3X.SetValue(housePlane->GetCreateInfo().p3.x);
                    g_housePlaneP3Y.SetValue(housePlane->GetCreateInfo().p3.y);
                    g_housePlaneP3Z.SetValue(housePlane->GetCreateInfo().p3.z);
                    g_textureOffsetU.SetValue(housePlane->GetCreateInfo().textureOffsetU);
                    g_textureOffsetV.SetValue(housePlane->GetCreateInfo().textureOffsetV);
                    g_textureScale.SetValue(housePlane->GetCreateInfo().textureScale);

                    if (g_materialDropDown.CreateImGuiElements()) {
                        housePlane->SetMaterial(g_materialDropDown.GetSelectedOptionText());
                        houseMeshUpdateRequired = true;
                        housePlaneUpdated = true;
                    }

                    if (g_housePlaneP0X.CreateImGuiElements()) housePlaneUpdated = true;
                    if (g_housePlaneP0Y.CreateImGuiElements()) housePlaneUpdated = true;
                    if (g_housePlaneP0Z.CreateImGuiElements()) housePlaneUpdated = true;
                    if (g_housePlaneP1X.CreateImGuiElements()) housePlaneUpdated = true;
                    if (g_housePlaneP1Y.CreateImGuiElements()) housePlaneUpdated = true;
                    if (g_housePlaneP1Z.CreateImGuiElements()) housePlaneUpdated = true;
                    if (g_housePlaneP2X.CreateImGuiElements()) housePlaneUpdated = true;
                    if (g_housePlaneP2Y.CreateImGuiElements()) housePlaneUpdated = true;
                    if (g_housePlaneP2Z.CreateImGuiElements()) housePlaneUpdated = true;
                    if (g_housePlaneP3X.CreateImGuiElements()) housePlaneUpdated = true;
                    if (g_housePlaneP3Y.CreateImGuiElements()) housePlaneUpdated = true;
                    if (g_housePlaneP3Z.CreateImGuiElements()) housePlaneUpdated = true;

                    if (g_textureScale.CreateImGuiElements()) {
                        housePlane->SetTextureScale(g_textureScale.GetValue());
                        houseMeshUpdateRequired = true;
                    }
                    if (g_textureOffsetU.CreateImGuiElements()) {
                        housePlane->SetTextureOffsetU(g_textureOffsetU.GetValue());
                        houseMeshUpdateRequired = true;
                    }
                    if (g_textureOffsetV.CreateImGuiElements()) {
                        housePlane->SetTextureOffsetV(g_textureOffsetV.GetValue());
                        houseMeshUpdateRequired = true;
                    }

                    if (housePlaneUpdated) {
                        HousePlaneCreateInfo& createInfo = housePlane->GetCreateInfo();
                        createInfo.p0.x = g_housePlaneP0X.GetValue();
                        createInfo.p0.y = g_housePlaneP0Y.GetValue();
                        createInfo.p0.z = g_housePlaneP0Z.GetValue();
                        createInfo.p1.x = g_housePlaneP1X.GetValue();
                        createInfo.p1.y = g_housePlaneP1Y.GetValue();
                        createInfo.p1.z = g_housePlaneP1Z.GetValue();
                        createInfo.p2.x = g_housePlaneP2X.GetValue();
                        createInfo.p2.y = g_housePlaneP2Y.GetValue();
                        createInfo.p2.z = g_housePlaneP2Z.GetValue();
                        createInfo.p3.x = g_housePlaneP3X.GetValue();
                        createInfo.p3.y = g_housePlaneP3Y.GetValue();
                        createInfo.p3.z = g_housePlaneP3Z.GetValue();
                        housePlane->UpdateVertexDataFromCreateInfo();
                        houseMeshUpdateRequired = true;
                    }
                }

                // Walls
                if (Wall* wall = LegacyWorld::GetWallByObjectId(GetSelectedObjectId())) {
                    g_materialDropDown.SetCurrentOption(wall->GetCreateInfo().materialName);
                    g_heightFloatInput.SetValue(wall->GetCreateInfo().height);
                    g_textureOffsetU.SetValue(wall->GetCreateInfo().textureOffsetU);
                    g_textureOffsetV.SetValue(wall->GetCreateInfo().textureOffsetV);
                    g_textureScale.SetValue(wall->GetCreateInfo().textureScale);

                    // Material
                    if (g_materialDropDown.CreateImGuiElements()) {
                        wall->SetMaterial(g_materialDropDown.GetSelectedOptionText());
                        houseMeshUpdateRequired = true;
                    }

                    // Height
                    if (g_heightFloatInput.CreateImGuiElements()) {
                        wall->SetHeight(g_heightFloatInput.GetValue());
                        houseMeshUpdateRequired = true;
                    }

                    // Texture settings
                    if (g_textureScale.CreateImGuiElements()) {
                        wall->SetTextureScale(g_textureScale.GetValue());
                        houseMeshUpdateRequired = true;
                    }
                    if (g_textureOffsetU.CreateImGuiElements()) {
                        wall->SetTextureOffsetU(g_textureOffsetU.GetValue());
                        houseMeshUpdateRequired = true;
                    }
                    if (g_textureOffsetV.CreateImGuiElements()) {
                        wall->SetTextureOffsetV(g_textureOffsetV.GetValue());
                        houseMeshUpdateRequired = true;
                    }
                }

                ImGui::Dummy(ImVec2(0.0f, 20.0f));

                // End scrollable region
                ImGui::EndChild();
            }
        }

        if (houseMeshUpdateRequired) {
            LegacyWorld::RecreateAllHouseGeometry();
        }
    }

    void EndLeftPanel() {
        g_leftPanel.EndImGuiElement();
    }
}


