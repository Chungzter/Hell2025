#pragma once

#include <cstdint>
#include <glm/vec3.hpp>
#include <string>

namespace Unloved::EditorSession::HeightMapEditor {

    enum class BrushType : uint8_t {
        ACRYLIC_1,
        CIRCLE_0,
        CIRCLE_1,
        CIRCLE_2,
        CIRCLE_3,
        CIRCLE_4,
        HILL_1,
        HILL_2,
        MOUNTAIN_1,
        MOUNTAIN_2,
        MOUNTAIN_3,
        MOUNTAIN_4,
        PEAK_1,
        PEAK_2,
        PEAK_3,
        RING_1,
        SMOKE,
        SQUARE_1,
        SQUARE_2,
        SQUARE_3,
        SQUARE_4,
        SQUARE_5,
        STONES,
        TERRAIN_1,
        TERRAIN_2,
        TERRAIN_3,
        TERRAIN_4,
        TERRAIN_5,
        TERRAIN_6,
        TEXTURE_1,
        TEXTURE_2,
        TEXTURE_3,
        TEXTURE_4,
        TEXTURE_5,
        VEGETATION_1,
        COUNT
    };

    struct BrushPreview {
        glm::vec3 position = glm::vec3(0.0f);
        float radius = 0.0f;
        int32_t viewportIndex = -1;
        bool visible = false;
    };

    void Update(bool allowMouseInput);

    void SetBrushSize(float value);
    void SetBrushStrength(float value);
    void SetTargetHeight(float value);
    void SetBrushRotation(float value);
    void SetBrushGamma(float value);
    void SetBrushType(BrushType value);
    void SetSelectedTerrainMaterial(uint8_t materialIndex);

    float GetBrushSize();
    float GetBrushStrength();
    float GetTargetHeight();
    float GetBrushRotation();
    float GetBrushGamma();
    BrushType GetBrushType();
    const char* GetBrushTypeName(BrushType value);
    std::string GetBrushTextureName(BrushType value);
    uint8_t GetSelectedTerrainMaterial();
    const BrushPreview& GetBrushPreview();
}
