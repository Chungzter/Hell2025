#pragma once
#include <Game/Types.h>
#include "Hell/ResourceManagement/Types/Texture.h"

namespace BakeQueue {
    // Textures
    void QueueTextureForBaking(Texture* texture);
    void RemoveQueuedTextureBakeByJobID(int jobID);
    void ImmediateBakeAllTextures();
    const int GetQueuedTextureBakeJobCount();
    QueuedTextureBake* GetNextQueuedTextureBake();
    QueuedTextureBake* GetQueuedTextureBakeByJobID(int jobID);
}
