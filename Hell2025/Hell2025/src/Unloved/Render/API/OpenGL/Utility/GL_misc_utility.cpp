#include "../GL_renderer.h"

namespace OpenGLRenderer {

    void ClearAllWoundMasks() {
        OpenGLTextureArray* woundMaskArray = OpenGL::ResourceManager::GetTextureArrayPtr("WoundMasks");
        if (!woundMaskArray) return;

        for (int i = 0; i < WOUND_MASK_TEXTURE_ARRAY_SIZE; i++) {
            woundMaskArray->ClearLayer(0, 0, 0, 0, i);
        }
    }
}