#pragma once

#include <glm/vec3.hpp>

#include <string>
#include <cstdint>

struct SkinnedMesh {
    std::string name = "undefined";
    int32_t baseVertexLocal = 0;
    int32_t baseVertexGlobal = 0;
    int32_t baseVertexWeight = 0;
    uint32_t baseIndex = 0;
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
    glm::vec3 aabbMin = glm::vec3(0);
    glm::vec3 aabbMax = glm::vec3(0);
    bool requiresSkinning = false;
    int32_t nonDeformingBoneIndex = 0;
};

