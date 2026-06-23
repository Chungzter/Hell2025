#pragma once

#include "Hell/BVH/Types.h"
#include "Hell/Render/VertexAttributes.h"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <cstdint>
#include <vector>

#define MAX_BVH_STACK_SIZE 32

namespace Bvh::Gpu {
    uint64_t CreateMeshBvhFromVertexData(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
    uint64_t CreateMeshBvhFromMeshBvh(MeshBvh& sourceMeshBvh);
    uint64_t CreateNewSceneBvh();

    void UpdateSceneBvh(uint64_t bvhId, std::vector<PrimitiveInstance>& instances);
    void DestroySceneBvh(uint64_t bvhId);
    bool SceneBvhExists(uint64_t bvhId); 
    bool MeshBvhExists(uint64_t bvhId);
    void DestroyMeshBvh(uint64_t bvhId);
    bool MeshBvhExists(uint64_t bvhId);
    void FlatternMeshBvhNodes();

    void RenderMesh(uint64_t bvhId, glm::vec4 color, glm::mat4 worldTransform);
    void RenderMeshBvh(uint64_t bvhId, glm::vec4 color, glm::mat4 worldTransform);
    void RenderRayResultTriangle(BvhRayResult& rayResult, glm::vec4 color);
    void RenderRayResultNode(BvhRayResult& rayResult, glm::vec4 color);
    void RenderSceneBvh(uint64_t bvhId, glm::vec4 color);

    bool IntersectNode(const RayData& rayData, const glm::vec3& aabbBoundsMin, const glm::vec3& aabbBoundsMax, float& t);
    bool IntersectNodeGpu(const RayData& rayData, const glm::vec3& aabbBoundsMin, const glm::vec3& aabbBoundsMax, float& t);
    bool IntersectTri(glm::vec3 rayOrigin, glm::vec3 rayDir, float minDistance, float maxDistance, glm::vec3 p0, glm::vec3 e1, glm::vec3 e2, glm::vec3 normal, float& t);

    RayData ComputeRayData(const glm::vec3& rayOrigin, const glm::vec3& rayDir, float minDistance, float maxDistance);

    BvhRayResult AnyHit(uint64_t sceneBvhId, glm::vec3 rayOrigin, glm::vec3 rayDir, float maxDistance);
    BvhRayResult ClosestHit(uint64_t sceneBvhId, glm::vec3 rayOrigin, glm::vec3 rayDir, float maxDistance);

    SceneBvh* GetSceneBvhById(uint64_t bvhId);
    MeshBvh* GetMeshBvhById(uint64_t bvhId);

    const std::vector<BvhNode>& GetMeshGpuBvhNodes();
    const std::vector<GpuPrimitiveInstance>& GetGpuEntityInstances(uint64_t sceneBvhId);
    const std::vector<BVHTriangle>& GetTriangles();

    glm::vec3 GetMeshBvhRootNodeBoundsMin(uint64_t bvhId);
    glm::vec3 GetMeshBvhRootNodeBoundsMax(uint64_t bvhId);

    void RenderMeshTest(int index, glm::vec4 color, glm::mat4 worldTransform);
    void RenderMeshBvhTest(int index, glm::vec4 color, glm::mat4 worldTransform);
}
