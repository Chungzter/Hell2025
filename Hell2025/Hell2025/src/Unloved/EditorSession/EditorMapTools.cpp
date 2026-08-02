#include "EditorMapTools.h"

#include "Unloved/EditorSession/UI/EditorLayout.h"
#include "EditorSession.h"
#include "Unloved/EditorSession/UI/EditorStyle.h"
#include "Unloved/EditorSession/UI/EditorUI.h"
#include "Unloved/EditorSession/Core/EditorViewports.h"

#include "Hell/Common/Enum.h"

#include "Unloved/Debug/Debug.h"

#include <array>

namespace Unloved::EditorSession::MapTools {
    namespace {
        constexpr int32_t RENDER_MODE_COUNT = 3;
        constexpr int32_t SCULPT_TOOL_COUNT = 4;
        constexpr int32_t TERRAIN_TOOL_COUNT = 3;
        constexpr int32_t RENDER_MODE_BUTTON_OFFSET = 1;
        constexpr int32_t SCULPT_BUTTON_OFFSET = RENDER_MODE_BUTTON_OFFSET + RENDER_MODE_COUNT;
        constexpr int32_t TERRAIN_BUTTON_OFFSET = SCULPT_BUTTON_OFFSET + SCULPT_TOOL_COUNT;
        constexpr int32_t LAYERS_BUTTON_INDEX = TERRAIN_BUTTON_OFFSET + TERRAIN_TOOL_COUNT;
        constexpr int32_t BUTTON_COUNT = LAYERS_BUTTON_INDEX + 1;
        constexpr int32_t MAX_VIEWPORT_COUNT = 4;

        const std::array<const char*, RENDER_MODE_COUNT> RENDER_MODE_ICON_TEXTURE_NAMES = {
            "render_pbr_button",
            "render_base_color_button",
            "render_normals_button"
        };

        const std::array<EditorRenderMode, RENDER_MODE_COUNT> RENDER_MODES = {
            EditorRenderMode::PBR,
            EditorRenderMode::SOLID_COLOR,
            EditorRenderMode::NORMALS
        };

        const std::array<const char*, SCULPT_TOOL_COUNT> SCULPT_ICON_TEXTURE_NAMES = {
            "height_add",
            "height_flat",
            "height_slope",
            "height_smooth"
        };

        const std::array<HeightMapTool, SCULPT_TOOL_COUNT> SCULPT_TOOLS = {
            HeightMapTool::ADD,
            HeightMapTool::FLAT,
            HeightMapTool::SLOPE,
            HeightMapTool::SMOOTH
        };

        const std::array<const char*, TERRAIN_TOOL_COUNT> TERRAIN_ICON_TEXTURE_NAMES = {
            "texture_paint",
            "texture_spray",
            "autoshader"
        };

        const std::array<HeightMapTool, TERRAIN_TOOL_COUNT> TERRAIN_TOOLS = {
            HeightMapTool::TEXTURE_PAINT,
            HeightMapTool::TEXTURE_SPRAY,
            HeightMapTool::AUTO_SHADER
        };

        std::array<EditorButton, BUTTON_COUNT> g_buttons;
        Mode g_mode = Mode::OBJECT;
        EditorRenderMode g_renderMode = EditorRenderMode::PBR;
        HeightMapTool g_heightMapTool = HeightMapTool::ADD;
        bool g_terrainLayersOpen = false;
        bool g_wantsMouseCapture = false;

        bool IsMapEditorVisible() {
            return Unloved::EditorSession::HasMode() && Unloved::EditorSession::GetMode() == EditorSessionMode::MAP;
        }

        void SelectHeightMapTool(HeightMapTool tool) {
            g_heightMapTool = tool;
            Debug::BlitQuickDebugMessage(Hell::Enum::ToString(g_heightMapTool));
        }

        const EditorViewportRegion* GetToolbarViewport() {
            for (int32_t viewportIndex = 0; viewportIndex < MAX_VIEWPORT_COUNT; viewportIndex++) {
                const EditorViewportRegion* region = Layout::GetViewportRegionByIndex(viewportIndex);
                if (region && region->visible) {
                    return region;
                }
            }
            return nullptr;
        }
    }

    void Init() {
        g_buttons[0].onPressed = [] {
            g_mode = g_mode == Mode::OBJECT ? Mode::HEIGHT_MAP : Mode::OBJECT;
            Debug::BlitQuickDebugMessage(Hell::Enum::ToString(g_mode));
        };

        for (int32_t renderModeIndex = 0; renderModeIndex < RENDER_MODE_COUNT; renderModeIndex++) {
            EditorButton& button = g_buttons[renderModeIndex + RENDER_MODE_BUTTON_OFFSET];
            button.iconTextureName = RENDER_MODE_ICON_TEXTURE_NAMES[renderModeIndex];
            button.onPressed = [renderModeIndex] {
                g_renderMode = RENDER_MODES[renderModeIndex];
                Debug::BlitQuickDebugMessage(Hell::Enum::ToString(g_renderMode));
            };
        }

        for (int32_t toolIndex = 0; toolIndex < SCULPT_TOOL_COUNT; toolIndex++) {
            EditorButton& button = g_buttons[toolIndex + SCULPT_BUTTON_OFFSET];
            button.iconTextureName = SCULPT_ICON_TEXTURE_NAMES[toolIndex];
            button.onPressed = [toolIndex] { SelectHeightMapTool(SCULPT_TOOLS[toolIndex]); };
        }

        for (int32_t toolIndex = 0; toolIndex < TERRAIN_TOOL_COUNT; toolIndex++) {
            EditorButton& button = g_buttons[toolIndex + TERRAIN_BUTTON_OFFSET];
            button.iconTextureName = TERRAIN_ICON_TEXTURE_NAMES[toolIndex];
            button.onPressed = [toolIndex] { SelectHeightMapTool(TERRAIN_TOOLS[toolIndex]); };
        }

        g_buttons[LAYERS_BUTTON_INDEX].iconTextureName = "layers";
        g_buttons[LAYERS_BUTTON_INDEX].onPressed = [] {
            g_terrainLayersOpen = !g_terrainLayersOpen;
            Debug::BlitQuickDebugMessage("LAYERS");
        };

        Reset();
    }

    void Reset() {
        g_mode = Mode::OBJECT;
        g_renderMode = EditorRenderMode::PBR;
        g_heightMapTool = HeightMapTool::ADD;
        g_terrainLayersOpen = false;
        g_wantsMouseCapture = false;
        for (EditorButton& button : g_buttons) {
            button.visible = false;
            button.hovered = false;
            button.selected = false;
        }
    }

    void Update(bool allowInput) {
        const EditorMapToolsStyle& style = GetStyle().mapTools;
        const bool mapEditorVisible = IsMapEditorVisible();
        const bool editorVisible = Unloved::EditorSession::HasMode();
        const bool inputEnabled = allowInput && !Viewports::IsFlyMode();
        const glm::ivec2 mousePosition = Coordinates::GetMousePositionUI();
        const EditorViewportRegion* region = GetToolbarViewport();
        const bool viewportVisible = editorVisible && region;
        const int32_t firstRenderModeSlot = mapEditorVisible ? 1 : 0;
        g_wantsMouseCapture = false;

        // Editor mode
        EditorButton& modeButton = g_buttons[0];
        modeButton.visible = viewportVisible && mapEditorVisible;
        modeButton.iconTextureName = g_mode == Mode::OBJECT ? "heightmap_mode" : "object_mode";
        if (viewportVisible) {
            modeButton.rect = { region->rect.x + style.viewportPadding, region->rect.y + style.viewportPadding + style.labelHeight + style.buttonGap, style.buttonSize, style.buttonSize };
        }

        Buttons::Update(modeButton, mousePosition, inputEnabled);
        g_wantsMouseCapture = modeButton.hovered;

        // Render modes
        for (int32_t renderModeIndex = 0; renderModeIndex < RENDER_MODE_COUNT; renderModeIndex++) {
            EditorButton& button = g_buttons[renderModeIndex + RENDER_MODE_BUTTON_OFFSET];
            button.visible = viewportVisible;
            button.selected = g_renderMode == RENDER_MODES[renderModeIndex];
            if (viewportVisible) {
                button.rect = { region->rect.x + style.viewportPadding, region->rect.y + style.viewportPadding + style.labelHeight + style.buttonGap + (renderModeIndex + firstRenderModeSlot) * (style.buttonSize + style.buttonGap), style.buttonSize, style.buttonSize };
            }

            Buttons::Update(button, mousePosition, inputEnabled);
            g_wantsMouseCapture = g_wantsMouseCapture || button.hovered;
        }

        // Sculpt tools
        for (int32_t toolIndex = 0; toolIndex < SCULPT_TOOL_COUNT; toolIndex++) {
            EditorButton& button = g_buttons[toolIndex + SCULPT_BUTTON_OFFSET];
            button.visible = viewportVisible && mapEditorVisible && g_mode == Mode::HEIGHT_MAP;
            button.selected = g_heightMapTool == SCULPT_TOOLS[toolIndex];
            if (viewportVisible) {
                button.rect = { region->rect.x + style.viewportPadding, region->rect.y + style.viewportPadding + style.labelHeight + style.buttonGap + (toolIndex + SCULPT_BUTTON_OFFSET) * (style.buttonSize + style.buttonGap), style.buttonSize, style.buttonSize };
            }

            Buttons::Update(button, mousePosition, inputEnabled);
            g_wantsMouseCapture = g_wantsMouseCapture || button.hovered;
        }

        // Terrain paint tools
        for (int32_t toolIndex = 0; toolIndex < TERRAIN_TOOL_COUNT; toolIndex++) {
            EditorButton& button = g_buttons[toolIndex + TERRAIN_BUTTON_OFFSET];
            button.visible = viewportVisible && mapEditorVisible && g_mode == Mode::HEIGHT_MAP;
            button.selected = g_heightMapTool == TERRAIN_TOOLS[toolIndex];
            if (viewportVisible) {
                button.rect = { region->rect.x + style.viewportPadding, region->rect.y + style.viewportPadding + style.labelHeight + style.buttonGap + (toolIndex + TERRAIN_BUTTON_OFFSET) * (style.buttonSize + style.buttonGap), style.buttonSize, style.buttonSize };
            }

            Buttons::Update(button, mousePosition, inputEnabled);
            g_wantsMouseCapture = g_wantsMouseCapture || button.hovered;
        }

        // Terrain layers
        EditorButton& layersButton = g_buttons[LAYERS_BUTTON_INDEX];
        layersButton.visible = viewportVisible && mapEditorVisible && g_mode == Mode::HEIGHT_MAP;
        layersButton.selected = g_terrainLayersOpen;
        if (viewportVisible) {
            layersButton.rect = { region->rect.x + style.viewportPadding, region->rect.y + style.viewportPadding + style.labelHeight + style.buttonGap + LAYERS_BUTTON_INDEX * (style.buttonSize + style.buttonGap), style.buttonSize, style.buttonSize };
        }

        Buttons::Update(layersButton, mousePosition, inputEnabled);
        g_wantsMouseCapture = g_wantsMouseCapture || layersButton.hovered;
    }

    void Render() {
        if (!Unloved::EditorSession::HasMode()) return;

        for (const EditorButton& button : g_buttons) {
            Buttons::Render(button);
        }
    }

    void SetMode(Mode mode) {
        g_mode = mode;
    }

    Mode GetMode() {
        return g_mode;
    }

    EditorRenderMode GetRenderMode() {
        return g_renderMode;
    }

    HeightMapTool GetHeightMapTool() {
        return g_heightMapTool;
    }

    bool IsTerrainLayersOpen() {
        return g_terrainLayersOpen;
    }

    bool WantsMouseCapture() {
        return g_wantsMouseCapture;
    }
}
