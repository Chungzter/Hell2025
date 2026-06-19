#pragma once
#include <Game/Types.h>
#include <Game/CreateInfo.h>
/*#include <vector>
#include <glad/gl.h>

#include "GlobalIllumination/DDGIVolume.h"

struct PointCloudOctrant {
    unsigned int m_cloudPointCount;
    unsigned int m_offset;
};

namespace GlobalIllumination {
    void Update();
    void SetGlobalIlluminationStructuresDirtyState(bool state);
    void SetPointCloudNeedsGpuUpdateState(bool state);

    bool GlobalIlluminationStructuresAreDirty();
    bool PointCloudNeedsGpuUpdate();

    float GetProbeSpacing();
    float GetPointCloudSpacing();

    uint64_t GetSceneBvhId();
    const std::vector<BvhNode>& GetSceneNodes();
    std::vector<CloudPoint>& GetPointClound();
	std::vector<CloudPointTextureInfo>& GetPointCloudTextureInfo();
    //std::vector<LightVolume>& GetLightVolumes();
    DDGIVolume& GetTestLightVolume();
    std::vector<PointCloudOctrant>& GetPointCloudOctrants();
    std::vector<unsigned int>& GetPointIndices();
    glm::uvec3 GetPointCloudGridDimensions();
    glm::vec3 GetPointGridWorldMin();
    glm::vec3 GetPointGridWorldMax();

    std::vector<DDGIVolumeCreateInfo> GetDDGIVolumesCreateInfo();
}*/