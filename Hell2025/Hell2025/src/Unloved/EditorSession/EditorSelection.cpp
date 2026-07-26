#include "EditorSelection.h"

#include "EditorInputElements.h"
#include "EditorViewports.h"

#include "Hell/Audio.h"
#include "Hell/Input.h"
#include "Hell/Physics/Physics.h"
#include "Unloved/Common/Constants.h"
#include "Unloved/Editor/Gizmo.h"
#include "Unloved/ObjectId.h"
#include "Unloved/Objects/House/Wall.h"
#include "Unloved/Objects/Props/Christmas/ChristmasLights.h"
#include "Unloved/Objects/Renderables/AnimatedGameObject.h"
#include "Unloved/Systems/WorldBVH/WorldBVH.h"
#include "Unloved/World/World.h"

#include <limits>
#include <vector>

namespace Unloved::EditorSession::Selection {
    namespace {
        constexpr float MAX_PICK_DISTANCE = 2000.0f;
        constexpr bool CULL_BACK_FACING = true;

        struct EditorPickResult {
            uint64_t objectId = 0;
            float distanceToHit = std::numeric_limits<float>::max();
        };

        uint64_t g_hoveredObjectId = 0;
        uint64_t g_selectedObjectId = 0;
        int32_t g_selectedChristmasLightPointIndex = -1;
        int32_t g_selectedWallSegmentIndex = -1;
        bool g_gizmoWasDragging = false;

        EditorPickResult PickClosestObject(uint32_t viewportIndex) {
            const glm::vec3& rayOrigin = Viewports::GetMouseRayOrigin(viewportIndex);
            const glm::vec3& rayDirection = Viewports::GetMouseRayDirection(viewportIndex);
            const BvhRayResult bvhResult = Unloved::WorldBVH::ClosestHit(rayOrigin, rayDirection, MAX_PICK_DISTANCE);
            const PhysXRayResult physXResult = Hell::Physics::CastPhysXRay(rayOrigin, rayDirection, MAX_PICK_DISTANCE, CULL_BACK_FACING);
            EditorPickResult result;

            if (bvhResult.hitFound) {
                result.objectId = bvhResult.objectId;
                result.distanceToHit = bvhResult.distanceToHit;
            }
            if (physXResult.hitFound && physXResult.distanceToHit < result.distanceToHit) {
                result.objectId = physXResult.userData.objectId;
                result.distanceToHit = physXResult.distanceToHit;
            }
            return result;
        }

        uint64_t ResolveSelectableObjectId(uint64_t objectId) {
            if (Unloved::GetObjectIdType(objectId) == ObjectType::ANIMATED_GAME_OBJECT) {
                AnimatedGameObject* animatedGameObject = World::GetAnimatedGameObjectByObjectId(objectId);
                return animatedGameObject ? animatedGameObject->GetOwnerObjectId() : 0;
            }
            if (Unloved::GetObjectIdType(objectId) != ObjectType::WALL_SEGMENT) return objectId;

            Wall* wall = Unloved::World::GetWallByWallSegmentObjectId(objectId);
            return wall ? wall->GetObjectId() : 0;
        }

        bool ObjectUsesRotation(uint64_t objectId) {
            switch (Unloved::GetObjectIdType(objectId)) {
                case ObjectType::ANIMATED_GAME_OBJECT:
                case ObjectType::CHRISTMAS_LIGHTS:
                case ObjectType::SHARK:
                case ObjectType::WALL:
                case ObjectType::WORLD_PLANE:
                    return false;
                default:
                    return true;
            }
        }

        void UpdateSelectedObjectFromGizmo() {
            if (g_selectedObjectId == 0) return;
            if (g_selectedWallSegmentIndex >= 0) return;

            if (g_selectedChristmasLightPointIndex >= 0) {
                ChristmasLightSet* christmasLights = World::GetChristmasLightsByObjectId(g_selectedObjectId);
                if (!christmasLights || Gizmo::GetMode() != GizmoMode::TRANSLATE) return;

                std::vector<SequencePoint> sequencePoints = christmasLights->GetCreateInfo().sequencePoints;
                if (g_selectedChristmasLightPointIndex >= static_cast<int32_t>(sequencePoints.size())) return;
                if (sequencePoints[g_selectedChristmasLightPointIndex].position == Gizmo::GetPosition()) return;

                sequencePoints[g_selectedChristmasLightPointIndex].position = Gizmo::GetPosition();
                christmasLights->UpdateSequencePoints(sequencePoints);
                return;
            }

            if (Gizmo::GetMode() == GizmoMode::TRANSLATE) Unloved::World::SetPositionById(g_selectedObjectId, Gizmo::GetPosition());
            if (Gizmo::GetMode() == GizmoMode::ROTATE) Unloved::World::SetRotationById(g_selectedObjectId, Gizmo::GetRotation());
        }
    }

    void Reset() {
        g_hoveredObjectId = 0;
        g_selectedObjectId = 0;
        g_selectedChristmasLightPointIndex = -1;
        g_selectedWallSegmentIndex = -1;
        g_gizmoWasDragging = false;
    }

    void Update(bool allowInput) {
        const bool gizmoDragging = Gizmo::GetAction() == GizmoAction::DRAGGING;
        if (gizmoDragging || g_gizmoWasDragging) UpdateSelectedObjectFromGizmo();
        g_gizmoWasDragging = gizmoDragging;
        g_hoveredObjectId = 0;

        if (!allowInput || Viewports::IsFlyMode() || gizmoDragging) return;

        const int32_t viewportIndex = Viewports::GetHoveredViewportIndex();
        if (viewportIndex < 0) return;

        g_hoveredObjectId = ResolveSelectableObjectId(PickClosestObject(static_cast<uint32_t>(viewportIndex)).objectId);
        if (!Hell::Input::LeftMousePressed() || Gizmo::HasHover()) return;

        if (g_hoveredObjectId == 0) ClearSelection();
        else SelectObject(g_hoveredObjectId);
    }

    void SelectObject(uint64_t objectId) {
        objectId = ResolveSelectableObjectId(objectId);
        if (objectId == 0) {
            ClearSelection();
            return;
        }

        InputElements::Reset();
        g_selectedObjectId = objectId;
        g_selectedChristmasLightPointIndex = -1;
        g_selectedWallSegmentIndex = -1;
        Gizmo::SetPosition(Unloved::World::GetPositionById(objectId));
        Gizmo::SetRotation(ObjectUsesRotation(objectId) ? Unloved::World::GetRotationById(objectId) : glm::vec3(0.0f));
        Gizmo::SetSourceObjectOffeset(glm::vec3(0.0f));
        Hell::Audio::PlayAudio(AUDIO_SELECT, 1.0f);
    }

    void SelectChristmasLightPoint(uint64_t objectId, int32_t pointIndex) {
        ChristmasLightSet* christmasLights = World::GetChristmasLightsByObjectId(objectId);
        if (!christmasLights || pointIndex < 0 || pointIndex >= static_cast<int32_t>(christmasLights->GetCreateInfo().sequencePoints.size())) {
            ClearSelection();
            return;
        }

        InputElements::Reset();
        g_selectedObjectId = objectId;
        g_selectedChristmasLightPointIndex = pointIndex;
        g_selectedWallSegmentIndex = -1;
        Gizmo::SetPosition(christmasLights->GetCreateInfo().sequencePoints[pointIndex].position);
        Gizmo::SetRotation(glm::vec3(0.0f));
        Gizmo::SetSourceObjectOffeset(glm::vec3(0.0f));
        Hell::Audio::PlayAudio(AUDIO_SELECT, 1.0f);
    }

    void SelectWallSegment(uint64_t objectId, int32_t segmentIndex) {
        Wall* wall = World::GetWallByObjectId(objectId);
        if (!wall || segmentIndex < 0 || segmentIndex >= static_cast<int32_t>(wall->GetWallSegments().size())) {
            ClearSelection();
            return;
        }

        const WallSegment& wallSegment = wall->GetWallSegments()[segmentIndex];
        InputElements::Reset();
        g_selectedObjectId = objectId;
        g_selectedChristmasLightPointIndex = -1;
        g_selectedWallSegmentIndex = segmentIndex;
        Gizmo::SetPosition((wallSegment.GetStart() + wallSegment.GetEnd()) * 0.5f);
        Gizmo::SetRotation(glm::vec3(0.0f));
        Gizmo::SetSourceObjectOffeset(glm::vec3(0.0f));
        Hell::Audio::PlayAudio(AUDIO_SELECT, 1.0f);
    }

    bool DeleteSelected() {
        if (g_selectedObjectId == 0) return false;
        if (g_selectedWallSegmentIndex >= 0) return false;

        if (g_selectedChristmasLightPointIndex >= 0) {
            ChristmasLightSet* christmasLights = World::GetChristmasLightsByObjectId(g_selectedObjectId);
            if (!christmasLights || g_selectedChristmasLightPointIndex >= static_cast<int32_t>(christmasLights->GetCreateInfo().sequencePoints.size())) return false;

            std::vector<SequencePoint> sequencePoints = christmasLights->GetCreateInfo().sequencePoints;
            sequencePoints.erase(sequencePoints.begin() + g_selectedChristmasLightPointIndex);
            christmasLights->UpdateSequencePoints(sequencePoints);
        }
        else if (!World::RemoveObjectById(g_selectedObjectId)) {
            return false;
        }

        ClearSelection();
        return true;
    }

    void ClearSelection() {
        g_selectedObjectId = 0;
        g_selectedChristmasLightPointIndex = -1;
        g_selectedWallSegmentIndex = -1;
        g_gizmoWasDragging = false;
        Gizmo::CancelInteraction();
    }

    uint64_t GetHoveredObjectId() {
        return g_hoveredObjectId;
    }

    uint64_t GetSelectedObjectId() {
        return g_selectedObjectId;
    }

    int32_t GetSelectedChristmasLightPointIndex() {
        return g_selectedChristmasLightPointIndex;
    }

    int32_t GetSelectedWallSegmentIndex() {
        return g_selectedWallSegmentIndex;
    }

    bool HasSelectedChristmasLightPoint() {
        return g_selectedObjectId != 0 && g_selectedChristmasLightPointIndex >= 0;
    }

    bool HasSelectedWallSegment() {
        return g_selectedObjectId != 0 && g_selectedWallSegmentIndex >= 0;
    }

    bool HasSelection() {
        return g_selectedObjectId != 0;
    }

    bool ShouldOutlineObject(uint64_t objectId) {
        if (g_selectedObjectId == 0 || objectId == 0) return false;

        if (g_selectedWallSegmentIndex >= 0) {
            Wall* wall = World::GetWallByObjectId(g_selectedObjectId);
            if (!wall || g_selectedWallSegmentIndex >= static_cast<int32_t>(wall->GetWallSegments().size())) return false;
            return wall->GetWallSegments()[g_selectedWallSegmentIndex].GetObjectId() == objectId;
        }

        if (objectId == g_selectedObjectId) return true;
        if (Unloved::GetObjectIdType(g_selectedObjectId) != ObjectType::WALL) return false;
        if (Unloved::GetObjectIdType(objectId) != ObjectType::WALL_SEGMENT) return false;

        Wall* wall = World::GetWallByObjectId(g_selectedObjectId);
        if (!wall) return false;
        for (const WallSegment& wallSegment : wall->GetWallSegments()) {
            if (wallSegment.GetObjectId() == objectId) return true;
        }
        return false;
    }
}
