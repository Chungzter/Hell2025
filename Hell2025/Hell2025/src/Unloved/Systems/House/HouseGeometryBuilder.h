#pragma once

#include "Hell/Render/VertexAttributes.h"
#include "Unloved/Common/PlanarQuad.h"

#include <cstdint>
#include <vector>

struct HouseGeometrySourceMesh {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    glm::vec3 aabbMin = glm::vec3(0.0f);
    glm::vec3 aabbMax = glm::vec3(0.0f);
};

namespace Unloved::HouseGeometryBuilder {

    void Init();

    void CreateDownFacingPlane(const PlanarQuad& planarQuad, std::vector<Vertex>& verticesOut, std::vector<uint32_t>& indicesOut);
    void CreateUpFacingPlane(const PlanarQuad& planarQuad, std::vector<Vertex>& verticesOut, std::vector<uint32_t>& indicesOut);

    const HouseGeometrySourceMesh& GetDeckingBoardsSourceMesh();
    const HouseGeometrySourceMesh& GetGutterSourceMesh();
    const HouseGeometrySourceMesh& GetRidgeCappingSourceMesh();
}
