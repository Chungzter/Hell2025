#pragma once
#include <Game/CreateInfo.h>
#include <Game/Types.h>

#include "PointCloud.h"
#include "Types/Renderer/MeshBufferOLD.h"

struct DDGIVolume {
    DDGIVolume() = default;
    DDGIVolume(uint64_t id, DDGIVolumeCreateInfo& createInfo, SpawnOffset& spawnOffset);
    DDGIVolume(const DDGIVolume&) = delete;
    DDGIVolume& operator=(const DDGIVolume&) = delete;
    DDGIVolume(DDGIVolume&&) noexcept = default;
    DDGIVolume& operator=(DDGIVolume&&) noexcept = default;
    ~DDGIVolume() = default;

    void Init(const glm::vec3& aabbMin, const glm::vec3& aabbMax, float probeSpacing);
    void Update();
    void CleanUp();
    
    void SetEditorName(const std::string& name);
    void SetOrigin(const glm::vec3& origin);
    void SetRotation(const glm::vec3& rotation);
    void SetExtents(const glm::vec3& extents);
    void SetProbeSpacing(float spacing);
    void DebugDraw();
    void CreateRaytracingData();
    void UpdateSceneBvh();

    void MarkPointCloudAsUploaded() { m_pointCloudNeedsGpuUpload = false; }

    uint32_t GetTotalProbeCount() const;
    DDGIVolumeGPU GetGPUData() const;
    const std::vector<BvhNode>& GetSceneNodes();

    uint32_t m_pointcloudVAO = 0;
    uint32_t m_pointcloudVBO = 0;

    uint64_t& GetId()                                                          { return m_id; }
    DDGIVolumeCreateInfo& GetCreateInfo()                                      { return m_createInfo; }
    const PointCloud& GetPointClound() const                                   { return m_pointCloud; }
    const std::vector<CloudPoint>& GetPointCloundPoints() const                { return m_pointCloud.GetPoints(); }
    const std::vector<CloudPointTextureInfo>& GetPointCloudTextureInfo() const { return m_pointCloud.GetTextureInfo(); }
    const std::string& GetEditorName() const                                   { return m_createInfo.editorName; }
    const glm::vec3& GetOrigin() const                                         { return m_createInfo.origin; }
    const glm::vec3& GetRotation() const                                       { return m_createInfo.rotation; }
    const glm::vec3& GetExtents() const                                        { return m_createInfo.extents; }
    const glm::vec3 GetBoundsMin() const                                       { return m_boundsMin; }
    const glm::vec3 GetBoundsMax() const                                       { return m_boundsMax; }
    float GetWorldSpaceWidth() const                                           { return m_worldSpaceWidth; }
    float GetWorldSpaceHeight() const                                          { return m_worldSpaceHeight; }
    float GetWorldSpaceDepth() const                                           { return m_worldSpaceDepth; }
    float GetProbeSpacing() const                                              { return m_createInfo.probeSpacing; }
    float GetPointCloudSpacing() const                                         { return m_createInfo.pointCloudSpacing; }
    int GetProbeCountX() const                                                 { return m_probeCountX; }
    int GetProbeCountY() const                                                 { return m_probeCountY; }
    int GetProbeCountZ() const                                                 { return m_probeCountZ; }
    bool PointCloudNeedsGPUUpload() const                                      { return m_pointCloudNeedsGpuUpload; }
    uint64_t GetSceneBvhId() const                                             { return m_sceneBvhId; }
    uint32_t GetPointCloudCount() const                                        { return m_pointCloud.GetPointCount(); }
    uint32_t GetProbePointIndexPoolSize() const                                { return m_probePointIndexPoolSize; }

    MeshBufferOLD m_staticMeshBuffer;

private:
    void UpdateMembers();
    void CleanUpRaytracingData();
    void CreateTriangleData();
    void CreateHouseBvh();
    void CreateDoorBvh();
    void CreatePointCloud();
    void CalculateProbePointIndexPoolSize();
    glm::vec3 GetProbeBaseWorldPosition(const glm::ivec3& probeCoords) const; // Used only for rendering probe base world position debug points

    uint64_t m_id = 0;
    DDGIVolumeCreateInfo m_createInfo;

    glm::vec3 m_boundsMin = glm::vec3(0.0f);
    glm::vec3 m_boundsMax = glm::vec3(0.0f);
    float m_worldSpaceWidth = 0.0f;
    float m_worldSpaceHeight = 0.0f;
    float m_worldSpaceDepth = 0.0f;
    int m_probeCountX = 0;
    int m_probeCountY = 0;
    int m_probeCountZ = 0;
    bool m_pointCloudNeedsGpuUpload = false;
    bool m_raytracingDataDirty = false;

    std::vector<Triangle> m_triangles;
    PointCloud m_pointCloud;

    uint64_t m_houseBvhId = 0;
    uint64_t m_doorBvhId = 0;
    uint64_t m_sceneBvhId = 0;

    uint32_t m_probePointIndexPoolSize;
};