#include "../GL_renderer.h"
#include "Hell/ResourceManagement/ResourceManager.h"

namespace OpenGLRenderer {

    void UploadVertexWeights() {
        Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("AssetGeometry");
        std::vector<VertexWeight>& vertexWeights = meshBuffer.GetVertexWeights();
        UploadSSBOStatic("VertexWeights", sizeof(VertexWeight) * vertexWeights.size(), vertexWeights.data());
    }
}
