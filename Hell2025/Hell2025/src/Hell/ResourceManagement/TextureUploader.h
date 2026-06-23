#pragma once

#include "Hell/ResourceManagement/Types/Texture.h"

namespace Hell::TextureUploader {
    bool ImmediateUpload(Texture& texture);
    void QueueUpload(Texture& texture);
    void Update();
    void CleanUp();
}
