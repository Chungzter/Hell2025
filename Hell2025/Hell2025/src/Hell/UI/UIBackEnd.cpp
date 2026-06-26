#include "UIBackEnd.h"
#include "Hell/Backend/BackEnd.h"
#include "Hell/Logging.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/UI/TextBlitter.h"

using namespace Hell;

namespace UIBackEnd {
    std::vector<RenderItemUI> g_renderItems;
    std::vector<Vertex2D> g_vertices;
    std::vector<uint32_t> g_indices;
    uint32_t g_uiResolutionWidth = 1;
    uint32_t g_uiResolutionHeight = 1;

    void Init() {
        Logging::Init() << "Initialized the UI Backend";
        TextBlitter::Init();
    }

    void BeginFrame() {
        g_vertices.clear();
        g_indices.clear();
        g_renderItems.clear();
    }

    void Update() {
        GenericMesh& genericMesh = ResourceManager::GetGenericMesh("UI");
        genericMesh.UpdateVertexData(g_vertices);
        genericMesh.UpdateIndexData(g_indices);
    }

    void SetUIResolution(uint32_t width, uint32_t height) {
        g_uiResolutionWidth = width;
        g_uiResolutionHeight = height;
    }

    void BlitText(const std::string& text, const std::string& fontName, glm::ivec2 location, Alignment alignment, float scale, TextureFilter textureFilter) {
        BlitText(text, fontName, location.x, location.y, alignment, scale, textureFilter);
    }

    void BlitText(const std::string& text, const std::string& fontName, int originX, int originY, Alignment alignment, float scale, TextureFilter textureFilter) {
        int textureIndex = Hell::ResourceManager::GetTextureBindlessIndexByName(fontName);
        if (textureIndex == -1) {
            Logging::Error() << "UIBackEnd::BlitText(..) failed to find texture " << fontName << "\n";
            return;
        }

        size_t baseIndex = g_indices.size();

        if (!TextBlitter::FontExists(fontName)) {
            Logging::Error() << "UIBackEnd::BlitText(..) failed to find " << fontName << "\n";
            return;
        }

        const glm::ivec2 resolution(static_cast<int32_t>(g_uiResolutionWidth), static_cast<int32_t>(g_uiResolutionHeight));
        TextBlitter::BlitText(text, fontName, originX, originY, resolution, alignment, scale, textureIndex, g_vertices, g_indices);
        
        size_t indexCount = g_indices.size() - baseIndex;
        if (indexCount == 0) return;

        RenderItemUI& renderItem = g_renderItems.emplace_back();
        renderItem.baseVertex = 0u;
        renderItem.baseIndex = static_cast<uint32_t>(baseIndex);
        renderItem.indexCount = static_cast<uint32_t>(indexCount);
        renderItem.textureIndex = static_cast<uint32_t>(textureIndex);
        renderItem.filterMode = (textureFilter == TextureFilter::NEAREST) ? 1u : 0u;
        renderItem.clipMinX = 0;
        renderItem.clipMinY = 0;
        renderItem.clipMaxX = static_cast<int32_t>(g_uiResolutionWidth);
        renderItem.clipMaxY = static_cast<int32_t>(g_uiResolutionHeight);
    }

    void BlitTexture(BlitTextureInfo info) {
        BlitTexture(info.textureName, info.location, info.alignment, info.colorTint, info.size, info.textureFilter, info.rotation, info.clipMinX, info.clipMinY, info.clipMaxX, info.clipMaxY);
    }

    void BlitTexture(const std::string& textureName, glm::ivec2 location, Alignment alignment, glm::vec4 colorTint, glm::ivec2 size, TextureFilter textureFilter, float rotation, int clipMinX, int clipMinY, int clipMaxX, int clipMaxY) {
        // Bail if texture not found
        int textureIndex = Hell::ResourceManager::GetTextureBindlessIndexByName(textureName);
        if (textureIndex == -1) {
            Logging::Error() << "UIBackEnd::BlitTexture(..) failed to find texture " << textureName << "\n";
            return;
        }
        // Get texture dimensions
        Texture* texture = Hell::ResourceManager::GetTextureByBindlessIndex(textureIndex);
        const float w = (size.x != -1) ? static_cast<float>(size.x) : static_cast<float>(texture->GetWidth());
        const float h = (size.y != -1) ? static_cast<float>(size.y) : static_cast<float>(texture->GetHeight());

        glm::vec2 positions[4] = {};
        glm::vec2 uvs[4] = {
            { 0.0f, 0.0f },
            { 1.0f, 0.0f },
            { 1.0f, 1.0f },
            { 0.0f, 1.0f }
        };

        // Alignment
        switch (alignment) {
            case Alignment::TOP_LEFT:
                positions[0] = { 0, 0 }; positions[1] = { w, 0 };
                positions[2] = { w, h }; positions[3] = { 0, h };
                break;
            case Alignment::TOP_RIGHT:
                positions[0] = { -w, 0 }; positions[1] = { 0, 0 };
                positions[2] = { 0, h }; positions[3] = { -w, h };
                break;
            case Alignment::BOTTOM_LEFT:
                positions[0] = { 0, -h }; positions[1] = { w, -h };
                positions[2] = { w, 0 }; positions[3] = { 0, 0 };
                break;
            case Alignment::BOTTOM_RIGHT:
                positions[0] = { -w, -h }; positions[1] = { 0, -h };
                positions[2] = { 0, 0 }; positions[3] = { -w, 0 };
                break;
            case Alignment::CENTERED:
                positions[0] = { -w * 0.5f, -h * 0.5f }; positions[1] = { w * 0.5f, -h * 0.5f };
                positions[2] = { w * 0.5f,  h * 0.5f }; positions[3] = { -w * 0.5f,  h * 0.5f };
                break;
            case Alignment::CENTERED_HORIZONTAL:
                positions[0] = { -w * 0.5f, 0 }; positions[1] = { w * 0.5f, 0 };
                positions[2] = { w * 0.5f,  h }; positions[3] = { -w * 0.5f,  h };
                break;
            case Alignment::CENTERED_VERTICAL:
                positions[0] = { 0, -h * 0.5f }; positions[1] = { w, -h * 0.5f };
                positions[2] = { w, h * 0.5f }; positions[3] = { 0, h * 0.5f };
                break;
            default:
                return;
        };

        // Rotation
        float s = sin(rotation);
        float c = cos(rotation);

        for (int i = 0; i < 4; ++i) {
            float newX = positions[i].x * c - positions[i].y * s;
            float newY = positions[i].x * s + positions[i].y * c;
            positions[i] = { newX, newY };
        }

        // Snap to integer pixels
        glm::vec2 anchor = glm::round(glm::vec2(location));

        // Convert the final screen position to NDC
        glm::vec2 finalVertices[4] = {};

        for (int i = 0; i < 4; ++i) {
            glm::vec2 screenPos = glm::vec2(anchor) + positions[i];
            finalVertices[i].x = (screenPos.x / static_cast<float>(g_uiResolutionWidth)) * 2.0f - 1.0f;
            finalVertices[i].y = 1.0f - (screenPos.y / static_cast<float>(g_uiResolutionHeight)) * 2.0f;
        }

        const uint32_t baseVertex = static_cast<uint32_t>(g_vertices.size());
        g_vertices.reserve(baseVertex + 4u);
        g_vertices.push_back({ { finalVertices[0].x, finalVertices[0].y }, uvs[0], colorTint });
        g_vertices.push_back({ { finalVertices[1].x, finalVertices[1].y }, uvs[1], colorTint });
        g_vertices.push_back({ { finalVertices[2].x, finalVertices[2].y }, uvs[2], colorTint });
        g_vertices.push_back({ { finalVertices[3].x, finalVertices[3].y }, uvs[3], colorTint });

        const uint32_t baseIndex = static_cast<uint32_t>(g_indices.size());
        g_indices.reserve(baseIndex + 6u);
        g_indices.push_back(baseVertex + 0u);
        g_indices.push_back(baseVertex + 1u);
        g_indices.push_back(baseVertex + 2u);
        g_indices.push_back(baseVertex + 0u);
        g_indices.push_back(baseVertex + 2u);
        g_indices.push_back(baseVertex + 3u);

        RenderItemUI& renderItem = g_renderItems.emplace_back();
        renderItem.baseVertex = 0u;
        renderItem.baseIndex = baseIndex;
        renderItem.indexCount = 6u;
        renderItem.textureIndex = static_cast<uint32_t>(textureIndex);
        renderItem.filterMode = (textureFilter == TextureFilter::NEAREST) ? 1u : 0u;
        renderItem.clipMinX = clipMinX;
        renderItem.clipMinY = clipMinY;
        renderItem.clipMaxX = clipMaxX;
        renderItem.clipMaxY = clipMaxY;

        // Maybe tidy this up later
        const int W = Hell::BackEnd::GetCurrentWindowWidth();
        const int H = Hell::BackEnd::GetCurrentWindowHeight();

        renderItem.clipMinX = (clipMinX >= 0) ? clipMinX : 0;
        renderItem.clipMinY = (clipMinY >= 0) ? clipMinY : 0;
        renderItem.clipMaxX = (clipMaxX >= 0) ? clipMaxX : W;
        renderItem.clipMaxY = (clipMaxY >= 0) ? clipMaxY : H;
    }

    const std::vector<RenderItemUI>& GetRenderItems() {
        return g_renderItems;
    }
}
