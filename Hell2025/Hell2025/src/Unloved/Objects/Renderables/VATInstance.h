#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <cstdint>
#include <string>

struct VATInstanceCreateInfo {
    std::string resourceName;
    float playbackSpeed = 1.0f;
    bool loop = true;
};

struct VATInstance {
    void Init(const VATInstanceCreateInfo& createInfo);
    void Update(float deltaTime, const glm::mat4& modelMatrix);
    void ResetPlayTime()                              { m_currentTime = 0.0f; m_currentFrameIdx = 0; }
    void SetPlayTime(float playTime)                  { m_currentTime = playTime; }

    int32_t GetCurrentFrameIndex() const            { return m_currentFrameIdx; }
    int32_t GetFrameCount() const                   { return m_frameCount; }
    float GetCurrentTime() const                    { return m_currentTime; }
    float GetFPS() const                            { return m_fps; }
    const std::string& GetResourceName() const      { return m_createInfo.resourceName; }
    int32_t GetPositionTextureIndex() const         { return m_positionTextureIndex; }
    int32_t GetRotationTextureIndex() const         { return m_rotationTextureIndex; }
    int32_t GetLookupTextureIndex() const           { return m_lookupTextureIndex; }
    bool HasValidTextureIndices() const             { return m_positionTextureIndex != -1 && m_rotationTextureIndex != -1 && m_lookupTextureIndex != -1; }

private:
    float m_currentTime = 0.0f;
    float m_fps = 0.0f;
    int32_t m_frameCount = 0;
    int32_t m_currentFrameIdx = 0;
    int32_t m_positionTextureIndex = -1;
    int32_t m_rotationTextureIndex = -1;
    int32_t m_lookupTextureIndex = -1;
    VATInstanceCreateInfo m_createInfo;
};
