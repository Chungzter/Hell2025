#include "UIBackEnd.h"

#include "Hell/Logging.h"

#include "AssetManagement/AssetManager.h"
#include "BackEnd/BackEnd.h"
#include "Config/Config.h"
#include "ResourceManagement/ResourceManager.h"
#include "UI/TextBlitter.h"

namespace UIBackEnd {
    std::vector<RenderItemUI> g_renderItems;
    std::vector<Vertex2D> g_vertices;
    std::vector<uint32_t> g_indices;

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

    void BlitText(const std::string& text, const std::string& fontName, glm::ivec2 location, Alignment alignment, float scale, TextureFilter textureFilter) {
        BlitText(text, fontName, location.x, location.y, alignment, scale, textureFilter);
    }

    void BlitText(const std::string& text, const std::string& fontName, int originX, int originY, Alignment alignment, float scale, TextureFilter textureFilter) {
        int textureIndex = AssetManager::GetTextureIndexByName(fontName);
        if (textureIndex == -1) {
            std::cout << "UIBackEnd::BlitText() failed to find texture " << fontName << "\n";
            return;
        }

        size_t baseIndex = g_indices.size();
        const Resolutions& resolutions = Config::GetResolutions();

        if (!TextBlitter::FontExists(fontName)) {
            Logging::Error() << "UIBackEnd::BlitText() failed to find " << fontName << "\n";
            return;
        }

        TextBlitter::BlitText(text, fontName, originX, originY, resolutions.ui, alignment, scale, textureIndex, g_vertices, g_indices);
        
        size_t indexCount = g_indices.size() - baseIndex;
        if (indexCount == 0) return;

        RenderItemUI& renderItem = g_renderItems.emplace_back();
        renderItem.baseVertex = 0;
        renderItem.baseIndex = static_cast<int>(baseIndex);
        renderItem.indexCount = static_cast<int>(indexCount);
        renderItem.textureIndex = textureIndex;
        renderItem.filter = (textureFilter == TextureFilter::NEAREST) ? 1 : 0;
        renderItem.clipMinX = 0;
        renderItem.clipMinY = 0;
        renderItem.clipMaxX = resolutions.ui.x;
        renderItem.clipMaxY = resolutions.ui.y;
    }

    void BlitTexture(BlitTextureInfo info) {
        BlitTexture(info.textureName, info.location, info.alignment, info.colorTint, info.size, info.textureFilter, info.rotation, info.clipMinX, info.clipMinY, info.clipMaxX, info.clipMaxY);
    }

    void BlitTexture(const std::string& textureName, glm::ivec2 location, Alignment alignment, glm::vec4 colorTint, glm::ivec2 size, TextureFilter textureFilter, float rotation, int clipMinX, int clipMinY, int clipMaxX, int clipMaxY) {
        // Bail if texture not found
        int textureIndex = AssetManager::GetTextureIndexByName(textureName);
        if (textureIndex == -1) {
            std::cout << "BlitTexture() failed. Could not find texture '" << textureName << "'\n";
            return;
        }
        // Get texture dimensions
        Texture* texture = AssetManager::GetTextureByIndex(textureIndex);
        float w = (size.x != -1) ? size.x : texture->GetWidth();
        float h = (size.y != -1) ? size.y : texture->GetHeight();

        glm::vec2 positions[4];
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
        const Resolutions& resolutions = Config::GetResolutions();
        glm::vec2 finalVertices[4];
        for (int i = 0; i < 4; ++i) {
            glm::vec2 screenPos = glm::vec2(anchor) + positions[i];
            finalVertices[i].x = (screenPos.x / static_cast<float>(resolutions.ui.x)) * 2.0f - 1.0f;
            finalVertices[i].y = 1.0f - (screenPos.y / static_cast<float>(resolutions.ui.y)) * 2.0f;
        }

        int baseVertex = g_vertices.size();
        g_vertices.reserve(baseVertex + 4);
        g_vertices.push_back({ { finalVertices[0].x, finalVertices[0].y }, uvs[0], colorTint, textureIndex });
        g_vertices.push_back({ { finalVertices[1].x, finalVertices[1].y }, uvs[1], colorTint, textureIndex });
        g_vertices.push_back({ { finalVertices[2].x, finalVertices[2].y }, uvs[2], colorTint, textureIndex });
        g_vertices.push_back({ { finalVertices[3].x, finalVertices[3].y }, uvs[3], colorTint, textureIndex });

        int baseIndex = g_indices.size();
        g_indices.reserve(baseIndex + 6);
        g_indices.push_back(baseVertex + 0);
        g_indices.push_back(baseVertex + 1);
        g_indices.push_back(baseVertex + 2);
        g_indices.push_back(baseVertex + 0);
        g_indices.push_back(baseVertex + 2);
        g_indices.push_back(baseVertex + 3);

        RenderItemUI& renderItem = g_renderItems.emplace_back();
        renderItem.baseVertex = 0;
        renderItem.baseIndex = baseIndex;
        renderItem.indexCount = 6;
        renderItem.textureIndex = textureIndex;
        renderItem.filter = (textureFilter == TextureFilter::NEAREST) ? 1 : 0;
        renderItem.clipMinX = clipMinX;
        renderItem.clipMinY = clipMinY;
        renderItem.clipMaxX = clipMaxX;
        renderItem.clipMaxY = clipMaxY;

        // Maybe tidy this up later
        const int W = BackEnd::GetCurrentWindowWidth();
        const int H = BackEnd::GetCurrentWindowHeight();

        renderItem.clipMinX = (clipMinX >= 0) ? clipMinX : 0;
        renderItem.clipMinY = (clipMinY >= 0) ? clipMinY : 0;
        renderItem.clipMaxX = (clipMaxX >= 0) ? clipMaxX : W;
        renderItem.clipMaxY = (clipMaxY >= 0) ? clipMaxY : H;
    }

    const std::vector<RenderItemUI>& GetRenderItems() {
        return g_renderItems;
    }
}
