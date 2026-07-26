#include "EditorMenuBar.h"

#include "EditorCoordinates.h"
#include "EditorLayout.h"
#include "EditorStyle.h"
#include "EditorUI.h"

#include "Hell/Backend/BackEnd.h"
#include "Hell/Input.h"
#include "Hell/UI/TextBlitter.h"
#include "Hell/UI/UIBackEnd.h"

#include "Unloved/Common/Constants.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace Unloved::EditorSession::MenuBar {
    namespace {
        constexpr int32_t MENU_BAR_LEFT_PADDING = 10;
        constexpr int32_t MENU_BUTTON_HORIZONTAL_PADDING = 10;
        constexpr int32_t POPUP_MIN_WIDTH = 220;
        constexpr int32_t POPUP_VERTICAL_PADDING = 4;
        constexpr int32_t ITEM_HEIGHT = 24;
        constexpr int32_t SEPARATOR_HEIGHT = 9;
        constexpr int32_t ITEM_HORIZONTAL_PADDING = 12;
        constexpr int32_t SHORTCUT_GAP = 32;
        constexpr int32_t SUBMENU_ARROW_SIZE = 8;

        const glm::vec4 MENU_BUTTON_HOVER_COLOR = glm::vec4(0.141176f, 0.125490f, 0.168627f, 1.0f); // #24202b
        const glm::vec4 POPUP_BACKGROUND_COLOR = glm::vec4(0.058824f, 0.050980f, 0.070588f, 1.0f);  // #0f0d12
        const glm::vec4 ITEM_HOVER_COLOR = glm::vec4(0.141176f, 0.125490f, 0.168627f, 1.0f);        // #24202b
        const glm::vec4 BORDER_COLOR = glm::vec4(0.42f, 0.40f, 0.46f, 1.0f);
        const glm::vec4 SEPARATOR_COLOR = glm::vec4(0.24f, 0.23f, 0.27f, 1.0f);
        const glm::vec4 TEXT_COLOR = glm::vec4(0.545098f, 0.541176f, 0.568627f, 1.0f);

        enum class MenuItemKind : uint8_t {
            ACTION,
            SEPARATOR
        };

        struct MenuItem {
            MenuItemKind kind = MenuItemKind::ACTION;
            std::string label;
            std::string shortcut;
            EditorMenuAction action = EditorMenuAction::NONE;
            PlacementTool placementTool = PlacementTool::NONE;
            EditorRect rect;
            EditorRect popupRect;
            std::vector<MenuItem> children;
        };

        struct Menu {
            std::string label;
            std::vector<MenuItem> items;
            EditorRect buttonRect;
            EditorRect popupRect;
        };

        std::vector<Menu> g_menus;
        int32_t g_openMenuIndex = -1;
        int32_t g_hoveredMenuIndex = -1;
        MenuItem* g_hoveredItem = nullptr;
        std::vector<MenuItem*> g_openSubmenus;
        bool g_wantsMouseCapture = false;
        bool g_wantsKeyboardCapture = false;
        EditorMenuAction g_pendingAction = EditorMenuAction::NONE;
        PlacementTool g_pendingPlacementTool = PlacementTool::NONE;

        MenuItem Action(const char* label, const char* shortcut, EditorMenuAction action) {
            MenuItem item;
            item.label = label;
            item.shortcut = shortcut;
            item.action = action;
            return item;
        }

        MenuItem Separator() {
            MenuItem item;
            item.kind = MenuItemKind::SEPARATOR;
            return item;
        }

        MenuItem Tool(const char* label, PlacementTool placementTool) {
            MenuItem item;
            item.label = label;
            item.placementTool = placementTool;
            return item;
        }

        MenuItem Submenu(const char* label, std::vector<MenuItem> children) {
            MenuItem item;
            item.label = label;
            item.children = std::move(children);
            return item;
        }

        bool CanEmit(EditorMenuAction action) {
            return action != EditorMenuAction::NONE;
        }

        void CloseMenus() {
            g_openMenuIndex = -1;
            g_hoveredItem = nullptr;
            g_openSubmenus.clear();
        }

        glm::ivec2 GetPopupSize(const std::vector<MenuItem>& items) {
            int32_t width = POPUP_MIN_WIDTH;
            int32_t height = POPUP_VERTICAL_PADDING * 2;

            for (const MenuItem& item : items) {
                if (item.kind == MenuItemKind::SEPARATOR) {
                    height += SEPARATOR_HEIGHT;
                    continue;
                }

                const int32_t labelWidth = TextBlitter::GetTextSize(item.label, Style::FONT_NAME, Style::FONT_SCALE).x;
                const int32_t shortcutWidth = TextBlitter::GetTextSize(item.shortcut, Style::FONT_NAME, Style::FONT_SCALE).x;
                const int32_t shortcutGap = item.shortcut.empty() ? 0 : SHORTCUT_GAP;
                const int32_t submenuWidth = item.children.empty() ? 0 : SUBMENU_ARROW_SIZE + ITEM_HORIZONTAL_PADDING;
                width = std::max(width, ITEM_HORIZONTAL_PADDING * 2 + labelWidth + shortcutGap + shortcutWidth + submenuWidth);
                height += ITEM_HEIGHT;
            }

            return { width, height };
        }

        void UpdateItemGeometry(std::vector<MenuItem>& items, const EditorRect& popupRect, const EditorRect& menuBarRect) {
            int32_t itemY = popupRect.y + POPUP_VERTICAL_PADDING;
            for (MenuItem& item : items) {
                const int32_t itemHeight = item.kind == MenuItemKind::SEPARATOR ? SEPARATOR_HEIGHT : ITEM_HEIGHT;
                item.rect = { popupRect.x + 1, itemY, popupRect.width - 2, itemHeight };
                itemY += itemHeight;

                if (item.children.empty()) continue;

                const glm::ivec2 popupSize = GetPopupSize(item.children);
                int32_t popupX = popupRect.Right();
                if (popupX + popupSize.x > menuBarRect.Right()) popupX = popupRect.x - popupSize.x;
                const int32_t maximumPopupY = std::max(menuBarRect.Bottom(), Hell::BackEnd::GetDrawableHeight() - popupSize.y);
                const int32_t popupY = std::clamp(item.rect.y - POPUP_VERTICAL_PADDING, menuBarRect.Bottom(), maximumPopupY);
                item.popupRect = { popupX, popupY, popupSize.x, popupSize.y };
                UpdateItemGeometry(item.children, item.popupRect, menuBarRect);
            }
        }

        void UpdatePopupGeometry(Menu& menu, const EditorRect& menuBarRect) {
            const glm::ivec2 popupSize = GetPopupSize(menu.items);

            const int32_t maxPopupX = std::max(menuBarRect.x, menuBarRect.Right() - popupSize.x);
            const int32_t popupX = std::clamp(menu.buttonRect.x, menuBarRect.x, maxPopupX);
            menu.popupRect = { popupX, menuBarRect.Bottom(), popupSize.x, popupSize.y };
            UpdateItemGeometry(menu.items, menu.popupRect, menuBarRect);
        }

        void UpdateGeometry() {
            const EditorRect& menuBarRect = Layout::GetFileMenuPanel().rect;
            int32_t buttonX = menuBarRect.x + MENU_BAR_LEFT_PADDING;
            const int32_t buttonHeight = std::max(0, menuBarRect.height - 1);

            for (Menu& menu : g_menus) {
                const int32_t textWidth = TextBlitter::GetTextSize(menu.label, Style::FONT_NAME, Style::FONT_SCALE).x;
                const int32_t buttonWidth = std::max(40, textWidth + MENU_BUTTON_HORIZONTAL_PADDING * 2);
                menu.buttonRect = { buttonX, menuBarRect.y, buttonWidth, buttonHeight };
                buttonX += buttonWidth;
                UpdatePopupGeometry(menu, menuBarRect);
            }
        }

        int32_t GetHoveredMenuIndex(const glm::ivec2& mousePosition) {
            for (size_t i = 0; i < g_menus.size(); i++) {
                if (g_menus[i].buttonRect.Contains(mousePosition)) {
                    return static_cast<int32_t>(i);
                }
            }
            return -1;
        }

        struct HoveredItem {
            MenuItem* item = nullptr;
            size_t level = 0;
        };

        HoveredItem GetHoveredItem(Menu& menu, const glm::ivec2& mousePosition) {
            for (int32_t level = static_cast<int32_t>(g_openSubmenus.size()); level >= 0; level--) {
                std::vector<MenuItem>& items = level == 0 ? menu.items : g_openSubmenus[level - 1]->children;
                for (MenuItem& item : items) {
                    if (item.rect.Contains(mousePosition)) return { &item, static_cast<size_t>(level) };
                }
            }
            return {};
        }

        bool ControlIsDown() {
            return Hell::Input::KeyDown(HELL_KEY_LEFT_CONTROL_GLFW) || Hell::Input::KeyDown(HELL_KEY_RIGHT_CONTROL);
        }

        EditorMenuAction GetShortcutAction() {
            if (!ControlIsDown()) return EditorMenuAction::NONE;
            if (Hell::Input::KeyPressed(HELL_KEY_N)) return EditorMenuAction::NEW_MAP;
            if (Hell::Input::KeyPressed(HELL_KEY_O)) return EditorMenuAction::OPEN_MAP;
            if (Hell::Input::KeyPressed(HELL_KEY_S)) return EditorMenuAction::SAVE;
            return EditorMenuAction::NONE;
        }

        std::string WithColor(const char* color, const std::string& text) {
            return std::string(color) + text;
        }

        void RenderPopup(const std::vector<MenuItem>& items, const EditorRect& popupRect, size_t level) {
            UI::DrawSolidRect(popupRect, POPUP_BACKGROUND_COLOR);

            for (const MenuItem& item : items) {
                const EditorRect& itemRect = item.rect;

                if (item.kind == MenuItemKind::SEPARATOR) {
                    UI::DrawSolidRect({ itemRect.x + ITEM_HORIZONTAL_PADDING, itemRect.y + itemRect.height / 2, itemRect.width - ITEM_HORIZONTAL_PADDING * 2, 1 }, SEPARATOR_COLOR);
                    continue;
                }

                if (&item == g_hoveredItem) UI::DrawSolidRect(itemRect, ITEM_HOVER_COLOR);

                const int32_t centerY = itemRect.y + itemRect.height / 2;
                UIBackEnd::BlitText(UICanvas::NATIVE, WithColor(Style::TEXT_COLOR_TAG, item.label), Style::FONT_NAME, glm::ivec2(itemRect.x + ITEM_HORIZONTAL_PADDING, centerY), Alignment::CENTERED_VERTICAL, Style::FONT_SCALE, TextureFilter::NEAREST);

                if (!item.children.empty()) {
                    UIBackEnd::BlitTexture(UICanvas::NATIVE, "DropDownArrow", glm::ivec2(itemRect.Right() - ITEM_HORIZONTAL_PADDING - SUBMENU_ARROW_SIZE / 2, centerY), Alignment::CENTERED, TEXT_COLOR, glm::ivec2(SUBMENU_ARROW_SIZE), TextureFilter::NEAREST, HELL_PI * -0.5f);
                }
                else if (!item.shortcut.empty()) {
                    const int32_t shortcutWidth = TextBlitter::GetTextSize(item.shortcut, Style::FONT_NAME, Style::FONT_SCALE).x;
                    UIBackEnd::BlitText(UICanvas::NATIVE, WithColor(Style::TEXT_COLOR_TAG, item.shortcut), Style::FONT_NAME, glm::ivec2(itemRect.Right() - ITEM_HORIZONTAL_PADDING - shortcutWidth, centerY), Alignment::CENTERED_VERTICAL, Style::FONT_SCALE, TextureFilter::NEAREST);
                }
            }

            EditorPanel popupPanel;
            popupPanel.rect = popupRect;
            popupPanel.edges = EditorPanelEdge::ALL;
            popupPanel.borderColor = BORDER_COLOR;
            popupPanel.borderThickness = 1;
            popupPanel.drawBackground = false;
            UI::DrawPanelEdges(popupPanel);

            if (level >= g_openSubmenus.size()) return;
            const MenuItem* submenu = g_openSubmenus[level];
            RenderPopup(submenu->children, submenu->popupRect, level + 1);
        }
    }

    void Init() {
        g_menus.clear();

        Menu fileMenu;
        fileMenu.label = "File";
        fileMenu.items = {
            Action("New",          "Ctrl+N", EditorMenuAction::NEW_MAP),
            Action("Open...",      "Ctrl+O", EditorMenuAction::OPEN_MAP),
            Action("Save",         "Ctrl+S", EditorMenuAction::SAVE),
            Separator(),
            Action("Close Editor", "`",      EditorMenuAction::CLOSE_EDITOR),
            Action("Exit",         "",       EditorMenuAction::EXIT_APPLICATION)
        };
        g_menus.push_back(std::move(fileMenu));

        Menu insertMenu;
        insertMenu.label = "Insert";
        insertMenu.items = {
            Action("Reinsert last", "Ctrl+T", EditorMenuAction::NONE),
            Submenu("Bathroom", { Tool("Basin", PlacementTool::GENERIC_BATHROOM_BASIN), Tool("Cabinet", PlacementTool::GENERIC_BATHROOM_CABINET), Tool("Toilet", PlacementTool::GENERIC_TOILET) }),
            Submenu("Christmas", { Tool("Christmas Lights", PlacementTool::CHRISTMAS_LIGHTS), Tool("Present Small", PlacementTool::GENERIC_CHRISTMAS_PRESENT_SMALL), Tool("Present Large", PlacementTool::GENERIC_CHRISTMAS_PRESENT_LARGE), Tool("Tree", PlacementTool::GENERIC_CHRISTMAS_TREE) }),
            Submenu("Enemies", { Tool("Dobermann", PlacementTool::DOBERMANN), Tool("Kangaroo", PlacementTool::KANGAROO), Tool("Shark", PlacementTool::SHARK) }),
            Submenu("Furniture", { Tool("Couch", PlacementTool::GENERIC_COUCH), Submenu("Chairs", { Tool("Chair RE", PlacementTool::GENERIC_CHAIR_RE), Tool("Chair Spindle Back", PlacementTool::GENERIC_CHAIR_SPINDLE_BACK) }), Submenu("Drawers", { Tool("Small", PlacementTool::GENERIC_DRAWERS_SMALL), Tool("Large", PlacementTool::GENERIC_DRAWERS_LARGE) }) }),
            Submenu("Fishing", { Action("Jetty", "", EditorMenuAction::NONE) }),
            Submenu("Rural", { Tool("Fence", PlacementTool::FENCE_FARM), Tool("Power Pole", PlacementTool::POWER_POLES) }),
            Submenu("House", { Submenu("Wall", { Action("Interior", "", EditorMenuAction::NONE), Action("Weather Boards", "", EditorMenuAction::NONE) }), Tool("Door", PlacementTool::DOOR_STANDARD_A), Tool("Window", PlacementTool::WINDOW), Tool("Staircase", PlacementTool::STAIRCASE), Submenu("Fireplace", { Tool("Open", PlacementTool::FIREPLACE_OPEN), Tool("Stove", PlacementTool::FIREPLACE_WOOD_STOVE) }) }),
            Submenu("Misc", { Tool("Ladder", PlacementTool::LADDER) }),
            Submenu("Plants", { Tool("Tree", PlacementTool::GENERIC_PLANT_TREE), Tool("Black Berries", PlacementTool::GENERIC_PLANT_BLACKBERRIES) }),
            Submenu("Lighting", { Tool("Christmas Lights", PlacementTool::CHRISTMAS_LIGHTS), Action("DDGI Volume", "", EditorMenuAction::NONE), Action("Light", "", EditorMenuAction::NONE) }),
            Submenu("Mermaids", { Action("Mermaid Shop Owner", "", EditorMenuAction::NONE), Action("Mermaid Visitor Rock", "", EditorMenuAction::NONE) }),
            Submenu("Pick Ups", { Submenu("Weapons", { Tool("AKS74U", PlacementTool::PICKUP_AKS74U), Tool("FN-P90", PlacementTool::PICKUP_P90), Tool("Glock", PlacementTool::PICKUP_GLOCK), Tool("Golden Glock", PlacementTool::PICKUP_GOLDEN_GLOCK), Tool("Knife", PlacementTool::PICKUP_KNIFE), Tool("Remington 870", PlacementTool::PICKUP_REMINGTON_870), Tool("SPAS", PlacementTool::PICKUP_SPAS), Tool("Tokarev", PlacementTool::PICKUP_TOKAREV) }), Submenu("Ammo", { Tool("Shotgun Shells Buckshot", PlacementTool::PICKUP_12_GAUGE_BUCKSHOT) }), Submenu("Items", { Tool("Black Skull", PlacementTool::PICKUP_BLACK_SKULL), Tool("Relief Pills", PlacementTool::PICKUP_PILLS), Tool("Small Key", PlacementTool::PICKUP_SMALL_KEY), Tool("Small Key Silver", PlacementTool::PICKUP_SMALL_KEY_SILVER) }) }),
            Submenu("Test Models", { Action("Test Model 1", "", EditorMenuAction::NONE), Action("Test Model 2", "", EditorMenuAction::NONE), Action("Test Model 3", "", EditorMenuAction::NONE), Action("Test Model 4", "", EditorMenuAction::NONE) })
        };
        g_menus.push_back(std::move(insertMenu));

        Menu viewportMenu;
        viewportMenu.label = "Viewport";
        viewportMenu.items = {
            Action("Single",             "", EditorMenuAction::VIEWPORT_SINGLE),
            Action("Left / Right split", "", EditorMenuAction::VIEWPORT_LEFT_RIGHT),
            Action("Top / Bottom split", "", EditorMenuAction::VIEWPORT_TOP_BOTTOM),
            Action("Four way split",     "", EditorMenuAction::VIEWPORT_FOUR)
        };
        g_menus.push_back(std::move(viewportMenu));

        g_pendingAction = EditorMenuAction::NONE;
        g_pendingPlacementTool = PlacementTool::NONE;
        Close();
    }

    void RefreshLayout() {
        UpdateGeometry();
    }

    void Update() {
        g_wantsMouseCapture = false;
        g_wantsKeyboardCapture = false;
        UpdateGeometry();

        const glm::ivec2 mousePosition = Coordinates::GetMousePositionUI();
        const EditorRect& menuBarRect = Layout::GetFileMenuPanel().rect;
        const int32_t openMenuAtFrameStart = g_openMenuIndex;
        const bool menuWasOpen = g_openMenuIndex >= 0;
        bool mousePressConsumed = false;

        g_hoveredMenuIndex = GetHoveredMenuIndex(mousePosition);
        if (g_openMenuIndex >= 0 && g_hoveredMenuIndex >= 0 && g_hoveredMenuIndex != g_openMenuIndex) {
            g_openMenuIndex = g_hoveredMenuIndex;
            g_openSubmenus.clear();
        }

        g_hoveredItem = nullptr;
        if (g_openMenuIndex >= 0) {
            const HoveredItem hoveredItem = GetHoveredItem(g_menus[g_openMenuIndex], mousePosition);
            g_hoveredItem = hoveredItem.item;
            if (g_hoveredItem && g_hoveredItem->kind == MenuItemKind::ACTION) {
                g_openSubmenus.resize(hoveredItem.level);
                if (!g_hoveredItem->children.empty()) g_openSubmenus.push_back(g_hoveredItem);
            }
        }

        if (Hell::Input::LeftMousePressed()) {
            if (g_hoveredMenuIndex >= 0) {
                g_openMenuIndex = openMenuAtFrameStart == g_hoveredMenuIndex ? -1 : g_hoveredMenuIndex;
                g_hoveredItem = nullptr;
                g_openSubmenus.clear();
                mousePressConsumed = true;
            }
            else if (g_openMenuIndex >= 0) {
                if (g_hoveredItem) {
                    if (g_hoveredItem->kind == MenuItemKind::ACTION && g_hoveredItem->children.empty()) {
                        if (CanEmit(g_hoveredItem->action)) g_pendingAction = g_hoveredItem->action;
                        if (g_hoveredItem->placementTool != PlacementTool::NONE) g_pendingPlacementTool = g_hoveredItem->placementTool;
                        CloseMenus();
                    }
                    mousePressConsumed = true;
                }
                else {
                    CloseMenus();
                    mousePressConsumed = true;
                }
            }
        }

        const EditorMenuAction shortcutAction = GetShortcutAction();
        if (CanEmit(shortcutAction)) {
            g_pendingAction = shortcutAction;
            CloseMenus();
            g_wantsKeyboardCapture = true;
        }

        g_wantsMouseCapture = menuBarRect.Contains(mousePosition) || menuWasOpen || g_openMenuIndex >= 0 || mousePressConsumed;
        g_wantsKeyboardCapture = g_wantsKeyboardCapture || g_openMenuIndex >= 0;

        if (g_wantsMouseCapture) {
            Hell::BackEnd::SetCursor(HELL_CURSOR_ARROW);
        }
    }

    void Render() {
        if (g_menus.empty()) return;

        for (size_t i = 0; i < g_menus.size(); i++) {
            const Menu& menu = g_menus[i];
            const bool highlighted = static_cast<int32_t>(i) == g_hoveredMenuIndex || static_cast<int32_t>(i) == g_openMenuIndex;
            if (highlighted) {
                UI::DrawSolidRect(menu.buttonRect, MENU_BUTTON_HOVER_COLOR);
            }

            UIBackEnd::BlitText(UICanvas::NATIVE, WithColor(Style::TEXT_COLOR_TAG, menu.label), Style::FONT_NAME, glm::ivec2(menu.buttonRect.x + MENU_BUTTON_HORIZONTAL_PADDING, menu.buttonRect.y + menu.buttonRect.height / 2), Alignment::CENTERED_VERTICAL, Style::FONT_SCALE, TextureFilter::NEAREST);
        }

        if (g_openMenuIndex < 0 || g_openMenuIndex >= static_cast<int32_t>(g_menus.size())) return;

        const Menu& menu = g_menus[g_openMenuIndex];
        RenderPopup(menu.items, menu.popupRect, 0);
    }

    void Close() {
        CloseMenus();
        g_hoveredMenuIndex = -1;
        g_wantsMouseCapture = false;
        g_wantsKeyboardCapture = false;
        g_pendingAction = EditorMenuAction::NONE;
        g_pendingPlacementTool = PlacementTool::NONE;
    }

    bool WantsMouseCapture() {
        return g_wantsMouseCapture;
    }

    bool WantsKeyboardCapture() {
        return g_wantsKeyboardCapture;
    }

    EditorMenuAction ConsumeAction() {
        const EditorMenuAction action = g_pendingAction;
        g_pendingAction = EditorMenuAction::NONE;
        return action;
    }

    PlacementTool ConsumePlacementTool() {
        const PlacementTool placementTool = g_pendingPlacementTool;
        g_pendingPlacementTool = PlacementTool::NONE;
        return placementTool;
    }
}
