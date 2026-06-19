#pragma once
#include "Hell/Enums.h"
#include "Hell/VertexAttributes.h"
#include "UI/FontSpriteSheet.h"

#include <cstdint>
#include <string>
#include <vector>

namespace TextBlitter {
    void Init();

    void AddFont(const FontSpriteSheet& font);
    void BlitText(const std::string& text, const std::string& fontName, int originX, int originY, glm::ivec2 viewportSize, Alignment alignment, float scale, int32_t textureIndex, std::vector<Vertex2D>& vertices, std::vector<uint32_t>& indices);

    bool FontExists(const std::string& fontName);
    FontSpriteSheet* GetFontSpriteSheet(const std::string& name);
    glm::ivec2 GetTextSize(const std::string& text, const std::string& fontName, float scale);
}
