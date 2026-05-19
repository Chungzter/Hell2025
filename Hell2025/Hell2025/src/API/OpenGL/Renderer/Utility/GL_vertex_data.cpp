#include "../GL_renderer.h"
#include "AssetManagement/AssetManager.h"

namespace OpenGLRenderer {

    void UploadVertexWeights() {
        std::vector<VertexWeight>& vertexWeights = AssetManager::GetVertexWeights();
        UploadSSBOStatic("VertexWeights", sizeof(VertexWeight) * vertexWeights.size(), vertexWeights.data());
    }

    void UploadWeightedVertexData() {
        std::vector<Vertex>& vertices = AssetManager::GetWeightedVertices();
        std::vector<uint32_t>& indices = AssetManager::GetWeightedIndies();

        UploadSSBOStatic("Vertices2", sizeof(Vertex) * vertices.size(), vertices.data());
        UploadSSBOStatic("Indices2", sizeof(uint32_t) * indices.size(), indices.data());
    }

}