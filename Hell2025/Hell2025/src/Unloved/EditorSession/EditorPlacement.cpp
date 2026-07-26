#include "EditorPlacement.h"

#include "EditorHierarchy.h"
#include "EditorSelection.h"
#include "EditorViewports.h"

#include "Hell/Common/Enum.h"
#include "Hell/Input.h"
#include "Hell/Logging.h"
#include "Hell/Physics/Physics.h"

#include "Unloved/Bible/Bible.h"
#include "Unloved/Objects/Exterior/Fence.h"
#include "Unloved/Objects/Exterior/PowerPoleSet.h"
#include "Unloved/Objects/Props/Christmas/ChristmasLights.h"
#include "Unloved/Systems/WorldBVH/WorldBVH.h"
#include "Unloved/World/World.h"

#include <cmath>
#include <limits>
#include <vector>

namespace Unloved::EditorSession::Placement {
    namespace {
        constexpr float MAX_RAY_DISTANCE = 2000.0f;
        constexpr bool CULL_BACK_FACING = true;

        struct PlacementHit {
            bool hitFound = false;
            glm::vec3 position = glm::vec3(0.0f);
            glm::vec3 normal = glm::vec3(0.0f, 1.0f, 0.0f);
            uint64_t objectId = 0;
            float distanceToHit = std::numeric_limits<float>::max();
        };

        PlacementTool g_currentTool = PlacementTool::NONE;
        std::vector<SequencePoint> g_sequencePoints;
        uint64_t g_placementObjectId = 0;
        float g_sequencePointValue = 0.0f;

        void ResetState() {
            g_currentTool = PlacementTool::NONE;
            g_sequencePoints.clear();
            g_placementObjectId = 0;
            g_sequencePointValue = 0.0f;
        }

        PlacementHit GetWorldHit() {
            const int32_t viewportIndex = Viewports::GetHoveredViewportIndex();
            if (viewportIndex < 0) return {};

            const glm::vec3& rayOrigin = Viewports::GetMouseRayOrigin(static_cast<uint32_t>(viewportIndex));
            const glm::vec3& rayDirection = Viewports::GetMouseRayDirection(static_cast<uint32_t>(viewportIndex));
            const BvhRayResult bvhResult = WorldBVH::ClosestHit(rayOrigin, rayDirection, MAX_RAY_DISTANCE);
            const PhysXRayResult physXResult = Hell::Physics::CastPhysXRay(rayOrigin, rayDirection, MAX_RAY_DISTANCE, CULL_BACK_FACING);
            PlacementHit hit;

            if (bvhResult.hitFound) {
                hit.hitFound = true;
                hit.position = bvhResult.hitPosition;
                hit.normal = bvhResult.hitNormal;
                hit.objectId = bvhResult.objectId;
                hit.distanceToHit = bvhResult.distanceToHit;
            }
            if (physXResult.hitFound && physXResult.distanceToHit < hit.distanceToHit) {
                hit.hitFound = true;
                hit.position = physXResult.hitPosition;
                hit.normal = physXResult.hitNormal;
                hit.objectId = physXResult.userData.objectId;
                hit.distanceToHit = physXResult.distanceToHit;
            }
            return hit;
        }

        PlacementHit GetTerrainHit() {
            const int32_t viewportIndex = Viewports::GetHoveredViewportIndex();
            if (viewportIndex < 0) return {};

            const glm::vec3& rayOrigin = Viewports::GetMouseRayOrigin(static_cast<uint32_t>(viewportIndex));
            const glm::vec3& rayDirection = Viewports::GetMouseRayDirection(static_cast<uint32_t>(viewportIndex));
            Hell::Physics::ActivateAllHeightFields();
            const PhysXRayResult heightMapResult = Hell::Physics::CastPhysXRayHeightMap(rayOrigin, rayDirection, MAX_RAY_DISTANCE);
            PlacementHit hit;

            if (heightMapResult.hitFound) {
                hit.hitFound = true;
                hit.position = heightMapResult.hitPosition;
                hit.normal = heightMapResult.hitNormal;
                hit.distanceToHit = heightMapResult.distanceToHit;
            }

            constexpr float GROUND_PLANE_Y = -0.01f;
            if (std::abs(rayDirection.y) < 0.000001f) return hit;

            const float rayDistance = (GROUND_PLANE_Y - rayOrigin.y) / rayDirection.y;
            if (rayDistance < 0.0f || rayDistance > MAX_RAY_DISTANCE) return hit;

            const glm::vec3 groundPosition = rayOrigin + rayDirection * rayDistance;
            const float groundDistance = glm::distance(rayOrigin, groundPosition);
            if (hit.hitFound && hit.distanceToHit <= groundDistance) return hit;

            hit.hitFound = true;
            hit.position = groundPosition;
            hit.normal = glm::vec3(0.0f, 1.0f, 0.0f);
            hit.distanceToHit = groundDistance;
            return hit;
        }

        std::vector<glm::vec2> GetFenceControlPoints() {
            std::vector<glm::vec2> controlPoints;
            controlPoints.reserve(g_sequencePoints.size());
            for (const SequencePoint& sequencePoint : g_sequencePoints) controlPoints.emplace_back(sequencePoint.position.x, sequencePoint.position.z);
            return controlPoints;
        }

        uint64_t CreateDirectObject(PlacementTool tool, const PlacementToolInfo& toolInfo, const PlacementHit& hit) {
            GenericObjectType genericObjectType = GenericObjectType::UNDEFINED;
            const char* pickUpName = nullptr;

            switch (tool) {
                case PlacementTool::DOOR_STANDARD_A: {
                    DoorCreateInfo createInfo;
                    createInfo.type = DoorType::STANDARD_A;
                    createInfo.materialTypeFront = DoorMaterialType::RESIDENT_EVIL;
                    createInfo.materialTypeBack = DoorMaterialType::RESIDENT_EVIL;
                    createInfo.materialTypeFrameFront = DoorMaterialType::RESIDENT_EVIL;
                    createInfo.materialTypeFrameBack = DoorMaterialType::RESIDENT_EVIL;
                    createInfo.position = hit.position;
                    createInfo.defaultEditorName = toolInfo.defaultEditorName;
                    return World::AddDoor(createInfo);
                }
                case PlacementTool::DOBERMANN: {
                    DobermannCreateInfo createInfo;
                    createInfo.position = hit.position;
                    createInfo.defaultEditorName = toolInfo.defaultEditorName;
                    return World::AddDobermann(createInfo);
                }
                case PlacementTool::KANGAROO: {
                    KangarooCreateInfo createInfo;
                    createInfo.position = hit.position;
                    createInfo.defaultEditorName = toolInfo.defaultEditorName;
                    return World::AddKangaroo(createInfo);
                }
                case PlacementTool::SHARK: {
                    SharkCreateInfo createInfo;
                    createInfo.position = hit.position;
                    createInfo.defaultEditorName = toolInfo.defaultEditorName;
                    return World::AddShark(createInfo);
                }
                case PlacementTool::FIREPLACE_OPEN:
                case PlacementTool::FIREPLACE_WOOD_STOVE: {
                    FireplaceCreateInfo createInfo;
                    createInfo.type = tool == PlacementTool::FIREPLACE_WOOD_STOVE ? FireplaceType::WOOD_STOVE : FireplaceType::DEFAULT;
                    createInfo.position = hit.position;
                    createInfo.defaultEditorName = toolInfo.defaultEditorName;
                    return World::AddFireplace(createInfo);
                }
                case PlacementTool::LADDER: {
                    LadderCreateInfo createInfo;
                    createInfo.position = hit.position + glm::vec3(0.0f, 1.0f, 0.0f);
                    createInfo.defaultEditorName = toolInfo.defaultEditorName;
                    return World::AddLadder(createInfo);
                }
                case PlacementTool::STAIRCASE: {
                    StaircaseCreateInfo createInfo;
                    createInfo.position = hit.position;
                    createInfo.defaultEditorName = toolInfo.defaultEditorName;
                    return World::AddStaircase(createInfo);
                }
                case PlacementTool::WINDOW: {
                    WindowCreateInfo createInfo;
                    createInfo.position = hit.position;
                    createInfo.defaultEditorName = toolInfo.defaultEditorName;
                    return World::AddWindow(createInfo);
                }
                case PlacementTool::GENERIC_BATHROOM_BASIN:          genericObjectType = GenericObjectType::BATHROOM_BASIN;          break;
                case PlacementTool::GENERIC_BATHROOM_CABINET:        genericObjectType = GenericObjectType::BATHROOM_CABINET;        break;
                case PlacementTool::GENERIC_CHAIR_RE:                genericObjectType = GenericObjectType::CHAIR_RE;                break;
                case PlacementTool::GENERIC_CHAIR_SPINDLE_BACK:      genericObjectType = GenericObjectType::CHAIR_SPINDLE_BACK;      break;
                case PlacementTool::GENERIC_CHRISTMAS_PRESENT_LARGE: genericObjectType = GenericObjectType::CHRISTMAS_PRESENT_LARGE; break;
                case PlacementTool::GENERIC_CHRISTMAS_PRESENT_SMALL: genericObjectType = GenericObjectType::CHRISTMAS_PRESENT_SMALL; break;
                case PlacementTool::GENERIC_CHRISTMAS_TREE:          genericObjectType = GenericObjectType::CHRISTMAS_TREE;          break;
                case PlacementTool::GENERIC_COUCH:                   genericObjectType = GenericObjectType::COUCH;                   break;
                case PlacementTool::GENERIC_DRAWERS_LARGE:           genericObjectType = GenericObjectType::DRAWERS_LARGE;           break;
                case PlacementTool::GENERIC_DRAWERS_SMALL:           genericObjectType = GenericObjectType::DRAWERS_SMALL;           break;
                case PlacementTool::GENERIC_PLANT_BLACKBERRIES:      genericObjectType = GenericObjectType::PLANT_BLACKBERRIES;      break;
                case PlacementTool::GENERIC_PLANT_TREE:              genericObjectType = GenericObjectType::PLANT_TREE;              break;
                case PlacementTool::GENERIC_TOILET:                  genericObjectType = GenericObjectType::TOILET;                  break;
                case PlacementTool::PICKUP_12_GAUGE_BUCKSHOT:        pickUpName = "12GaugeBuckShot"; break;
                case PlacementTool::PICKUP_AKS74U:                   pickUpName = "AKS74U";          break;
                case PlacementTool::PICKUP_BLACK_SKULL:              pickUpName = "BlackSkull";      break;
                case PlacementTool::PICKUP_GLOCK:                    pickUpName = "Glock";           break;
                case PlacementTool::PICKUP_GOLDEN_GLOCK:             pickUpName = "GoldenGlock";     break;
                case PlacementTool::PICKUP_KNIFE:                    pickUpName = "Knife";           break;
                case PlacementTool::PICKUP_P90:                      pickUpName = "P90";             break;
                case PlacementTool::PICKUP_PILLS:                    pickUpName = "Pills";           break;
                case PlacementTool::PICKUP_REMINGTON_870:            pickUpName = "Remington870";    break;
                case PlacementTool::PICKUP_SMALL_KEY:                pickUpName = "SmallKey";        break;
                case PlacementTool::PICKUP_SMALL_KEY_SILVER:         pickUpName = "SmallKeySilver";  break;
                case PlacementTool::PICKUP_SPAS:                     pickUpName = "SPAS";            break;
                case PlacementTool::PICKUP_TOKAREV:                  pickUpName = "Tokarev";         break;
                default: break;
            }

            if (pickUpName) {
                PickUpCreateInfo createInfo;
                createInfo.position = hit.position + glm::vec3(0.0f, 0.5f, 0.0f);
                createInfo.name = pickUpName;
                createInfo.respawn = true;
                createInfo.saveToFile = true;
                createInfo.type = Bible::GetItemType(pickUpName);
                createInfo.defaultEditorName = toolInfo.defaultEditorName;
                return World::AddPickUp(createInfo);
            }

            if (genericObjectType == GenericObjectType::UNDEFINED) {
                Logging::Error() << "EditorPlacement::CreateDirectObject() has no case for '" << Hell::Enum::ToString(tool) << "'\n";
                return 0;
            }

            GenericObjectCreateInfo createInfo;
            createInfo.position = hit.position;
            createInfo.type = genericObjectType;
            createInfo.defaultEditorName = toolInfo.defaultEditorName;
            return World::AddGenericObject(createInfo);
        }

        uint64_t CreatePointSequenceObject(PlacementTool tool, const PlacementToolInfo& toolInfo) {
            switch (tool) {
                case PlacementTool::CHRISTMAS_LIGHTS: {
                    ChristmasLightsCreateInfo createInfo;
                    createInfo.sequencePoints = g_sequencePoints;
                    createInfo.defaultEditorName = toolInfo.defaultEditorName;
                    const uint64_t objectId = World::AddChristmasLights(createInfo);
                    WorldBVH::MarkStaticSceneBvhDirty();
                    return objectId;
                }
                case PlacementTool::FENCE_FARM: {
                    FenceCreateInfo createInfo;
                    createInfo.controlPoints2D = GetFenceControlPoints();
                    createInfo.defaultEditorName = toolInfo.defaultEditorName;
                    const uint64_t objectId = World::AddFence(createInfo);
                    WorldBVH::MarkStaticSceneBvhDirty();
                    return objectId;
                }
                case PlacementTool::POWER_POLES: {
                    PowerPoleSetCreateInfo createInfo;
                    createInfo.sequencePoints = g_sequencePoints;
                    createInfo.defaultEditorName = toolInfo.defaultEditorName;
                    const uint64_t objectId = World::AddPowerPoleSet(createInfo);
                    WorldBVH::MarkStaticSceneBvhDirty();
                    return objectId;
                }
                default: {
                    Logging::Error() << "EditorPlacement::CreatePointSequenceObject() has no case for '" << Hell::Enum::ToString(tool) << "'\n";
                    return 0;
                }
            }
        }

        void UpdatePointSequenceObject(PlacementTool tool) {
            switch (tool) {
                case PlacementTool::CHRISTMAS_LIGHTS: {
                    ChristmasLightSet* christmasLights = World::GetChristmasLightsByObjectId(g_placementObjectId);
                    if (christmasLights) christmasLights->UpdateSequencePoints(g_sequencePoints);
                    WorldBVH::MarkStaticSceneBvhDirty();
                    return;
                }
                case PlacementTool::FENCE_FARM: {
                    Fence* fence = World::GetFenceByObjectId(g_placementObjectId);
                    if (fence) fence->UpdateControlPoints(GetFenceControlPoints());
                    WorldBVH::MarkStaticSceneBvhDirty();
                    return;
                }
                case PlacementTool::POWER_POLES: {
                    PowerPoleSet* powerPoleSet = World::GetPowerPoleSetByObjectId(g_placementObjectId);
                    if (powerPoleSet) powerPoleSet->UpdateSequencePoints(g_sequencePoints);
                    WorldBVH::MarkStaticSceneBvhDirty();
                    return;
                }
                default: {
                    Logging::Error() << "EditorPlacement::UpdatePointSequenceObject() has no case for '" << Hell::Enum::ToString(tool) << "'\n";
                    return;
                }
            }
        }

        void FinishPointSequence() {
            const uint64_t objectId = g_placementObjectId;
            ResetState();
            Hierarchy::Refresh();
            Selection::SelectObject(objectId);
        }

        void UpdatePointSequence(const PlacementToolInfo& toolInfo, const PlacementHit& hit, bool allowKeyboardInput) {
            if (allowKeyboardInput && Hell::Input::KeyDown(HELL_KEY_LEFT_ALT)) {
                if (Hell::Input::MouseWheelUp()) g_sequencePointValue -= toolInfo.sequencePointValueStep;
                if (Hell::Input::MouseWheelDown()) g_sequencePointValue += toolInfo.sequencePointValueStep;
            }

            if (g_placementObjectId != 0 && hit.hitFound) {
                g_sequencePoints.back().position = hit.position;
                g_sequencePoints.back().normal = hit.normal;
                g_sequencePoints.back().value = g_sequencePointValue;
                UpdatePointSequenceObject(g_currentTool);
            }

            if (!Hell::Input::LeftMousePressed() || !hit.hitFound) return;

            if (g_placementObjectId == 0) {
                SequencePoint& sequencePoint = g_sequencePoints.emplace_back();
                sequencePoint.position = hit.position;
                sequencePoint.normal = hit.normal;
                sequencePoint.value = g_sequencePointValue;
            }
            else {
                g_sequencePoints.back().position = hit.position;
                g_sequencePoints.back().normal = hit.normal;
                g_sequencePoints.back().value = g_sequencePointValue;
            }

            const glm::vec3 livePointOffset = toolInfo.rayMode == PlacementRayMode::HEIGHT_MAP ? glm::vec3(0.1f, 0.0f, 0.0f) : hit.normal * 0.1f;
            SequencePoint& liveSequencePoint = g_sequencePoints.emplace_back();
            liveSequencePoint.position = hit.position + livePointOffset;
            liveSequencePoint.normal = hit.normal;
            liveSequencePoint.value = g_sequencePointValue;

            if (g_placementObjectId == 0) g_placementObjectId = CreatePointSequenceObject(g_currentTool, toolInfo);
            else UpdatePointSequenceObject(g_currentTool);

            if (g_placementObjectId == 0) ResetState();
        }
    }

    void Begin(PlacementTool tool) {
        if (tool == PlacementTool::NONE) return;

        const PlacementToolInfo* toolInfo = GetPlacementToolInfo(tool);
        const bool supportedRayMode = toolInfo && (toolInfo->rayMode == PlacementRayMode::WORLD || toolInfo->rayMode == PlacementRayMode::HEIGHT_MAP);
        if (!supportedRayMode || (toolInfo->insertMode != PlacementInsertMode::DIRECT && toolInfo->insertMode != PlacementInsertMode::POINT_SEQUENCE)) {
            Logging::Error() << "EditorPlacement::Begin() only supports direct or point sequence world and height map tools\n";
            Cancel();
            return;
        }

        Cancel();
        Selection::ClearSelection();
        g_currentTool = tool;
        g_sequencePointValue = toolInfo->sequencePointDefaultValue;
    }

    void Update(bool allowKeyboardInput, bool allowMouseInput) {
        if (!IsActive()) return;

        const PlacementToolInfo* toolInfo = GetPlacementToolInfo(g_currentTool);
        if (!toolInfo) {
            Cancel();
            return;
        }

        if (allowKeyboardInput && Hell::Input::KeyPressed(HELL_KEY_ESCAPE)) {
            Cancel();
            return;
        }
        if (allowMouseInput && Hell::Input::RightMousePressed()) {
            if (toolInfo->insertMode == PlacementInsertMode::POINT_SEQUENCE && g_placementObjectId != 0) FinishPointSequence();
            else Cancel();
            return;
        }
        if (!allowMouseInput) return;

        const PlacementHit hit = toolInfo->rayMode == PlacementRayMode::HEIGHT_MAP ? GetTerrainHit() : GetWorldHit();
        if (toolInfo->insertMode == PlacementInsertMode::POINT_SEQUENCE) {
            UpdatePointSequence(*toolInfo, hit, allowKeyboardInput);
            return;
        }
        if (!Hell::Input::LeftMousePressed() || !hit.hitFound) return;

        const uint64_t objectId = CreateDirectObject(g_currentTool, *toolInfo, hit);
        if (objectId == 0) return;

        ResetState();
        Hierarchy::Refresh();
        Selection::SelectObject(objectId);
    }

    void Cancel() {
        if (g_placementObjectId != 0 && World::RemoveObjectById(g_placementObjectId)) WorldBVH::MarkStaticSceneBvhDirty();
        ResetState();
    }

    bool IsActive() {
        return g_currentTool != PlacementTool::NONE;
    }
}
