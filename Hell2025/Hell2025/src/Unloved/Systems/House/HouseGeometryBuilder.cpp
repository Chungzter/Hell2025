#include "HouseGeometryBuilder.h"

#include "Hell/ResourceManagement/ResourceManager.h"

namespace Unloved::HouseGeometryBuilder {

    namespace {
        HouseGeometrySourceMesh g_deckingBoardsSourceMesh;
        HouseGeometrySourceMesh g_gutterSourceMesh;
        HouseGeometrySourceMesh g_ridgeCappingSourceMesh;

        HouseGeometrySourceMesh LoadFromModel(const std::string& modelName) {
            HouseGeometrySourceMesh sourceMesh;

            Model* model = Hell::ResourceManager::GetModelByName(modelName);
            if (!model || model->GetMeshIndices().empty()) return sourceMesh;

            Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("AssetGeometry");
            Mesh* mesh = meshBuffer.GetMeshById(model->GetMeshIndices()[0]);
            if (!mesh) return sourceMesh;

            // Rip em out
            sourceMesh.vertices.assign(meshBuffer.GetVertices().begin() + mesh->baseVertex, meshBuffer.GetVertices().begin() + mesh->baseVertex + mesh->vertexCount);
            sourceMesh.indices.assign(meshBuffer.GetIndices().begin() + mesh->baseIndex, meshBuffer.GetIndices().begin() + mesh->baseIndex + mesh->indexCount);
            sourceMesh.aabbMin = mesh->aabbMin;
            sourceMesh.aabbMax = mesh->aabbMax;

            return sourceMesh;
        }
    }

    void Init() {
        g_deckingBoardsSourceMesh = LoadFromModel("DeckingBoards");
        g_gutterSourceMesh = LoadFromModel("Gutter");
        g_ridgeCappingSourceMesh = LoadFromModel("RidgeCapping");
    }

    void CreateDownFacingPlane(const PlanarQuad& planarQuad, std::vector<Vertex>& verticesOut, std::vector<uint32_t>& indicesOut) {
        if (!planarQuad.IsValid()) return;

        const uint32_t baseVertex = static_cast<uint32_t>(verticesOut.size());
        const std::array<glm::vec3, 4> positions = { planarQuad.GetPositionP0(), planarQuad.GetPositionP1(), planarQuad.GetPositionP2(), planarQuad.GetPositionP3() };
        const std::array<glm::vec2, 4> uvs = { glm::vec2(0.0f, 0.0f), glm::vec2(0.0f, planarQuad.GetDepth()), glm::vec2(planarQuad.GetWidth(), planarQuad.GetDepth()), glm::vec2(planarQuad.GetWidth(), 0.0f) };

        for (uint32_t i = 0; i < 4; i++) {
            Vertex& vertex = verticesOut.emplace_back();
            vertex.position = positions[i];
            vertex.normal = -planarQuad.GetNormal();
            vertex.uv = uvs[i];
            vertex.tangent = planarQuad.GetRight();
        }

        indicesOut.insert(indicesOut.end(), { baseVertex + 0, baseVertex + 3, baseVertex + 2, baseVertex + 2, baseVertex + 1, baseVertex + 0 });
    }

    void CreateUpFacingPlane(const PlanarQuad& planarQuad, std::vector<Vertex>& verticesOut, std::vector<uint32_t>& indicesOut) {
        if (!planarQuad.IsValid()) return;

        const uint32_t baseVertex = static_cast<uint32_t>(verticesOut.size());
        const std::array<glm::vec3, 4> positions = { planarQuad.GetPositionP0(), planarQuad.GetPositionP1(), planarQuad.GetPositionP2(), planarQuad.GetPositionP3() };
        const std::array<glm::vec2, 4> uvs = { glm::vec2(0.0f, 0.0f), glm::vec2(0.0f, planarQuad.GetDepth()), glm::vec2(planarQuad.GetWidth(), planarQuad.GetDepth()), glm::vec2(planarQuad.GetWidth(), 0.0f) };

        for (uint32_t i = 0; i < 4; i++) {
            Vertex& vertex = verticesOut.emplace_back();
            vertex.position = positions[i];
            vertex.normal = planarQuad.GetNormal();
            vertex.uv = uvs[i];
            vertex.tangent = planarQuad.GetRight();
        }

        indicesOut.insert(indicesOut.end(), { baseVertex + 0, baseVertex + 1, baseVertex + 2, baseVertex + 2, baseVertex + 3, baseVertex + 0 });
    }

    const HouseGeometrySourceMesh& GetDeckingBoardsSourceMesh() { return g_deckingBoardsSourceMesh; }
    const HouseGeometrySourceMesh& GetGutterSourceMesh()        { return g_gutterSourceMesh; }
    const HouseGeometrySourceMesh& GetRidgeCappingSourceMesh()  { return g_ridgeCappingSourceMesh; }
}
