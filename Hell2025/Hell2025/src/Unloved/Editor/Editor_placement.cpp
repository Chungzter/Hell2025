#include "Editor.h"
#include "Editor_placement.h"
#include "Editor_rays.h"
#include "PlacementObjectCreation.h"
#include "PlacementSequencePoints.h"

#include "Hell/Input.h"

namespace Input = Hell::Input;

namespace Unloved::Editor {

    namespace {
        PlacementTool g_currentPlacementTool = PlacementTool::NONE;
        std::vector<SequencePoint> g_sequencePoints;
        uint64_t g_placementObjectId = 0;
        float g_sequencePointValue = 0;
    }

    void BeginPlacement(PlacementTool placementTool) {
        if (const PlacementToolInfo* toolInfo = GetPlacementToolInfo(placementTool)) {
            g_currentPlacementTool = placementTool;
            g_placementObjectId = 0;
            g_sequencePointValue = toolInfo->sequencePointDefaultValue;
            g_sequencePoints.clear();

            SetEditorState(EditorState::PLACEMENT);
        }
        else {
            CancelPlacement();
        }
    }

    void UpdatePlacement() {
        // Right click to exit object placement
        if (Input::RightMousePressed()) {
            if (g_placementObjectId != 0) {
                FinishPlacement();
            }
            else {
                CancelPlacement();
            }
            return;
        }

        // Bail if tool is invalid
        if (g_currentPlacementTool == PlacementTool::NONE) {
            CancelPlacement();
            return;
        }

        // Bail if no tool info for the current tool
        const PlacementToolInfo* toolInfo = GetPlacementToolInfo(g_currentPlacementTool);
        if (!toolInfo) {
            CancelPlacement();
            return;
        }

        // Get ray hit
        EditorRayResult rayResult;
        if (toolInfo->rayMode == PlacementRayMode::HEIGHT_MAP) rayResult = GetEditorHeightMapRayResult();
        if (toolInfo->rayMode == PlacementRayMode::WORLD)      rayResult = GetEditorWorldRayResult();

        // Direct objects
        if (toolInfo->insertMode == PlacementInsertMode::DIRECT) {

            // Place direct object
            if (Input::LeftMousePressed() && rayResult.hitFound) {
                PlaceDirectObject(g_currentPlacementTool, rayResult, *toolInfo);
                FinishPlacement();
            }
        }
        
        // Point sequence objects
        if (toolInfo->insertMode == PlacementInsertMode::POINT_SEQUENCE) {
            glm::vec3 livePointOffset = toolInfo->rayMode == PlacementRayMode::HEIGHT_MAP ? glm::vec3(0.1f, 0.0f, 0.0f) : rayResult.normal * 0.1f;

            // Sequence point value
            if (Input::KeyDown(HELL_KEY_LEFT_ALT)) {

                // Update editor value
                if (Input::MouseWheelUp()) {
                    g_sequencePointValue -= toolInfo->sequencePointValueStep;
                }
                if (Input::MouseWheelDown()) {
                    g_sequencePointValue += toolInfo->sequencePointValueStep;
                }
            }

            // Update live point
            if (g_placementObjectId != 0 && rayResult.hitFound) {
                g_sequencePoints.back().position = rayResult.position;
                g_sequencePoints.back().normal = rayResult.normal;
                g_sequencePoints.back().value = g_sequencePointValue;
                UpdatePointSequenceObject(g_currentPlacementTool, g_placementObjectId, g_sequencePoints);
            }

            // Place next point
            if (Input::LeftMousePressed() && rayResult.hitFound) {
                if (g_placementObjectId == 0) {
                    SequencePoint& sequencePoint = g_sequencePoints.emplace_back();
                    sequencePoint.position = rayResult.position;
                    sequencePoint.normal = rayResult.normal;
                    sequencePoint.value = g_sequencePointValue;

                    SequencePoint& liveSequencePoint = g_sequencePoints.emplace_back();
                    liveSequencePoint.position = rayResult.position + livePointOffset;
                    liveSequencePoint.normal = rayResult.normal;
                    liveSequencePoint.value = g_sequencePointValue;

                    g_placementObjectId = CreatePointSequenceObject(g_currentPlacementTool, g_sequencePoints, *toolInfo);
                }
                else {
                    g_sequencePoints.back().position = rayResult.position;
                    g_sequencePoints.back().normal = rayResult.normal;
                    g_sequencePoints.back().value = g_sequencePointValue;

                    SequencePoint& liveSequencePoint = g_sequencePoints.emplace_back();
                    liveSequencePoint.position = rayResult.position + livePointOffset;
                    liveSequencePoint.normal = rayResult.normal;
                    liveSequencePoint.value = g_sequencePointValue;

                    UpdatePointSequenceObject(g_currentPlacementTool, g_placementObjectId, g_sequencePoints);
                }
            }
        }
    }

    void CancelPlacement() {
        g_currentPlacementTool = PlacementTool::NONE;
        g_placementObjectId = 0;
        g_sequencePointValue = 0;
        g_sequencePoints.clear();

        SetEditorState(EditorState::IDLE);
    }

    void FinishPlacement() {
        g_currentPlacementTool = PlacementTool::NONE;
        g_placementObjectId = 0;
        g_sequencePointValue = 0;
        g_sequencePoints.clear();

        SetEditorState(EditorState::IDLE);
        UpdateOutliner();
    }
}
