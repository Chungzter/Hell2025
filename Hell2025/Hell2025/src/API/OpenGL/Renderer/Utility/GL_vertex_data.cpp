#include "../GL_renderer.h"
#include "AssetManagement/AssetManager.h"

namespace OpenGLRenderer {

    void UploadVertexWeights() {
        std::vector<VertexWeight>& vertexWeights = AssetManager::GetVertexWeights();
        UploadSSBOStatic("VertexWeights", sizeof(VertexWeight) * vertexWeights.size(), vertexWeights.data());
    }
}