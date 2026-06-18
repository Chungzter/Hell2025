#pragma once
#include <Hell/Types.h>
#include "Types/Renderer/MeshBufferOLD.h"

struct Wire {
    void Init(glm::vec3 begin, glm::vec3 end, float sag, float radius, float spacing);
    void Update();
    void CleanUp();

    MeshBufferOLD& GetMeshBuffer()                             { return m_meshBuffer; }
    const std::vector<glm::vec3>& GetSegmentPoints() const  { return m_segmentPoints; }

private:
    float m_sag = 1.0f;
    float m_radius = 0.1f;
    glm::vec3 m_begin = glm::vec3(0.0f);
    glm::vec3 m_end = glm::vec3(0.0f);
    std::vector<glm::vec3> m_segmentPoints;
    MeshBufferOLD m_meshBuffer;
};