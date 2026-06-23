#pragma once

#include "Hell/ResourceManagement/Types/Texture.h"

namespace OpenGLTextureUploader {
    bool ImmediateUpload(Texture& texture);
    void QueueUpload(Texture& texture);
}