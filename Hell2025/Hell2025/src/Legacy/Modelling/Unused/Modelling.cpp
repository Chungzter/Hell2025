/*#include "Modelling.h"
#include "Hell/Backend/BackEnd.h"
#include "Core/GameOLD.h"
#include "Editor/Editor.h"
#include "Renderer/Renderer.h"
#include "Renderer/RenderDataManager.h"
#include "Viewport/ViewportManager.h"


namespace Modelling {

    EdgeRef g_hoveredEdge;
    EdgeRef g_selectedEdge;
    glm::vec3 g_rayOrigin = glm::vec3(0.0f);
    glm::vec3 g_rayDir = glm::vec3(0.0f);
    glm::vec3 g_viewPos = glm::vec3(0.0f);
    glm::mat4 g_viewMatrix = glm::mat4(1.0f);
    float g_mouseX = 0.0f;
    float g_mouseY = 0.0f;
    Viewport* g_activeViewport = nullptr;

    void UpdateViewportData();
    void UpdateFaceHover();
    void UpdateEdgeHover();
    void UpdateFaceHover();
    void RenderPotentialEdgeLoop();

    void Init() {
        return;
        g_editableMesh.InitAsCube(glm::vec3(17.0f, 1.0f, 18.0f));
    }

    void Update() {
        return;
        const std::vector<ViewportData>& viewportData = RenderDataManager::GetViewportData();
        if (viewportData.empty()) return;

        EditableMesh& mesh = g_editableMesh;
        mesh.SubmitVerticesForRendering();
        mesh.SubmitEdgesForRendering();
        mesh.BuildInternalMeshData();

        UpdateViewportData();
        UpdateFaceHover();
        UpdateEdgeHover();
        RenderPotentialEdgeLoop();

        if (Input::KeyPressed(HELL_KEY_HOME)) {
            mesh.CleanUp();
            mesh.InitAsCube(glm::vec3(17.0f, 1.0f, 18.0f));
        }
    }

    void UpdateFaceHover() {
        EditableMesh& mesh = g_editableMesh;

        int faceIndex = -1;
        float closestDistance = std::numeric_limits<float>::max();

        for (int i = 0; i < static_cast<int>(mesh.m_faces.size()); i++) {
            Face* face = mesh.m_faces[i];
            if (!face) continue;

            // Retrieve positions from the face's vertices.
            glm::vec3 p0 = face->vertices[0]->position;
            glm::vec3 p1 = face->vertices[1]->position;
            glm::vec3 p2 = face->vertices[2]->position;
            glm::vec3 p3 = face->vertices[3]->position;

            // Test two triangles: (p0, p1, p2) and (p0, p2, p3).
            float tA, tB;
            bool hitA = Util::RayIntersectsTriangle(g_rayOrigin, g_rayDir, p0, p1, p2, tA);
            bool hitB = Util::RayIntersectsTriangle(g_rayOrigin, g_rayDir, p0, p2, p3, tB);

            float distanceToHit = std::numeric_limits<float>::max();
            if (hitA && tA < distanceToHit)
                distanceToHit = tA;
            if (hitB && tB < distanceToHit)
                distanceToHit = tB;

            // If this face is hit and is the closest so far, update.
            if (distanceToHit < closestDistance) {
                closestDistance = distanceToHit;
                faceIndex = i;
            }
        }

        // If we found a hit, draw only that face's outline.
        if (faceIndex != -1) {
            Face* face = mesh.m_faces[faceIndex];
            glm::vec3 p0 = face->vertices[0]->position;
            glm::vec3 p1 = face->vertices[1]->position;
            glm::vec3 p2 = face->vertices[2]->position;
            glm::vec3 p3 = face->vertices[3]->position;

            // Draw the outline in yellow.
            DebugDraw::DrawLine(p0, p1, YELLOW);
            DebugDraw::DrawLine(p1, p2, YELLOW);
            DebugDraw::DrawLine(p2, p3, YELLOW);
            DebugDraw::DrawLine(p3, p0, YELLOW);

            // If the left mouse button is pressed, delete the face.
            if (Input::LeftMousePressed()) {
                mesh.DeleteFace(faceIndex);
                std::cout << "deleted face: " << faceIndex << "\n";
            }
        }
    }

    void UpdateEdgeHover() {
        EditableMesh& mesh = g_editableMesh;

        g_hoveredEdge.face = nullptr;
        g_hoveredEdge.edgeIndex = -1;

        glm::vec2 mousePos(g_mouseX, g_mouseY);
        float closestDistanceToEdgeCenter = std::numeric_limits<float>::max();
        int threshold = 10;

        // Iterate over each face.
        for (Face* face : mesh.m_faces) {
            if (!face) continue;

            // For each face, iterate over its 4 edges
            for (int i = 0; i < 4; i++) {
                int next = (i + 1) % 4;
                glm::vec3 endpoint1 = face->vertices[i]->position;
                glm::vec3 endpoint2 = face->vertices[next]->position;

                // Convert endpoints to screen space.
                glm::vec2 sp1 = g_activeViewport->WorldToScreen(g_viewMatrix, endpoint1); // WARNING! this is only correct for ortho projection
                glm::vec2 sp2 = g_activeViewport->WorldToScreen(g_viewMatrix, endpoint2); // WARNING! this is only correct for ortho projection

                // Compute the edge vector in screen space
                glm::vec2 edgeVec = sp2 - sp1;
                float edgeLengthSq = glm::dot(edgeVec, edgeVec);

                // Project the mouse position onto the edge
                float t = 0.0f;
                if (edgeLengthSq > 0.0f) {
                    t = glm::clamp(glm::dot(mousePos - sp1, edgeVec) / edgeLengthSq, 0.0f, 1.0f);
                }
                glm::vec2 projPoint = sp1 + t * edgeVec;

                // Compute the distance from the mouse to the edge.
                float dist = glm::length(mousePos - projPoint);
                if (dist < threshold) {
                    glm::vec3 edgeCenter = (endpoint1 + endpoint2) * 0.5f;
                    float distanceToEdgeCenter = glm::distance(edgeCenter, g_viewPos);

                    // If it is closer than the any previously hovered edge, update it
                    if (distanceToEdgeCenter < closestDistanceToEdgeCenter) {
                        closestDistanceToEdgeCenter = distanceToEdgeCenter;
                        g_hoveredEdge.face = face;
                        g_hoveredEdge.edgeIndex = i;
                    }
                }
            }
        }


        // For example, to visualize the hovered edge:
        if (g_hoveredEdge.face) {
            int i = g_hoveredEdge.edgeIndex;
            int next = (i + 1) % 4;
            glm::vec3 a = g_hoveredEdge.face->vertices[i]->position;
            glm::vec3 b = g_hoveredEdge.face->vertices[next]->position;
            DebugDraw::DrawLine(a, b, RED);

            // And if needed, trigger extrusion when clicking, etc.
            if (Input::RightMousePressed()) {

                if (mesh.IsBoundaryEdge(g_hoveredEdge.face, g_hoveredEdge.edgeIndex)) {
                    mesh.ExtrudeEdge(&g_hoveredEdge, 0.1f);
                    std::cout << "Extruding edge " << g_hoveredEdge.edgeIndex << " of hovered face " << g_hoveredEdge.face << "\n";
                }
                else {
                    std::cout << "Hovered edge is NOT a boundary edge.\n";
                }

            }
        }
    }

    void UpdateEdgeSelect() {

    }

    float ComputeParameterOnEdgeFromRay(EdgeRef edgeRef, glm::vec3 rayOrigin, glm::vec3 rayDir) {
        if (!edgeRef.IsValid()) {
            std::cout << "ComputeParameterOnEdgeFromRay() failed: invalid edge ref\n";
            return 0.0f;
        }

        int i = edgeRef.edgeIndex;
        int next = (i + 1) % 4;
        glm::vec3 P0 = edgeRef.face->vertices[i]->position;
        glm::vec3 P1 = edgeRef.face->vertices[next]->position;
        glm::vec3 u = P1 - P0;
        float a = glm::dot(u, u);
        float b = glm::dot(u, rayDir);
        float c = glm::dot(rayDir, rayDir);  // Should be 1 if rayDir is normalized
        glm::vec3 w0 = P0 - rayOrigin;
        float d = glm::dot(u, w0);
        float e = glm::dot(g_rayDir, w0);

        float denom = a * c - b * b;
        float t = 0.0f;
        if (glm::abs(denom) < 1e-6f)
            t = 0.0f; // Lines are nearly parallel
        else
            t = (b * e - c * d) / denom;

        // Clamp t to the edge segment
        return glm::clamp(t, 0.0f, 1.0f);
    }

    glm::vec3 ComputePointOnEdge(EdgeRef edgeRef, float t) {
        if (!edgeRef.IsValid()) {
            std::cout << "ComputePointOnEdge() failed: invalid edge ref\n";
            return glm::vec3(0.0f);
        }

        int next = (g_hoveredEdge.edgeIndex + 1) % 4;
        glm::vec3 P0 = g_hoveredEdge.face->vertices[g_hoveredEdge.edgeIndex]->position;
        glm::vec3 P1 = g_hoveredEdge.face->vertices[next]->position;
        return P0 + t * (P1 - P0);
    }

    float ComputeParameterOnEdgeFromReferenceRay(const EdgeRef& edge,
                                                 const glm::vec3& R,
                                                 const glm::vec3& refDir) {
        EditableVertex* v0 = edge.face->vertices[edge.edgeIndex];
        EditableVertex* v1 = edge.face->vertices[(edge.edgeIndex + 1) % 4];
        glm::vec3 P0 = v0->position;
        glm::vec3 P1 = v1->position;

        glm::vec3 u = P1 - P0;       // Direction of the edge.
        float uu = glm::dot(u, u);
        if (uu < 1e-6f)
            return 0.0f;  // Degenerate edge.

        // The reference ray is L2(s) = R + s * refDir (assumed normalized).
        glm::vec3 v = refDir;        // v should be normalized.
        float vv = glm::dot(v, v);   // Should be 1.
        float uv = glm::dot(u, v);
        glm::vec3 w0 = P0 - R;

        // Denom = (u�u)(v�v) - (u�v)�.
        float denom = uu * vv - uv * uv;
        if (fabs(denom) < 1e-6f)
            return 0.0f;

        // Standard formula:
        float t = (glm::dot(w0, u) * vv - glm::dot(w0, v) * uv) / denom;
        return glm::clamp(t, 0.0f, 1.0f);
    }

    void RenderPotentialEdgeLoop() {
        // If there's no valid hovered edge, do nothing.
        if (g_hoveredEdge.edgeIndex == -1)
            return;


        EditableMesh& mesh = g_editableMesh;
        EdgeRef startEdge = g_hoveredEdge;

        std::vector<EdgeRef> loopEdges = mesh.GetLoopEdges(&startEdge);

        float t = ComputeParameterOnEdgeFromRay(g_hoveredEdge, g_rayOrigin, g_rayDir);

        glm::vec3 refPoint = ComputePointOnEdge(g_hoveredEdge, t);
        glm::vec3 refNormal = Util::ComputeFaceNormal(
            g_hoveredEdge.face->vertices[0]->position,
            g_hoveredEdge.face->vertices[1]->position,
            g_hoveredEdge.face->vertices[2]->position
        );


        // Gather unique edges
        std::vector<EdgeRef> uniqueEdges;
        for (const EdgeRef& edge : loopEdges) {

            glm::vec3 p1 = edge.face->vertices[edge.edgeIndex]->position;
            glm::vec3 p2 = edge.face->vertices[(edge.edgeIndex + 1) % 4]->position;

            bool found = false;
            for (EdgeRef& uniqueEdge : uniqueEdges) {

                glm::vec3 uniqueP1 = uniqueEdge.face->vertices[uniqueEdge.edgeIndex]->position;
                glm::vec3 uniqueP2 = uniqueEdge.face->vertices[(uniqueEdge.edgeIndex + 1) % 4]->position;

                if (p1 == uniqueP1 && p2 == uniqueP2 ||
                    p1 == uniqueP2 && p2 == uniqueP1) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                uniqueEdges.push_back(edge);
            }
        }

        // Get intersection points
        std::vector<glm::vec3> points;
        points.reserve(uniqueEdges.size());
        for (EdgeRef& edge : uniqueEdges) {
            int next = (edge.edgeIndex + 1) % 4;
            glm::vec3 P0 = edge.face->vertices[edge.edgeIndex]->position;
            glm::vec3 P1 = edge.face->vertices[next]->position;
            const glm::vec3& rayOrigin = refPoint;
            const glm::vec3& rayDir = refNormal;
            glm::vec3 intersection = Util::ClosestPointOnSegmentToRay(P0, P1, rayOrigin, rayDir);
            points.push_back(intersection);
            DebugDraw::DrawPoint(intersection, GREEN);
        }


        // Draw connecting lines between consecutive edges
        for (size_t i = 0; i + 1 < points.size(); i++) {
            DebugDraw::DrawLine(points[i], points[i + 1], GREEN);
        }

        if (Input::MiddleMousePressed()) {

            std::cout << "loopEdges.size():   " << loopEdges.size() << "\n";
            std::cout << "uniqueEdges.size(): " << uniqueEdges.size() << "\n";

            std::cout << "face count before: " << mesh.m_faces.size() << "\n";
            mesh.InsertEdgeLoop(uniqueEdges, points);
            std::cout << "face count after:  " << mesh.m_faces.size() << "\n";
        }
    }

    void UpdateViewportData() {
        const std::vector<ViewportData>& viewportData = RenderDataManager::GetViewportData();
        if (viewportData.empty()) {
            return;
        }

        int32_t viewportIndex = Editor::GetHoveredViewportIndex();
        g_activeViewport = ViewportManager::GetViewportByIndex(viewportIndex);
        SpaceCoords gBufferSpaceCoords = g_activeViewport->GetGBufferSpaceCoords();

        if (Editor::IsOpen()) {
            int32_t viewportIndex = Editor::GetHoveredViewportIndex();
            g_viewMatrix = viewportData[viewportIndex].view;
            g_viewPos = viewportData[viewportIndex].inverseView[3];
            g_rayOrigin = Editor::GetMouseRayOriginByViewportIndex(viewportIndex);
            g_rayDir = Editor::GetMouseRayDirectionByViewportIndex(viewportIndex);
            g_mouseX = gBufferSpaceCoords.localMouseX;
            g_mouseY = gBufferSpaceCoords.localMouseY;
        }
        else {
            g_viewMatrix = viewportData[0].view;
            g_viewPos = viewportData[0].inverseView[3];
            g_rayOrigin = viewportData[0].inverseView[3];
            g_rayDir = Game::GetLocalPlayerByIndex(0)->GetCameraForward();
            g_mouseX = gBufferSpaceCoords.width * 0.5f;
            g_mouseY = gBufferSpaceCoords.height * 0.5f;
        }
    }

    EditableMesh& GetEditableMesh() {
        return g_editableMesh;
    }
}
*/