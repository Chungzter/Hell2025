#pragma once
#include "Hell/Enums.h"
#include "Hell/VertexAttributes.h"
#include "UI/FontSpriteSheet.h"

#include <string>

struct MeshData2D {
    std::vector<Vertex2D> vertices;
    std::vector<uint32_t> indices;
};

namespace TextBlitter {
    void AddFont(const FontSpriteSheet& font);
    MeshData2D BlitText(const std::string& text, const std::string& fontName, int originX, int originY, glm::ivec2 viewportSize, Alignment alignment, float scale, uint32_t baseVertex);
    FontSpriteSheet* GetFontSpriteSheet(const std::string& name);
    glm::ivec2 GetBlitTextSize(const std::string& text, const std::string& fontName, float scale);
}