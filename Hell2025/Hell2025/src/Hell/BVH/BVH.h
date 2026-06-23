#pragma once
#include "Hell/BVH/Types.h"
#include "Hell/Render/VertexAttributes.h"

#include <cstdint>
#include <vector>

// TODO: merge all cpu/gpu bvh stuff
// For cpu bvh instance meta data, write it with templates so different games using hell engine can supply their own custom meta data struct
// But also rethink this, that might be insanely overkill. If possible to just use an uint64_t objectId without bundling in the openableId then that would be ideal to avoid templates

namespace Hell::Bvh {
    MeshBvh BuildMeshBvh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
}
