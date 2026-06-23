#pragma once
#include "Common/LoadingState.h"

#include "Hell/File.h"

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

#include <map>
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
    std::map<std::string, unsigned int> m_NodeMapping;

    void SetFileInfo(const FileInfo& fileInfo);
    void SetLoadingState(LoadingState value);

    const FileInfo& GetFileInfo() const;
    LoadingState GetLoadingState() const;
    float GetTicksPerSecond() const;
    const std::string& GetName() const;

    void PrintNodeNames() const;

private:
    FileInfo m_fileInfo;
    LoadingState m_loadingState { LoadingState::Value::AWAITING_LOADING_FROM_DISK };
};
