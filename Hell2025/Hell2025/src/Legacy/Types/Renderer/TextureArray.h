#pragma once
#include <Game/Enums.h>
#include "Hell/Render/API/OpenGL/Types/gl_texture_array.h"

struct TextureArray {
    OpenGLTextureArray& GetGLTextureArray() { return m_glTextureArray; };


private:
    OpenGLTextureArray m_glTextureArray;
};