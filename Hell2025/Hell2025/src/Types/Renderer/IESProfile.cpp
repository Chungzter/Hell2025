#include "IESProfile.h"
#include "Hell/Logging.h"
#include <tinyies/tiny_ies.hpp>

#include <iostream> // TODO clean up logging

void IESProfile::Load(const FileInfo& fileInfo, int32_t textureIndex) {
    m_name = fileInfo.name;
    m_textureIndex = textureIndex;

    tiny_ies<float>::light ies;
    std::string err;
    std::string warn;

    if (!tiny_ies<float>::load_ies(fileInfo.path, err, warn, ies)) {
        Logging::Fatal() << "Failed to load " << fileInfo.path << " " << err << "\n";
        return;
    }

    m_verticalAngles = std::move(ies.vertical_angles);
    m_horizontalAngles = std::move(ies.horizontal_angles);
    m_candelaValues = std::move(ies.candela);
    m_horizontalAngleCount = ies.number_horizontal_angles;
    m_verticalAngleCount = ies.number_vertical_angles;

    m_maxIntensity = ies.max_candela;

    // Normalize values for GPU [0.0, 1.0]
    if (m_maxIntensity > 0.0f) {
        for (float& val : m_candelaValues) {
            val /= m_maxIntensity;
        }
    }

    float vRange = GetVerticalAngleRange();
    float hRange = GetHorizontalAngleRange();

    // Precompute Scale and Bias to turn (x - min) / range into (x * scale + bias)
    m_vScale = (vRange != 0.0f) ? 1.0f / vRange : 0.0f;
    m_vBias = -GetMinVerticalAngle() * m_vScale;

    m_hScale = (hRange != 0.0f) ? 1.0f / hRange : 0.0f;
    m_hBias = -GetMinHorizontalAngle() * m_hScale;

    //PrintDebugInfo();
}

void IESProfile::PrintDebugInfo() {
    std::cout << "------------------------------------------\n";
    std::cout << "IES PROFILE LOADED: " << m_name << "\n";
    std::cout << "Data Points:    " << m_candelaValues.size() << "\n";
    std::cout << "Max Intensity:  " << m_maxIntensity << " cd\n";
    std::cout << "Grid Size:      " << m_verticalAngleCount << " (V) x " << m_horizontalAngleCount << " (H)\n";

    // Angle Ranges
    std::cout << "Vertical:       " << GetMinVerticalAngle() << " to " << GetMaxVerticalAngle() << " (Range: " << GetVerticalAngleRange() << ")\n";
    std::cout << "Horizontal:     " << GetMinHorizontalAngle() << " to " << GetMaxHorizontalAngle() << " (Range: " << GetHorizontalAngleRange() << ")\n";

    // Precomputed Shader Constants
    std::cout << "V-Scale/Bias:   " << m_vScale << " / " << m_vBias << "\n";
    std::cout << "H-Scale/Bias:   " << m_hScale << " / " << m_hBias << "\n";

    // Symmetry Detection
    if (GetHorizontalAngleRange() == 180.0f) {
        std::cout << "Note:           Bilateral Symmetry detected (180 deg range).\n";
    }

    std::cout << "------------------------------------------\n\n";
}

float IESProfile::GetMinVerticalAngle() const {
    return m_verticalAngles.empty() ? 0.0f : m_verticalAngles.front();
}

float IESProfile::GetMaxVerticalAngle() const {
    return m_verticalAngles.empty() ? 0.0f : m_verticalAngles.back();
}

float IESProfile::GetMinHorizontalAngle() const {
    return m_horizontalAngles.empty() ? 0.0f : m_horizontalAngles.front();
}

float IESProfile::GetMaxHorizontalAngle() const {
    return m_horizontalAngles.empty() ? 0.0f : m_horizontalAngles.back();
}

float IESProfile::GetVerticalAngleRange() const {
    return GetMaxVerticalAngle() - GetMinVerticalAngle();
}

float IESProfile::GetHorizontalAngleRange() const {
    return GetMaxHorizontalAngle() - GetMinHorizontalAngle();
}