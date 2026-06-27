#include "../GL_renderer.h"

#include "Hell/ResourceManagement/ResourceManager.h"

namespace OpenGLRenderer {

    void ClearAllWoundMasks() {
        Hell::TextureArray* woundMaskArray = Hell::ResourceManager::GetTextureArrayPtr("WoundMasks");
        if (!woundMaskArray) return;

        for (int i = 0; i < WOUND_MASK_TEXTURE_ARRAY_SIZE; i++) {
            woundMaskArray->ClearLayer(0, 0, 0, 0, i);
        }
    }
}
