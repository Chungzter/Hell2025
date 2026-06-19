/*#include "EditableMesh.h"
#include "Renderer/Renderer.h"
#include "Util/Util.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <map>

void EditableMesh::InitAsCube(const glm::vec3& spawnOffset) {
    CleanUp();

    // Create 8 vertices for the cube.
    // Cube spans from (-0.5, -0.5, -0.5) to (0.5, 0.5, 0.5)
    std::vector<EditableVertex*> verts(8);
    for (int i = 0; i < 8; ++i)
        verts[i] = new EditableVertex();

    verts[0]->position = glm::vec3(-0.5f, -0.5f, -0.5f);
    verts[1]->position = glm::vec3(0.5f, -0.5f, -0.5f);
    verts[2]->position = glm::vec3(0.5f, 0.5f, -0.5f);
    verts[3]->position = glm::vec3(-0.5f, 0.5f, -0.5f);
    verts[4]->position = glm::vec3(-0.5f, -0.5f, 0.5f);
    verts[5]->position = glm::vec3(0.5f, -0.5f, 0.5f);
    verts[6]->position = glm::vec3(0.5f, 0.5f, 0.5f);
    verts[7]->position = glm::vec3(-0.5f, 0.5f, 0.5f);

    // Offset vertices by spawnOffset and add them to m_vertices.
    for (EditableVertex* v : verts) {
        v->position += spawnOffset;
        m_vertices.push_back(v);
    }

    // Create 6 faces with explicit, consistent vertex ordering.

    // Bottom face (y = -0.5, outward normal = (0, -1, 0)):
    // Use vertices: v0, v1, v5, v4.
    {
        Face* bottom = new Face();
        bottom->vertices = { verts[0], verts[1], verts[5], verts[4] };
        m_faces.push_back(bottom);
    }

    // Top face (y = +0.5, outward normal = (0, 1, 0)):
    // Use vertices: v3, v7, v6, v2.
    {
        Face* top = new Face();
        top->vertices = { verts[3], verts[7], verts[6], verts[2] };
        m_faces.push_back(top);
    }

    // Front face (z = +0.5, outward normal = (0, 0, 1)):
    // Use vertices: v4, v5, v6, v7.
    {
        Face* front = new Face();
        front->vertices = { verts[4], verts[5], verts[6], verts[7] };
        m_faces.push_back(front);
    }

    // Back face (z = -0.5, outward normal = (0, 0, -1)):
    // Use vertices: v0, v3, v2, v1.
    {
        Face* back = new Face();
        back->vertices = { verts[0], verts[3], verts[2], verts[1] };
        m_faces.push_back(back);
    }

    // Left face (x = -0.5, outward normal = (-1, 0, 0)):
    // Use vertices: v0, v4, v7, v3.
    {
        Face* left = new Face();
        left->vertices = { verts[0], verts[4], verts[7], verts[3] };
        m_faces.push_back(left);
    }

    // Right face (x = +0.5, outward normal = (1, 0, 0)):
    // Use vertices: v1, v2, v6, v5.
    {
        Face* right = new Face();
        right->vertices = { verts[1], verts[2], verts[6], verts[5] };
        m_faces.push_back(right);
    }
}

std::vector<EdgeRef> EditableMesh::GetLoopEdges(EdgeRef* startingEdge) {

    std::vector<EdgeRef> loop;
    if (!startingEdge || !startingEdge->IsValid()) {
        std::cout << "GetLoopEdges() failed: starting edge is invalid\n";
        return loop;
    }

    EdgeRef start = *startingEdge;
    EdgeRef current = start;

    while (true) {
        loop.push_back(current);

        // 1) For a quad face, the edge opposite the current edge is at index (edgeIndex + 2) % 4.
        int oppIdx = (current.edgeIndex + 2) % 4;
        EditableVertex* vA = current.face->vertices[oppIdx];
        EditableVertex* vB = current.face->vertices[(oppIdx + 1) % 4];

        // 2) Try to find an adjacent face that shares the edge (vA, vB) (in either order).
        Face* adjacent = nullptr;
        int adjacentEdgeIdx = -1;
        for (Face* f : m_faces) {
            if (f == current.face)
                continue;
            for (int i = 0; i < 4; i++) {
                int ni = (i + 1) % 4;
                EditableVertex* fvA = f->vertices[i];
                EditableVertex* fvB = f->vertices[ni];
                if ((fvA == vA && fvB == vB) || (fvA == vB && fvB == vA)) {
                    adjacent = f;
                    adjacentEdgeIdx = i;
                    break;
                }
            }
            if (adjacent)
                break;
        }

        EdgeRef nextEdge;
        if (adjacent) {
            // Found an adjacent face; jump across to that face.
            nextEdge.face = adjacent;
            nextEdge.edgeIndex = adjacentEdgeIdx;
        }
        else {
            // No adjacent face: current face is isolated along this edge.
            // In that case, continue along the same face by taking the next edge.
            nextEdge.face = current.face;
            nextEdge.edgeIndex = (current.edgeIndex + 2) % 4;
        }

        // 3) Check for loop closure.
        if (nextEdge == start) {
            loop.push_back(*startingEdge);
            //std::cout << "Loop closed back at the start\n";
            break;
        }
        // 4) Avoid infinite loops by checking if we've already visited this edge.
        if (std::find(loop.begin(), loop.end(), nextEdge) != loop.end()) {
            //std::cout << "Already visited this edge; stopping to avoid infinite loop\n";
            break;
        }

        current = nextEdge;
    }

    return loop;
}

void EditableMesh::SubmitVerticesForRendering() {
    for (EditableVertex* v : m_vertices) {
        DebugDraw::DrawPoint(v->position, OUTLINE_COLOR);
    }
}

void EditableMesh::SubmitEdgesForRendering() {
    for (Face* face : m_faces) {
        if (!face) continue;
        const auto& verts = face->vertices;
        for (size_t i = 0; i < verts.size(); ++i) {
            size_t next = (i + 1) % verts.size();
            DebugDraw::DrawLine(verts[i]->position, verts[next]->position, WHITE);
        }
    }
}

void EditableMesh::BuildInternalMeshData() {
    float createBackFaces = true;

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    vertices.reserve(m_faces.size() * 4);
    indices.reserve(m_faces.size() * 6 * (createBackFaces ? 2 : 1));

    for (Face* face : m_faces) {
        if (!face) continue;

        EditableVertex* v0 = face->vertices[0];
        EditableVertex* v1 = face->vertices[1];
        EditableVertex* v2 = face->vertices[2];
        EditableVertex* v3 = face->vertices[3];
        if (!v0 || !v1 || !v2 || !v3) continue;

        // Position
        glm::vec3 p0 = v0->position;
        glm::vec3 p1 = v1->position;
        glm::vec3 p2 = v2->position;
        glm::vec3 p3 = v3->position;

        // Normals
        glm::vec3 edge1 = p1 - p0;
        glm::vec3 edge2 = p2 - p0;
        glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

        // UVs
        glm::vec2 uv0(0.0f, 0.0f);
        glm::vec2 uv1(1.0f, 0.0f);
        glm::vec2 uv2(1.0f, 1.0f);
        glm::vec2 uv3(0.0f, 1.0f);

        // Tangent
        glm::vec2 deltaUV1 = uv1 - uv0;
        glm::vec2 deltaUV2 = uv2 - uv0;
        float det = deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x;
        float r = (std::abs(det) < 1e-6f) ? 1.0f : (1.0f / det);
        glm::vec3 tangent = glm::normalize((edge1 * deltaUV2.y - edge2 * deltaUV1.y) * r);

        // Add vertices
        unsigned baseIndex = static_cast<unsigned>(vertices.size());
        vertices.push_back({ p0, normal, uv0, tangent });
        vertices.push_back({ p1, normal, uv1, tangent });
        vertices.push_back({ p2, normal, uv2, tangent });
        vertices.push_back({ p3, normal, uv3, tangent });

        // Add indices
        indices.push_back(baseIndex + 0);
        indices.push_back(baseIndex + 1);
        indices.push_back(baseIndex + 2);
        indices.push_back(baseIndex + 0);
        indices.push_back(baseIndex + 2);
        indices.push_back(baseIndex + 3);

        // Back faces
        if (createBackFaces) {
            unsigned baseIndexBack = static_cast<unsigned>(vertices.size());
            glm::vec3 backNormal = -normal;
            glm::vec3 backTangent = -tangent;
            vertices.push_back({ p0, backNormal, uv0, backTangent });
            vertices.push_back({ p1, backNormal, uv1, backTangent });
            vertices.push_back({ p2, backNormal, uv2, backTangent });
            vertices.push_back({ p3, backNormal, uv3, backTangent });

            indices.push_back(baseIndexBack + 0);
            indices.push_back(baseIndexBack + 2);
            indices.push_back(baseIndexBack + 1);
            indices.push_back(baseIndexBack + 0);
            indices.push_back(baseIndexBack + 3);
            indices.push_back(baseIndexBack + 2);
        }
    }

    m_glMesh.UpdateBuffers(vertices, indices);
}

void EditableMesh::DeleteFace(int faceIndex) {
    if (faceIndex < 0 || faceIndex >= static_cast<int>(m_faces.size())) {
        std::cerr << "DeleteFace: Invalid face index!\n";
        return;
    }
    // Free the face
    Face* face = m_faces[faceIndex];
    m_faces.erase(m_faces.begin() + faceIndex);
    delete face;

    // Clean up vertices that are no longer referenced
    CleanUpEmptyData();
}

void EditableMesh::CleanUpEmptyData() {
    // First, build a set of vertices that are referenced by any face
    std::vector<EditableVertex*> usedVertices;
    for (Face* face : m_faces) {
        if (!face) continue;
        for (EditableVertex* v : face->vertices) {
            // Add v if it's not already in the used list
            if (std::find(usedVertices.begin(), usedVertices.end(), v) == usedVertices.end()) {
                usedVertices.push_back(v);
            }
        }
    }

    // Now, go through the global vertex list and delete any vertex that is not used
    std::vector<EditableVertex*> newVertices;
    for (EditableVertex* v : m_vertices) {
        if (std::find(usedVertices.begin(), usedVertices.end(), v) != usedVertices.end()) {
            newVertices.push_back(v);
        }
        else {
            delete v;
        }
    }
    m_vertices = std::move(newVertices);
}

bool EditableMesh::IsBoundaryEdge(Face* face, int edgeIndex) {
    if (!face) return false;

    // Get the two vertices that define this edge
    EditableVertex* vA = face->vertices[edgeIndex];
    EditableVertex* vB = face->vertices[(edgeIndex + 1) % 4];

    // Iterate over all faces in the mesh
    for (Face* otherFace : m_faces) {
        // Skip the same face
        if (otherFace == face) continue;

        // Check each edge in the other face
        for (int i = 0; i < 4; i++) {
            int next = (i + 1) % 4;
            EditableVertex* ovA = otherFace->vertices[i];
            EditableVertex* ovB = otherFace->vertices[next];

            // Check if the edge in otherFace matches (vA,vB) in either order
            if ((ovA == vA && ovB == vB) ||
                (ovA == vB && ovB == vA))
            {
                // Found an adjacent face that shares this edge
                return false;
            }
        }
    }

    // No other face uses this edge
    return true;
}


EdgeRef EditableMesh::ExtrudeEdge(EdgeRef* edge, float extrudeAmount) {
   // Validate input
    if (!edge || !edge->IsValid()) {
        std::cerr << "ExtrudeEdge: Invalid edge reference.\n";
        return EdgeRef();
    }

    // The face that owns the edge
    Face* face = edge->face;
    int idx = edge->edgeIndex;
    int nextIdx = (idx + 1) % 4;
    EditableVertex* v0 = face->vertices[idx];
    EditableVertex* v1 = face->vertices[nextIdx];
    if (!v0 || !v1) {
        std::cerr << "ExtrudeEdge: One or both edge vertices are null.\n";
        return EdgeRef();
    }
    glm::vec3 p0 = v0->position;
    glm::vec3 p1 = v1->position;

    // To compute the face normal, we use v0, v1, and a third vertex (at index (idx + 2) % 4)
    int idx2 = (idx + 2) % 4;
    EditableVertex* v2 = face->vertices[idx2];
    if (!v2) {
        std::cerr << "ExtrudeEdge: Third vertex is null.\n";
        return EdgeRef();
    }
    glm::vec3 p2 = v2->position;
    glm::vec3 faceNormal = glm::normalize(glm::cross(p1 - p0, p2 - p0));
    if (glm::length(faceNormal) < 1e-6f) {
        std::cerr << "ExtrudeEdge: Could not compute a valid face normal.\n";
        return EdgeRef();
    }

    // Compute the edge direction
    glm::vec3 edgeDir = p1 - p0;
    float eLen = glm::length(edgeDir);
    if (eLen < 1e-6f) {
        std::cerr << "ExtrudeEdge: Edge length is near zero.\n";
        return EdgeRef();
    }
    edgeDir /= eLen;

    // Compute a left vector from the face normal and the edge direction
    glm::vec3 leftVec = glm::cross(faceNormal, edgeDir);
    if (glm::length(leftVec) < 1e-6f) {
        std::cerr << "ExtrudeEdge: Could not compute a valid left vector.\n";
        return EdgeRef();
    }
    leftVec = glm::normalize(leftVec);

    // Compute the centroid of the face (average of the 4 vertices)
    glm::vec3 centroid(0.0f);
    for (int i = 0; i < 4; i++) {
        centroid += face->vertices[i]->position;
    }
    centroid /= 4.0f;

    // Compute the edge midpoint
    glm::vec3 midpoint = (p0 + p1) * 0.5f;

    // Determine the offset direction so that extrusion goes away from the face
    glm::vec3 offsetDir;
    if (glm::dot(leftVec, centroid - midpoint) > 0)
        offsetDir = -leftVec * extrudeAmount;
    else
        offsetDir = leftVec * extrudeAmount;

    // Create two new vertices along the extruded edge
    EditableVertex* newV0 = new EditableVertex();
    EditableVertex* newV1 = new EditableVertex();
    newV0->position = p0 + offsetDir;
    newV1->position = p1 + offsetDir;
    m_vertices.push_back(newV0);
    m_vertices.push_back(newV1);

    // Create a new face (quad) connecting the old edge to the new extruded edge
    // Initial guess at ordering is { v0, v1, newV1, newV0 }
    Face* newFace = new Face();
    newFace->vertices = { v0, v1, newV1, newV0 };
    m_faces.push_back(newFace);

    // Ensure the new face's orientation matches the original face
    // Compute the new face's normal from the first triangle (v0, v1, newV1)
    glm::vec3 newp0 = newFace->vertices[0]->position;
    glm::vec3 newp1 = newFace->vertices[1]->position;
    glm::vec3 newp2 = newFace->vertices[2]->position;

    glm::vec3 newFaceNormal = glm::cross(newp1 - newp0, newp2 - newp0);
    float lenNF = glm::length(newFaceNormal);
    if (lenNF > 1e-6f) {
        newFaceNormal = glm::normalize(newFaceNormal);
        // If dot < 0, flip the winding
        if (glm::dot(newFaceNormal, faceNormal) < 0.0f) {
            // Reverse the 4 vertices to flip the face
            std::reverse(newFace->vertices.begin(), newFace->vertices.end());
        }
    }

    EdgeRef ret;
    for (int i = 0; i < 4; i++) {
        int ni = (i + 1) % 4;
        if (newFace->vertices[i] == newV1 && newFace->vertices[ni] == newV0) {
            ret.face = newFace;
            ret.edgeIndex = i;
            break;
        }
    }

    return ret;
}

#include <set>
#include <map>
#include <utility>
#include <vector>
#include <algorithm>
#include <iostream>

void EditableMesh::InsertEdgeLoop(std::vector<EdgeRef>& loopEdges, std::vector<glm::vec3>& loopPoints) {
    // Gather all unique faces
    std::vector<Face*> affectedFaces;
    for (const EdgeRef& edge : loopEdges) {
        bool found = false;
        for (Face* face : affectedFaces) {
            if (face == edge.face) {
                found = true;
                break;
            }
        }
        if (!found) {
            affectedFaces.push_back(edge.face);
        }
    }

    // Gather unique edges
   //std::vector<EdgeRef> uniqueEdges;
   //for (const EdgeRef& edge : loopEdges) {
   //
   //    glm::vec3 p1 = edge.face->vertices[edge.edgeIndex]->position;
   //    glm::vec3 p2 = edge.face->vertices[(edge.edgeIndex + 1) % 4]->position;
   //
   //    bool found = false;
   //    for (EdgeRef& uniqueEdge : uniqueEdges) {
   //
   //        glm::vec3 uniqueP1 = uniqueEdge.face->vertices[uniqueEdge.edgeIndex]->position;
   //        glm::vec3 uniqueP2 = uniqueEdge.face->vertices[(uniqueEdge.edgeIndex + 1) % 4]->position;
   //
   //        if (p1 == uniqueP1 && p2 == uniqueP2 ||
   //            p1 == uniqueP2 && p2 == uniqueP1) {
   //            found = true;
   //            break;
   //        }
   //    }
   //    if (!found) {
   //        uniqueEdges.push_back(edge);
   //    }
   //}

    // Split each face
    int k = -1;
    for (Face* face : affectedFaces) {
        k++;

        EdgeRef* firstEdge = nullptr;
        EdgeRef* secondEdge = nullptr;
        int vertexIdx1 = -1;
        int vertexIdx2 = -1;
        int vertexIdx3 = -1;
        int vertexIdx4 = -1;

        // Find first matching loop edge
        for (EdgeRef& edge : loopEdges) {
            vertexIdx1 = -1, 
            vertexIdx2 = -1;
            glm::vec3 p1 = edge.face->vertices[edge.edgeIndex]->position;
            glm::vec3 p2 = edge.face->vertices[(edge.edgeIndex + 1) % 4]->position;

            for (int i = 0; i < 4; i++) {
                if (face->vertices[i]->position == p1) {
                    vertexIdx1 = i;
                }
                else if (face->vertices[i]->position == p2) {
                    vertexIdx2 = i;
                }
            }
            if (vertexIdx1 != -1 && vertexIdx2 != -1) {
                firstEdge = &edge;
                break;
            }
        }
        if (!firstEdge) {
            std::cout << "Failed to find first edge for face " << face << "\n";
            continue;
        }

        // Find second matching loop edge
        for (EdgeRef& edge : loopEdges) {
            vertexIdx3 = -1;
            vertexIdx4 = -1;

            // Skip if this edge matches the first
            glm::vec3 firstEdgeP1 = firstEdge->face->vertices[firstEdge->edgeIndex]->position;
            glm::vec3 firstEdgeP2 = firstEdge->face->vertices[(firstEdge->edgeIndex + 1) % 4]->position;
            glm::vec3 p1 = edge.face->vertices[edge.edgeIndex]->position;
            glm::vec3 p2 = edge.face->vertices[(edge.edgeIndex + 1) % 4]->position;
            if ((firstEdgeP1 == p1 && firstEdgeP2 == p2) || (firstEdgeP1 == p2 && firstEdgeP2 == p1)) {
                continue;
            }

            for (int i = 0; i < 4; i++) {
                // Skip the vertices we've already found
                if (i == vertexIdx1 || i == vertexIdx2) {
                    continue;
                }
                else if (face->vertices[i]->position == p1) {
                    vertexIdx3 = i;
                }
                else if (face->vertices[i]->position == p2) {
                    vertexIdx4 = i;
                }
            }
            if (vertexIdx3 != -1 && vertexIdx4 != -1) {
                secondEdge = &edge;
                break;
            }
        }
        if (!secondEdge) {
            std::cout << "Failed to find second edge for face " << face << "\n";
        }
                

        if (firstEdge && secondEdge) {
            std::cout << "successfully found edges " << firstEdge << " & " << secondEdge << " for face " << face << "\n";

            // Add the vertices
            EditableVertex* v1 = new EditableVertex();
            EditableVertex* v2 = new EditableVertex();
            m_vertices.push_back(v1);
            m_vertices.push_back(v2);

            // Set their positions
            for (int i = 0; i < loopEdges.size(); i++) {
                EdgeRef* edge = &loopEdges[i];
                if (firstEdge == edge) {
                    v1->position = loopPoints[i];
                    //std::cout << "inserted v1\n";
                }
                if (secondEdge == edge) {
                    v2->position = loopPoints[i];
                    //std::cout << "inserted v2\n";
                }
            }


            // Create new face, first by duplicating the original face
            Face* newFace1 = new Face();
            Face* newFace2 = new Face();
            m_faces.push_back(newFace1);
            m_faces.push_back(newFace2);
            for (int j = 0; j < 4; j++) {
                newFace1->vertices[j] = face->vertices[j];
                newFace2->vertices[j] = face->vertices[j];
            }

            glm::vec3 firstEdgeP1 = face->vertices[firstEdge->edgeIndex]->position;
            glm::vec3 firstEdgeP2 = face->vertices[(firstEdge->edgeIndex + 1) % 4]->position;
            glm::vec3 secondEdgeP1 = face->vertices[secondEdge->edgeIndex]->position;
            glm::vec3 secondEdgeP2 = face->vertices[(secondEdge->edgeIndex + 1) % 4]->position;
            //glm::vec3 p2 = face->vertices[(firstEdge->edgeIndex + 1) % 4]->position;

            // Update current face by moving 2 of its vertices

            bool foundCase = false;
            for (int i = 0; i < 4; i++) {

                int thisVertex = (i + 0) % 4;
                int nextVertex = (i + 1) % 4;
                int nextNextVertex = (i + 2) % 4;
                int nextNextNextVertex = (i + 3) % 4;

                // Case 1
                if (face->vertices[thisVertex]->position == firstEdgeP1 &&
                    face->vertices[nextVertex]->position == firstEdgeP2) {

                    std::cout << "case 1\n";
                    foundCase = true;
                    newFace1->vertices[nextVertex] = v1;
                    newFace1->vertices[nextNextVertex] = v2;
                    newFace2->vertices[thisVertex] = v1;
                    newFace2->vertices[nextNextNextVertex] = v2;
                    break;
                }
            }

            if (!foundCase) {

                std::cout << "Case not found!!!\n";

                for (int i = 0; i < 4; i++) {
                    std::cout << "v" << i << ": " << Util::Vec3ToString(face->vertices[i]->position) << "\n";
                }

                std::cout << "firstEdgeP1: " << Util::Vec3ToString(firstEdgeP1) << "\n";
                std::cout << "firstEdgeP2: " << Util::Vec3ToString(firstEdgeP2) << "\n";
                std::cout << "secondEdgeP1: " << Util::Vec3ToString(secondEdgeP1) << "\n";
                std::cout << "secondEdgeP2: " << Util::Vec3ToString(secondEdgeP2) << "\n";

                std::cout << "\n\n";
            }

            // Update new face by moving 2 of its vertices
            //for (int i = 0; i < 4; i++) {
            //    int nextVertex = (i + 1) % 4;
            //    int nextOppositeVertex = (i + 2) % 4;
            //    if (newFace->vertices[i]->position == secondEdgeP1) {
            //        newFace->vertices[nextVertex] = v2;
            //        newFace->vertices[nextOppositeVertex] = v1;
            //    }
            //}



            if (k == 3) {


                std::cout << "\n";
                std::cout << "\n";

                for (int i = 0; i < 4; i++) {
                    std::cout << "v" << i << ": " << Util::Vec3ToString(face->vertices[i]->position) << "\n";
                }

                std::cout << "\n";

                std::cout << "firstEdgeP1: " << Util::Vec3ToString(firstEdgeP1) << "\n";
                std::cout << "firstEdgeP2: " << Util::Vec3ToString(firstEdgeP2) << "\n";
                std::cout << "secondEdgeP1: " << Util::Vec3ToString(secondEdgeP1) << "\n";
                std::cout << "secondEdgeP2: " << Util::Vec3ToString(secondEdgeP2) << "\n";
                
                std::cout << "\n";

                for (int i = 0; i < loopEdges.size(); i++) {

                    EdgeRef& edge = loopEdges[i];
                    glm::vec3 p1 = edge.face->vertices[edge.edgeIndex]->position;
                    glm::vec3 p2 = edge.face->vertices[(edge.edgeIndex + 1) % 4]->position;

                    std::cout << i << ":   p1 " << Util::Vec3ToString(p1) << "   ";
                    std::cout << "p2 " << Util::Vec3ToString(p2) << "\n";
                }

                continue;
            }
        }


    }

    // Remove old faces
    for (int i = 0; i < m_faces.size(); i++) {
        for (Face* face : affectedFaces) {
            if (m_faces[i] == face) {
                m_faces.erase(m_faces.begin() + i);
                delete face;
                i--;
                break;
            }
        }
    }

    

    std::cout << "unique faces.size(): " << affectedFaces.size() << "\n";
}

void EditableMesh::CleanUp() {
    for (auto v : m_vertices) delete v;
    for (auto f : m_faces) delete f;
    m_vertices.clear();
    m_faces.clear();
}
*/