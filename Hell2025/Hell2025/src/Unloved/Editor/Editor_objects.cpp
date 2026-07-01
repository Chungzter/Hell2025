#include "Editor.h"

#include "Hell/Audio.h"
#include "Hell/Input.h"
#include "Hell/Logging.h"
#include "Hell/Math/Ray.h"
#include "Hell/Physics/Physics.h"

#include "Legacy/Renderer/Renderer.h"
#include "Unloved/Viewport/ViewportManager.h"
#include "Legacy/World/LegacyWorld.h"

#include "Unloved/Debug/DebugDraw.h"
#include "Unloved/Editor/Gizmo.h"
#include "Unloved/ObjectId.h"
#include "Unloved/Session/Session.h"
#include "Unloved/World/World.h"

namespace Audio = Hell::Audio;
namespace Input = Hell::Input;

namespace Unloved::Editor {

    int g_selectedVertexIndex = 0;
    bool g_heightMapMouseHitFound = false;
    glm::vec3 g_heightMapMouseHitPosition = glm::vec3(0.0f);

    void UpdateObjectHover() {
        // Reset values from last frame
        SetHoveredObjectType(ObjectType::NO_TYPE);
        SetHoveredObjectId(0);

        // Bail if there is no hovered viewport
        Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(GetHoveredViewportIndex());
        if (!viewport) return;

        // Cast physx ray
        float maxRayDistance = 2000;
        glm::vec3 rayOrigin = GetMouseRayOriginByViewportIndex(GetHoveredViewportIndex());
        glm::vec3 rayDir = GetMouseRayDirectionByViewportIndex(GetHoveredViewportIndex());
        bool backfaceCulling = BackfaceCullingEnabled();

        //std::cout << "ray origin: " << rayOrigin << "  ray dir: " << rayDir << "\n";

        PhysXRayResult physxRayResult = Hell::Physics::CastPhysXRay(rayOrigin, rayDir, maxRayDistance, backfaceCulling);
        if (physxRayResult.hitFound) {
            ObjectType type = Unloved::GetObjectIdType(physxRayResult.userData.objectId);
            SetHoveredObjectType(type);
            SetHoveredObjectId(physxRayResult.userData.objectId);
            //std::cout << "phyx hit: " << physxRayResult.userData.objectId << "\n";

            //std::cout << "physx hit: " << physxRayResult.userData.objectId << " ";
            //std::cout << Hell::Enum::ToString(Unloved::GetObjectIdType(GetSelectedObjectId())) << " ";
            //std::cout << physxRayResult.hitPosition << " " << glm::distance(physxRayResult.hitPosition, rayOrigin) << "\n";
        }

        // BVH ray
        BvhRayResult bvhRayResult = LegacyWorld::ClosestHit(rayOrigin, rayDir, maxRayDistance);
        if (bvhRayResult.hitFound) {
            float physXDistance = glm::distance(physxRayResult.hitPosition, rayOrigin);
            float bvhDistance = glm::distance(bvhRayResult.hitPosition, rayOrigin);
            if (bvhDistance < physXDistance) {
                SetHoveredObjectType(Unloved::GetObjectIdType(bvhRayResult.objectId));
                SetHoveredObjectId(bvhRayResult.objectId);
            }
            //std::cout << "bvhRayResult hit: " << bvhRayResult.objectId << " ";
            //std::cout << Hell::Enum::ToString(Unloved::GetObjectIdType(GetSelectedObjectId())) << " ";
            //std::cout << bvhRayResult.hitPosition << " " << bvhDistance << "\n";
        }

        if (GetHoveredObjectType() == ObjectType::WALL_SEGMENT) {
            Wall* wall = Unloved::World::GetWallByWallSegmentObjectId(GetHoveredObjectId());
            if (wall) {
                SetHoveredObjectType(ObjectType::WALL);
                SetHoveredObjectId(wall->GetObjectId());
            }
        }

        // Height map mouse position
        Hell::Physics::ActivateAllHeightFields();
        PhysXRayResult heightMapResult = Hell::Physics::CastPhysXRayHeightMap(rayOrigin, rayDir, 10000.0f);
        g_heightMapMouseHitFound = heightMapResult.hitFound;
        g_heightMapMouseHitPosition = heightMapResult.hitPosition;
    }

    void UpdateObjectSelection() {

        //std::cout << "Selected object: " << GetSelectedObjectId() << " " << Hell::Enum::ToString(Unloved::GetObjectIdType(GetSelectedObjectId())) << "\n";

        //switch (Unloved::GetObjectIdType(objectId)) {
        //case ObjectType::DDGI_VOLUME: SetEditorSelectionMode(EditorSelectionMode::OBJECT); break;
        //default: Logging::Warning() << "Editor::SelectObject(..) is missing selection mode implementation for " << Hell::Enum::ToString(Unloved::GetObjectIdType(objectId)) << "\n"; break;
        //}


        //std::cout << Hell::Enum::ToString(GetEditorSelectionMode()) << "\n";


        // Vertex interaction HACK. Find a better place for me
        int viewportIndex = GetHoveredViewportIndex();
        glm::vec3 rayOrigin = GetMouseRayOriginByViewportIndex(viewportIndex);
        glm::vec3 rayDir = GetMouseRayDirectionByViewportIndex(viewportIndex);


        if (GetEditorSelectionMode() != EditorSelectionMode::VERTEX) {
            g_selectedVertexIndex = 0; // maybe -1 is better?
        }

        if (GetSelectedObjectType() == ObjectType::WALL) {
            if (Wall* wall = Unloved::World::GetWallByObjectId(GetSelectedObjectId())) {

                wall->DrawSegmentVertices(OUTLINE_COLOR);
                wall->DrawSegmentLines(OUTLINE_COLOR);


                // Draw hovered verets and HACK to select it
                for (int i = 0; i < wall->GetWallSegments().size(); i++) {
                    WallSegment& wallSegment = wall->GetWallSegments()[i];
                    glm::vec3 position = wallSegment.GetStart();
                    float radius = Editor::GetScalingFactor(10);

                    bool rayHit = Hell::Ray::IntersectSphere(rayOrigin, rayDir, position, radius);

                    if (rayHit) {
                        DebugDraw::DrawPoint(position, WHITE);
                    }

                    if (rayHit && Input::LeftMousePressed()) {
                        g_selectedVertexIndex = i;
                        Audio::PlayAudio(AUDIO_SELECT, 1.0f);
                        Gizmo::SetPosition(position);
                        SetEditorSelectionMode(EditorSelectionMode::VERTEX);
                    }
                }

                if (GetEditorSelectionMode() == EditorSelectionMode::VERTEX) {
                    // Draw selcted vertex
                    WallSegment& wallSegment = wall->GetWallSegments()[g_selectedVertexIndex];
                    glm::vec3 position = wallSegment.GetStart();
                    DebugDraw::DrawPoint(position, YELLOW);


                    if (g_selectedVertexIndex != 0) {
                       // std::cout << "selectedVertexIndex: " << position << "\n";
                    }
                }

            }
        }


        // HACKKK
        if (WorldPlane* plane = Unloved::World::GetHousePlaneByObjectId(GetSelectedObjectId())) {

            plane->DrawEdges(OUTLINE_COLOR);
            plane->DrawVertices(OUTLINE_COLOR);

            // Draw hovered verts and HACK to select it
            for (int i = 0; i < 4; i++) {

                glm::vec3 position = plane->GetVertices()[i].position;
                float radius = Editor::GetScalingFactor(10);

                bool rayHit = Hell::Ray::IntersectSphere(rayOrigin, rayDir, position, radius);

                if (rayHit) {
                    DebugDraw::DrawPoint(position, WHITE);
                }

                if (rayHit && Input::LeftMousePressed()) {
                    g_selectedVertexIndex = i;
                    Audio::PlayAudio(AUDIO_SELECT, 1.0f);
                    Gizmo::SetPosition(position);
                    SetEditorSelectionMode(EditorSelectionMode::VERTEX);
                }
            }

            if (GetEditorSelectionMode() == EditorSelectionMode::VERTEX) {
                // Draw selcted vertex
                glm::vec3 position = plane->GetVertices()[g_selectedVertexIndex].position;
                DebugDraw::DrawPoint(position, YELLOW);
            }

            // is this IF neccesssary? write safer less confusing logic!!!
           // if (GetEditorSelectionMode() == EditorSelectionMode::OBJECT) {
           //     Gizmo::SetPosition(plane->GetWorldSpaceCenter());
           // }
        }


        if (GetEditorState() != EditorState::IDLE) return;
        if (GetEditorSelectionMode() == EditorSelectionMode::VERTEX) return;

        if (Input::LeftMousePressed() && !Gizmo::HasHover() && Input::GetMouseX() > EDITOR_LEFT_PANEL_WIDTH) {
            Audio::PlayAudio(AUDIO_SELECT, 1.0f);
            SetSelectedObjectType(GetHoveredObjectType());
            SetSelectedObjectId(GetHoveredObjectId());

            Gizmo::SetSourceObjectOffeset(LegacyWorld::GetGizmoOffest(GetSelectedObjectId()));

            if (GenericObject* genericObject = Unloved::World::GetGenericObjectById(GetSelectedObjectId())) {
                Gizmo::SetPosition(genericObject->GetPosition());
                Gizmo::SetRotation(genericObject->GetRotation());
            }

            if (Door* door = Unloved::World::GetDoorByObjectId(GetSelectedObjectId())) {
                Gizmo::SetPosition(door->GetPosition());
                Gizmo::SetRotation(door->GetRotation());
            }

            if (Piano* piano = Unloved::World::GetPianoByObjectId(GetSelectedObjectId())) {
                Gizmo::SetPosition(piano->GetPosition());
                Gizmo::SetRotation(piano->GetPosition());
            }

            if (PickUp* pickup = Unloved::World::GetPickUpByObjectId(GetSelectedObjectId())) {
                Gizmo::SetPosition(pickup->GetPosition());
                Gizmo::SetRotation(pickup->GetRotation());
            }

            if (Fireplace* fireplace = Unloved::World::GetFireplaceById(GetSelectedObjectId())) {
                Gizmo::SetPosition(fireplace->GetPosition());
                Gizmo::SetRotation(fireplace->GetRotation());
            }

            if (Staircase* staircase = Unloved::World::GetStaircaseByObjectId(GetSelectedObjectId())) {
                Gizmo::SetPosition(staircase->GetPosition());
                Gizmo::SetRotation(staircase->GetRotation());
            }

            if (GetSelectedObjectType() == ObjectType::HOUSE_PLANE) {
                WorldPlane* plane = Unloved::World::GetHousePlaneByObjectId(GetSelectedObjectId());
                if (plane) {
                    // is this IF neccesssary? write safer less confusing logic!!!
                    if (GetEditorSelectionMode() == EditorSelectionMode::OBJECT) {
                        Gizmo::SetPosition(plane->GetWorldSpaceCenter());
                    }
                }
            }

            if (Ladder* ladder = Unloved::World::GetLadderByObjectId(GetSelectedObjectId())) {
                Gizmo::SetPosition(ladder->GetPosition());
                Gizmo::SetRotation(ladder->GetRotation());
            }

            if (Light* light = Unloved::World::GetLightByObjectId(GetSelectedObjectId())) {
                Gizmo::SetPosition(light->GetPosition());
            }

            if (GetSelectedObjectType() == ObjectType::PICTURE_FRAME) {
                PictureFrame* pictureFrame = Unloved::World::GetPictureFrameByObjectId(GetSelectedObjectId());
                if (pictureFrame) {
                    Gizmo::SetPosition(pictureFrame->GetPosition());
                }
            }

            if (GetSelectedObjectType() == ObjectType::WALL) {
                if (Wall* wall = Unloved::World::GetWallByObjectId(GetSelectedObjectId())) {

                    // is this IF neccesssary? write safer less confusing logic!!!
                    if (GetEditorSelectionMode() == EditorSelectionMode::OBJECT) {
                        Gizmo::SetPosition(wall->GetWorldSpaceCenter());
                    }
                }
            }

            if (GetSelectedObjectType() == ObjectType::WINDOW) {
                Window* window = Unloved::World::GetWindowByObjectId(GetSelectedObjectId());
                if (window) {
                    Gizmo::SetPosition(window->GetPosition());
                }
            }

            //if (GetSelectedObjectType() == ObjectType::TREE) {
            //    Tree* tree = LegacyWorld::GetTreeByObjectId(GetSelectedObjectId());
            //    if (tree) {
            //        Gizmo::SetPosition(tree->GetPosition());
            //    }
            //}
            UpdateOutliner();
        }
    }

    void UpdateObjectGizmoInteraction() {

        UpdateGizmoInteract();

        if (GetEditorState() == EditorState::GIZMO_TRANSLATING) {

            if (GetEditorSelectionMode() == EditorSelectionMode::OBJECT) {
                LegacyWorld::SetObjectPosition(GetSelectedObjectId(), Gizmo::GetPosition());
            }
            else if (GetEditorSelectionMode() == EditorSelectionMode::VERTEX) {


                // HACK
                // HACK
                // HACK
                // HACK
                // HACK
                // HACK
                if (GetSelectedObjectType() == ObjectType::WALL) {
                    if (Wall* wall = Unloved::World::GetWallByObjectId(GetSelectedObjectId())) {
                        if (wall->UpdatePointPosition(g_selectedVertexIndex, Gizmo::GetPosition())) {
                            LegacyWorld::RecreateAllHouseGeometry(); // this could be slow???????????????????????????????
                        }
                    }
                }


                // HACK
                // HACK
                // HACK
                // HACK
                // HACK
                // HACK
                if (GetSelectedObjectType() == ObjectType::HOUSE_PLANE) {
                    if (WorldPlane* plane = Unloved::World::GetHousePlaneByObjectId(GetSelectedObjectId())) {

                        HousePlaneCreateInfo& createInfo = plane->GetCreateInfo();

                        if (g_selectedVertexIndex == 0) {
                            createInfo.p0 = Gizmo::GetPosition();
                        }
                        if (g_selectedVertexIndex == 1) {
                            createInfo.p1 = Gizmo::GetPosition();
                        }
                        if (g_selectedVertexIndex == 2) {
                            createInfo.p2 = Gizmo::GetPosition();
                        }
                        if (g_selectedVertexIndex == 3) {
                            createInfo.p3 = Gizmo::GetPosition();
                        }
                        plane->UpdateVertexDataFromCreateInfo();
                        LegacyWorld::RecreateAllHouseGeometry(); // this could be slow???????????????????????????????
                    }
                }
            }

     

        }
        if (GetEditorState() == EditorState::GIZMO_ROTATING) {
            LegacyWorld::SetObjectRotation(GetSelectedObjectId(), Gizmo::GetRotation());
        }

    }

    void UnselectAnyObject() {
        SetSelectedObjectType(ObjectType::NO_TYPE);
        SetSelectedObjectId(0);
    }

    bool HeightMapMouseHitFound() {
        return g_heightMapMouseHitFound;
    }

    const glm::vec3& GetHeightMapMouseHitPosition() {
        return g_heightMapMouseHitPosition;
    }
}
