#pragma once
#include <Game/Constants.h>
#include <Game/Types.h>
#include <vector>

struct IESProfile {
    IESProfile() = default;
    void Load(const FileInfo& fileInfo, int32_t textureIndex);
    void PrintDebugInfo();

    float GetMinVerticalAngle() const;
    float GetMaxVerticalAngle() const;
    float GetMinHorizontalAngle() const;
    float GetMaxHorizontalAngle() const;
    float GetVerticalAngleRange() const;
    float GetHorizontalAngleRange() const;

    const std::string& GetName() const                 { return m_name; }
    const std::vector<float>& GetCandelaValues() const { return m_candelaValues; }
    int32_t GetTextureIndex() const                    { return m_textureIndex; }
    int32_t GetHorizontalAngleCount() const            { return m_horizontalAngleCount; }
    int32_t GetVerticalAngleCount() const              { return m_verticalAngleCount; }
    float GetMaxIntensity() const                      { return m_maxIntensity; }
    float GetVScale() const                            { return m_vScale; }
    float GetVBias() const                             { return m_vBias; }
    float GetHScale() const                            { return m_hScale; }
    float GetHBias() const                             { return m_hBias; }

private:
    std::string m_name = UNDEFINED_STRING;
    std::vector<float> m_verticalAngles;
    std::vector<float> m_horizontalAngles;
    std::vector<float> m_candelaValues;
    int32_t m_horizontalAngleCount = 0;
    int32_t m_verticalAngleCount = 0;
    int32_t m_textureIndex = 0;
    float m_maxIntensity = 0.0f;
    float m_vScale = 0.0f;
    float m_vBias = 0.0f;
    float m_hScale = 0.0f;
    float m_hBias = 0.0f;
};