#pragma once
#include "Hell/Math/AABB.h"
#include "Hell/Math/VecXZ.h"
#include "Hell/Containers/SlotMap.h"

#include <cstdint>
#include <vector>

namespace Unloved {

    struct TerrainData {
        int textureWidth;
        int textureHeight;
        int chunkCountX;
        int chunkCountZ;
        std::vector<float> samples;
    };

    struct TerrainChunk {
        Hell::ivecXZ coord;
        uint32_t meshId;
        uint32_t baseVertex;
        uint32_t baseIndex;
        AABB bounds;
        uint64_t heightFieldId; // ? what is this
        bool meshDirty;
        bool physicsDirty;
    };

    struct Terrain {
        TerrainData* sourceData;
        Hell::SlotMap<TerrainChunk> chunks;

        void LoadFromData(/**/);
        void Paint(/**/);
        void RebuildRenderGeometry(/**/);
        void RebuildPhysics(/**/);
    };
}
