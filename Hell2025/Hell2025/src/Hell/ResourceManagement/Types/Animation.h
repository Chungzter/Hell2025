#pragma once

#include "Hell/Common.h"
#include "Hell/File.h"

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

#include <cstddef>
#include <string>
#include <vector>

struct SQT {
    glm::quat rotation = glm::quat(1, 0, 0, 0);
    glm::vec3 positon = glm::vec3(0, 0, 0);
    glm::vec3 scale = glm::vec3(1.0f);
    float timeStamp = -1;
};

struct AnimatedNode {
    explicit AnimatedNode(const std::string& name) : m_nodeName(name) {}

    std::vector<SQT> m_nodeKeys;
    std::string m_nodeName;
};

struct Animation {
    Animation() = default;
    float m_duration = 0.0f;
    float m_ticksPerSecond = 0.0f;
    float m_finalTimeStamp = 0.0f;
    std::vector<AnimatedNode> m_animatedNodes;

    void SetFileInfo(const FileInfo& fileInfo);
    void SetLoadState(LoadState value);

    const FileInfo& GetFileInfo() const;
    LoadState GetLoadState() const;
    float GetTicksPerSecond() const;
    const std::string& GetName() const;
    size_t GetCPUAllocatedByteCount() const;

    void PrintNodeNames() const;

private:
    FileInfo m_fileInfo;
    LoadState m_loadState = LoadState::QUEUED;
};
