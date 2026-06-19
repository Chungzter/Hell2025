#pragma once
#include <glm/glm.hpp>
#include <string>

struct Mesh {
    int32_t baseVertex = 0;
    uint32_t baseIndex = 0;
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
    uint64_t meshBvhId = 0;
    int32_t parentIndex = -1;
    glm::vec3 aabbMin = glm::vec3(0);
    glm::vec3 aabbMax = glm::vec3(0);
    glm::vec3 extents = glm::vec3(0);
    std::string name = "undefined";
    float boundingSphereRadius = 0;
    glm::mat4 localTransform = glm::mat4(1.0f);
    glm::mat4 inverseBindTransform = glm::mat4(1.0f);

    void SetName(const std::string& name);
    int32_t GetIndexCount();
    const std::string& GetName();
};
