#include "Editor_rays.h"

#include "Editor.h"

#include "Hell/Physics/Physics.h"
#include "Unloved/Systems/WorldBVH/WorldBVH.h"
#include "Unloved/Viewport/ViewportManager.h"

namespace Unloved::Editor {

    namespace {
        bool GetHoveredViewportRay(glm::vec3& rayOrigin, glm::vec3& rayDirection) {
            int hoveredViewportIndex = GetHoveredViewportIndex();
            if (!Unloved::ViewportManager::GetViewportByIndex(hoveredViewportIndex)) {
                return false;
            }

            rayOrigin = GetMouseRayOriginByViewportIndex(hoveredViewportIndex);
            rayDirection = GetMouseRayDirectionByViewportIndex(hoveredViewportIndex);
            return true;
        }

        EditorRayResult CreateEditorRayResult(const BvhRayResult& bvhRayResult) {
            EditorRayResult editorRayResult;
            if (!bvhRayResult.hitFound) {
                return editorRayResult;
            }

            editorRayResult.hitFound = bvhRayResult.hitFound;
            editorRayResult.position = bvhRayResult.hitPosition;
            editorRayResult.normal = bvhRayResult.hitNormal;
            editorRayResult.hitObjectId = bvhRayResult.objectId;
            editorRayResult.distanceToHit = bvhRayResult.distanceToHit;
            return editorRayResult;
        }

        EditorRayResult CreateEditorRayResult(const PhysXRayResult& physXRayResult) {
            EditorRayResult editorRayResult;
            if (!physXRayResult.hitFound) {
                return editorRayResult;
            }

            editorRayResult.hitFound = physXRayResult.hitFound;
            editorRayResult.position = physXRayResult.hitPosition;
            editorRayResult.normal = physXRayResult.hitNormal;
            editorRayResult.hitObjectId = physXRayResult.userData.objectId;
            editorRayResult.distanceToHit = physXRayResult.distanceToHit;
            return editorRayResult;
        }
    }

    EditorRayResult GetEditorBvhRayResult(float maxRayDistance) {
        glm::vec3 rayOrigin;
        glm::vec3 rayDirection;
        if (!GetHoveredViewportRay(rayOrigin, rayDirection)) {
            return EditorRayResult();
        }

        return CreateEditorRayResult(Unloved::WorldBVH::ClosestHit(rayOrigin, rayDirection, maxRayDistance));
    }

    EditorRayResult GetEditorHeightMapRayResult(float maxRayDistance) {
        glm::vec3 rayOrigin;
        glm::vec3 rayDirection;
        if (!GetHoveredViewportRay(rayOrigin, rayDirection)) {
            return EditorRayResult();
        }

        Hell::Physics::ActivateAllHeightFields();
        return CreateEditorRayResult(Hell::Physics::CastPhysXRayHeightMap(rayOrigin, rayDirection, maxRayDistance));
    }

    EditorRayResult GetEditorPhysicsRayResult(float maxRayDistance) {
        glm::vec3 rayOrigin;
        glm::vec3 rayDirection;
        if (!GetHoveredViewportRay(rayOrigin, rayDirection)) {
            return EditorRayResult();
        }

        return CreateEditorRayResult(Hell::Physics::CastPhysXRay(rayOrigin, rayDirection, maxRayDistance, Editor::BackfaceCullingEnabled()));
    }

    EditorRayResult GetEditorWorldRayResult(float maxRayDistance) {
        glm::vec3 rayOrigin;
        glm::vec3 rayDirection;
        if (!GetHoveredViewportRay(rayOrigin, rayDirection)) {
            return EditorRayResult();
        }

        EditorRayResult bvhRayResult = CreateEditorRayResult(Unloved::WorldBVH::ClosestHit(rayOrigin, rayDirection, maxRayDistance));
        EditorRayResult physXRayResult = CreateEditorRayResult(Hell::Physics::CastPhysXRay(rayOrigin, rayDirection, maxRayDistance, Editor::BackfaceCullingEnabled()));

        if (physXRayResult.hitFound && physXRayResult.distanceToHit < bvhRayResult.distanceToHit) {
            return physXRayResult;
        }

        return bvhRayResult;
    }
}
