/*#pragma once
#include <array>
#include <vector>
#include <Game/Types.h>
#include "API/OpenGL/Types/GL_mesh_buffer.h"

//struct Edge;

struct EditableVertex {
    glm::vec3 position;
    //std::vector<Edge*> incidentEdges; // For modeling
};

struct Face {
    std::array<EditableVertex*, 4> vertices;
};

struct EdgeRef {
    Face* face = nullptr;
    int edgeIndex = -1;
    bool IsValid() const { return face && edgeIndex >= 0 && edgeIndex < 4; }
    bool operator==(const EdgeRef& other) const { return face == other.face && edgeIndex == other.edgeIndex; }
};

class EditableMesh {
public:
    std::vector<EditableVertex*> m_vertices;
    std::vector<Face*>           m_faces;

    OpenGLMeshBuffer m_glMesh;

    void InitAsCube(const glm::vec3& spawnOffset = glm::vec3(0.0f));
    void CleanUp();
    void SubmitVerticesForRendering();
    void SubmitEdgesForRendering();
    void BuildInternalMeshData();
    void DeleteFace(int faceIndex);

    bool IsBoundaryEdge(Face* face, int edgeIndex);
    EdgeRef ExtrudeEdge(EdgeRef* edge, float extrudeAmount);
    std::vector<EdgeRef> GetLoopEdges(EdgeRef* startingEdge);
    void InsertEdgeLoop(std::vector<EdgeRef>& loopEdges, std::vector<glm::vec3>& loopPoints);

private:
    void CleanUpEmptyData();
    void BuildOpenGLMesh(bool createBackFaces);
};
*/