#include "EditorInputElements.h"

#include "EditorCoordinates.h"
#include "EditorScrollBar.h"
#include "EditorStyle.h"
#include "EditorUI.h"

#include "Hell/Backend/BackEnd.h"
#include "Hell/Input.h"
#include "Hell/Time/Time.h"
#include "Hell/UI/TextBlitter.h"
#include "Hell/UI/UIBackEnd.h"
#include "Unloved/Common/Constants.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace Unloved::EditorSession::InputElements {
    void Begin(const EditorRect& rect, int32_t labelColumnWidth);
    bool String(uint64_t objectId, const std::string& label, std::string& value);
    bool CheckBox(const std::string& label, bool& value);
    bool DropDown(uint64_t objectId, const std::string& label, const std::vector<std::string>& options, std::string& value);
    bool Float(uint64_t objectId, const std::string& label, float& value);
    bool UInt(uint64_t objectId, const std::string& label, uint32_t& value);
    bool Vec2(uint64_t objectId, const std::string& label, glm::vec2& value);
    bool Vec3(uint64_t objectId, const std::string& label, glm::vec3& value);
    void End();

    namespace {
        constexpr int32_t ROW_HEIGHT = 26;
        constexpr int32_t CONTENT_PADDING = 10;
        constexpr int32_t LABEL_FIELD_GAP = 16;
        constexpr int32_t FIELD_PADDING = 6;
        constexpr int32_t FIELD_GAP = 4;
        constexpr int32_t CHECKBOX_SIZE = 16;
        constexpr int32_t DROP_DOWN_ARROW_SIZE = 8;
        constexpr int32_t DROP_DOWN_MAX_VISIBLE_OPTIONS = 8;
        constexpr int32_t DROP_DOWN_ROWS_PER_SCROLL = 3;
        constexpr int32_t DROP_DOWN_SCROLL_BAR_WIDTH = 8;
        constexpr float CARET_FLASH_TIME = 0.5f;

        const glm::vec4 FIELD_COLOR = glm::vec4(0.101961f, 0.090196f, 0.121569f, 1.0f);
        const glm::vec4 FIELD_HOVER_COLOR = glm::vec4(0.141176f, 0.125490f, 0.168627f, 1.0f);
        const glm::vec4 SELECTION_COLOR = glm::vec4(0.231373f, 0.196078f, 0.286275f, 1.0f);
        const glm::vec4 CARET_COLOR = glm::vec4(0.545098f, 0.541176f, 0.568627f, 1.0f);

        struct KeyCharacter {
            uint32_t keyCode = 0;
            char character = 0;
            char shiftedCharacter = 0;
        };

        EditorRect g_rect;
        uint64_t g_activeObjectId = 0;
        std::string g_activeLabel;
        std::string g_editValue;
        std::string g_originalValue;
        int32_t g_activeComponentIndex = -1;
        int32_t g_labelColumnWidth = 0;
        int32_t g_rowIndex = 0;
        size_t g_caretIndex = 0;
        size_t g_selectionAnchor = 0;
        float g_caretFlashTimer = 0.0f;
        bool g_activeInputWasDrawn = false;
        bool g_dragSelecting = false;
        uint64_t g_openDropDownObjectId = 0;
        std::string g_openDropDownLabel;
        std::string g_openDropDownValue;
        std::vector<std::string> g_openDropDownOptions;
        EditorRect g_openDropDownPopupRect;
        EditorScrollBar g_dropDownScrollBar;
        int32_t g_dropDownVisibleOptionCount = 0;
        int32_t g_hoveredDropDownOption = -1;
        bool g_openDropDownWasDrawn = false;
        bool g_dropDownConsumedMousePress = false;

        bool ShiftIsDown() {
            return Hell::Input::KeyDown(HELL_KEY_LEFT_SHIFT_GLFW) || Hell::Input::KeyDown(HELL_KEY_RIGHT_SHIFT);
        }

        bool ControlIsDown() {
            return Hell::Input::KeyDown(HELL_KEY_LEFT_CONTROL_GLFW) || Hell::Input::KeyDown(HELL_KEY_RIGHT_CONTROL);
        }

        char GetPressedCharacter() {
            const bool shiftDown = ShiftIsDown();

            for (uint32_t keyCode = HELL_KEY_A; keyCode <= HELL_KEY_Z; keyCode++) {
                if (Hell::Input::KeyPressed(keyCode)) return static_cast<char>((shiftDown ? 'A' : 'a') + keyCode - HELL_KEY_A);
            }

            constexpr char SHIFTED_NUMBER_CHARACTERS[] = { ')', '!', '@', '#', '$', '%', '^', '&', '*', '(' };
            for (uint32_t keyCode = HELL_KEY_0; keyCode <= HELL_KEY_9; keyCode++) {
                if (Hell::Input::KeyPressed(keyCode)) return shiftDown ? SHIFTED_NUMBER_CHARACTERS[keyCode - HELL_KEY_0] : static_cast<char>(keyCode);
            }

            constexpr KeyCharacter CHARACTERS[] = {
                { HELL_KEY_SPACE,         ' ', ' ' },
                { HELL_KEY_APOSTROPHE,    '\'', '"' },
                { HELL_KEY_COMMA,         ',', '<' },
                { HELL_KEY_MINUS,         '-', '_' },
                { HELL_KEY_PERIOD,        '.', '>' },
                { HELL_KEY_SLASH,         '/', '?' },
                { HELL_KEY_SEMICOLON,     ';', ':' },
                { HELL_KEY_EQUAL,         '=', '+' },
                { HELL_KEY_LEFT_BRACKET,  '[', '{' },
                { HELL_KEY_BACKSLASH,     '\\', '|' },
                { HELL_KEY_RIGHT_BRACKET, ']', '}' },
                { HELL_KEY_GRAVE_ACCENT,  '`', '~' },
            };

            for (const KeyCharacter& keyCharacter : CHARACTERS) {
                if (Hell::Input::KeyPressed(keyCharacter.keyCode)) return shiftDown ? keyCharacter.shiftedCharacter : keyCharacter.character;
            }
            return 0;
        }

        bool IsActive(uint64_t objectId, const std::string& label, int32_t componentIndex) {
            return g_activeObjectId == objectId && g_activeLabel == label && g_activeComponentIndex == componentIndex;
        }

        size_t GetSelectionBegin() {
            return std::min(g_caretIndex, g_selectionAnchor);
        }

        size_t GetSelectionEnd() {
            return std::max(g_caretIndex, g_selectionAnchor);
        }

        bool HasSelection() {
            return g_caretIndex != g_selectionAnchor;
        }

        int32_t GetCharacterWidth() {
            return TextBlitter::GetTextSize(" ", Style::FONT_NAME, Style::FONT_SCALE).x;
        }

        int32_t GetCharacterHeight() {
            const FontSpriteSheet* font = TextBlitter::GetFontSpriteSheet(Style::FONT_NAME);
            return font ? static_cast<int32_t>(std::round(static_cast<float>(font->m_charHeight) * Style::FONT_SCALE)) : 0;
        }

        size_t GetCharacterIndexAtMouse(const EditorRect& fieldRect, const std::string& value) {
            const int32_t characterWidth = GetCharacterWidth();
            const int32_t localMouseX = Coordinates::GetMousePositionUI().x - fieldRect.x - FIELD_PADDING;
            if (localMouseX <= 0 || characterWidth <= 0) return 0;

            const size_t index = static_cast<size_t>((localMouseX + characterWidth / 2) / characterWidth);
            return std::min(index, value.size());
        }

        bool DeleteSelection(std::string& value) {
            if (!HasSelection()) return false;

            const size_t selectionBegin = GetSelectionBegin();
            value.erase(selectionBegin, GetSelectionEnd() - selectionBegin);
            g_caretIndex = selectionBegin;
            g_selectionAnchor = selectionBegin;
            g_caretFlashTimer = 0.0f;
            return true;
        }

        void StopEditing() {
            g_activeObjectId = 0;
            g_activeLabel.clear();
            g_originalValue.clear();
            g_activeComponentIndex = -1;
            g_caretIndex = 0;
            g_selectionAnchor = 0;
            g_caretFlashTimer = 0.0f;
            g_dragSelecting = false;
        }

        void CloseDropDown() {
            g_openDropDownObjectId = 0;
            g_openDropDownLabel.clear();
            g_openDropDownValue.clear();
            g_openDropDownOptions.clear();
            g_dropDownScrollBar = {};
            g_dropDownVisibleOptionCount = 0;
            g_hoveredDropDownOption = -1;
        }

        bool IsDropDownOpen(uint64_t objectId, const std::string& label) {
            return g_openDropDownObjectId == objectId && g_openDropDownLabel == label;
        }

        bool UpdateString(std::string& value, bool numeric, bool unsignedInteger) {
            if (Hell::Input::KeyPressed(HELL_KEY_ESCAPE)) {
                const bool changed = value != g_originalValue;
                value = g_originalValue;
                StopEditing();
                return changed;
            }
            if (Hell::Input::KeyPressed(HELL_KEY_ENTER)) {
                StopEditing();
                return false;
            }

            if (ControlIsDown() && Hell::Input::KeyPressed(HELL_KEY_A)) {
                g_selectionAnchor = 0;
                g_caretIndex = value.size();
                g_caretFlashTimer = 0.0f;
                return false;
            }
            if (ControlIsDown()) return false;

            if (Hell::Input::KeyPressed(HELL_KEY_LEFT)) {
                if (!ShiftIsDown() && HasSelection()) g_caretIndex = GetSelectionBegin();
                else if (g_caretIndex > 0) g_caretIndex--;
                if (!ShiftIsDown()) g_selectionAnchor = g_caretIndex;
                g_caretFlashTimer = 0.0f;
                return false;
            }
            if (Hell::Input::KeyPressed(HELL_KEY_RIGHT)) {
                if (!ShiftIsDown() && HasSelection()) g_caretIndex = GetSelectionEnd();
                else if (g_caretIndex < value.size()) g_caretIndex++;
                if (!ShiftIsDown()) g_selectionAnchor = g_caretIndex;
                g_caretFlashTimer = 0.0f;
                return false;
            }
            if (Hell::Input::KeyPressed(HELL_KEY_HOME)) {
                g_caretIndex = 0;
                if (!ShiftIsDown()) g_selectionAnchor = g_caretIndex;
                g_caretFlashTimer = 0.0f;
                return false;
            }
            if (Hell::Input::KeyPressed(HELL_KEY_END)) {
                g_caretIndex = value.size();
                if (!ShiftIsDown()) g_selectionAnchor = g_caretIndex;
                g_caretFlashTimer = 0.0f;
                return false;
            }

            if (Hell::Input::KeyPressed(HELL_KEY_BACKSPACE)) {
                if (DeleteSelection(value)) return true;
                if (g_caretIndex == 0) return false;
                value.erase(g_caretIndex - 1, 1);
                g_caretIndex--;
                g_selectionAnchor = g_caretIndex;
                g_caretFlashTimer = 0.0f;
                return true;
            }
            if (Hell::Input::KeyPressed(HELL_KEY_DELETE)) {
                if (DeleteSelection(value)) return true;
                if (g_caretIndex >= value.size()) return false;
                value.erase(g_caretIndex, 1);
                g_caretFlashTimer = 0.0f;
                return true;
            }

            const char character = GetPressedCharacter();
            if (character == 0) return false;
            if (unsignedInteger && (character < '0' || character > '9')) return false;
            if (numeric && !unsignedInteger && character != '-' && character != '+' && character != '.' && character != 'e' && character != 'E' && (character < '0' || character > '9')) return false;

            DeleteSelection(value);
            value.insert(value.begin() + g_caretIndex, character);
            g_caretIndex++;
            g_selectionAnchor = g_caretIndex;
            g_caretFlashTimer = 0.0f;
            return true;
        }

        std::string FloatToString(float value) {
            char buffer[64];
            std::snprintf(buffer, sizeof(buffer), "%.3f", value);
            std::string text = buffer;
            while (!text.empty() && text.back() == '0') text.pop_back();
            if (!text.empty() && text.back() == '.') text.pop_back();
            if (text == "-0") text = "0";
            return text;
        }

        bool StringToFloat(const std::string& text, float& value) {
            if (text.empty()) return false;

            char* end = nullptr;
            const float parsedValue = std::strtof(text.c_str(), &end);
            if (end == text.c_str() || *end != '\0') return false;

            value = parsedValue;
            return true;
        }

        int32_t GetRowY() {
            return g_rect.y + CONTENT_PADDING + g_rowIndex * ROW_HEIGHT;
        }

        void DrawLabel(const std::string& label, int32_t rowY) {
            UIBackEnd::BlitText(UICanvas::NATIVE, std::string(Style::TEXT_COLOR_TAG) + label, Style::FONT_NAME, glm::ivec2(g_rect.x + CONTENT_PADDING, rowY + ROW_HEIGHT / 2), Alignment::CENTERED_VERTICAL, Style::FONT_SCALE, TextureFilter::NEAREST);
        }

        EditorRect GetFieldRect(int32_t rowY, int32_t componentIndex, int32_t componentCount) {
            const int32_t fieldX = g_rect.x + CONTENT_PADDING + g_labelColumnWidth + LABEL_FIELD_GAP;
            const int32_t totalWidth = std::max(0, g_rect.Right() - CONTENT_PADDING - fieldX);
            const int32_t componentWidth = std::max(0, totalWidth - FIELD_GAP * (componentCount - 1)) / componentCount;
            const int32_t componentX = fieldX + componentIndex * (componentWidth + FIELD_GAP);
            const int32_t componentRight = componentIndex == componentCount - 1 ? g_rect.Right() - CONTENT_PADDING : componentX + componentWidth;
            return { componentX, rowY + 2, std::max(0, componentRight - componentX), ROW_HEIGHT - 4 };
        }

        EditorRect GetDropDownOptionRect(int32_t visibleIndex) {
            const int32_t scrollBarWidth = g_dropDownScrollBar.visible ? DROP_DOWN_SCROLL_BAR_WIDTH : 0;
            return { g_openDropDownPopupRect.x, g_openDropDownPopupRect.y + visibleIndex * ROW_HEIGHT, g_openDropDownPopupRect.width - scrollBarWidth, ROW_HEIGHT };
        }

        void RenderOpenDropDown() {
            if (g_openDropDownObjectId == 0 || g_dropDownVisibleOptionCount <= 0) return;

            UI::DrawSolidRect(g_openDropDownPopupRect, FIELD_COLOR);

            for (int32_t i = 0; i < g_dropDownVisibleOptionCount; i++) {
                const int32_t optionIndex = g_dropDownScrollBar.value + i;
                if (optionIndex < 0 || optionIndex >= static_cast<int32_t>(g_openDropDownOptions.size())) break;

                const EditorRect optionRect = GetDropDownOptionRect(i);
                const std::string& option = g_openDropDownOptions[optionIndex];
                if (option == g_openDropDownValue) UI::DrawSolidRect(optionRect, SELECTION_COLOR);
                else if (optionIndex == g_hoveredDropDownOption) UI::DrawSolidRect(optionRect, FIELD_HOVER_COLOR);

                UIBackEnd::BlitText(UICanvas::NATIVE, std::string(Style::TEXT_COLOR_TAG) + option, Style::FONT_NAME, glm::ivec2(optionRect.x + FIELD_PADDING, optionRect.y + optionRect.height / 2), Alignment::CENTERED_VERTICAL, Style::FONT_SCALE, TextureFilter::NEAREST, optionRect.x, optionRect.y, optionRect.Right(), optionRect.Bottom());
            }

            ScrollBar::Render(g_dropDownScrollBar);
        }

        bool DrawTextField(uint64_t objectId, const std::string& label, int32_t componentIndex, const EditorRect& fieldRect, std::string& value, bool numeric, bool unsignedInteger = false) {
            const bool hovered = fieldRect.Contains(Coordinates::GetMousePositionUI());
            bool active = IsActive(objectId, label, componentIndex);

            if (Hell::Input::LeftMousePressed() && !g_dropDownConsumedMousePress) {
                if (hovered) {
                    const bool startingEdit = !active;
                    if (!active) {
                        g_activeObjectId = objectId;
                        g_activeLabel = label;
                        g_activeComponentIndex = componentIndex;
                        g_editValue = value;
                        g_originalValue = value;
                        active = true;
                    }

                    g_caretIndex = GetCharacterIndexAtMouse(fieldRect, g_editValue);
                    if (startingEdit || !ShiftIsDown()) g_selectionAnchor = g_caretIndex;
                    g_caretFlashTimer = 0.0f;
                    g_dragSelecting = true;
                }
                else if (active) {
                    StopEditing();
                    active = false;
                }
            }

            if (active && g_dragSelecting && Hell::Input::LeftMouseDown()) {
                g_caretIndex = GetCharacterIndexAtMouse(fieldRect, g_editValue);
                g_caretFlashTimer = 0.0f;
            }
            if (!Hell::Input::LeftMouseDown()) g_dragSelecting = false;

            bool changed = false;
            if (active) {
                g_caretIndex = std::min(g_caretIndex, g_editValue.size());
                g_selectionAnchor = std::min(g_selectionAnchor, g_editValue.size());
                g_activeInputWasDrawn = true;
                changed = UpdateString(g_editValue, numeric, unsignedInteger);
                active = IsActive(objectId, label, componentIndex);
                if (changed) value = g_editValue;
                if (active) g_caretFlashTimer += Hell::Time::RawDeltaTime();
            }

            if (hovered) Hell::BackEnd::SetCursor(HELL_CURSOR_IBEAM);

            UI::DrawSolidRect(fieldRect, hovered || active ? FIELD_HOVER_COLOR : FIELD_COLOR);

            const std::string& displayedValue = active ? g_editValue : value;
            const int32_t characterWidth = GetCharacterWidth();
            const int32_t textX = fieldRect.x + FIELD_PADDING;
            if (active && HasSelection()) {
                const int32_t selectionX = textX + static_cast<int32_t>(GetSelectionBegin()) * characterWidth;
                const int32_t selectionWidth = static_cast<int32_t>(GetSelectionEnd() - GetSelectionBegin()) * characterWidth;
                const int32_t clippedSelectionX = std::max(selectionX, fieldRect.x);
                const int32_t clippedSelectionRight = std::min(selectionX + selectionWidth, fieldRect.Right());
                if (clippedSelectionRight > clippedSelectionX) UI::DrawSolidRect({ clippedSelectionX, fieldRect.y + 1, clippedSelectionRight - clippedSelectionX, fieldRect.height - 2 }, SELECTION_COLOR);
            }

            UIBackEnd::BlitText(UICanvas::NATIVE, std::string(Style::TEXT_COLOR_TAG) + displayedValue, Style::FONT_NAME, glm::ivec2(textX, fieldRect.y + fieldRect.height / 2), Alignment::CENTERED_VERTICAL, Style::FONT_SCALE, TextureFilter::NEAREST, fieldRect.x, fieldRect.y, fieldRect.Right(), fieldRect.Bottom());
            if (active && std::fmod(g_caretFlashTimer, CARET_FLASH_TIME * 2.0f) < CARET_FLASH_TIME) {
                const int32_t caretX = textX + static_cast<int32_t>(g_caretIndex) * characterWidth;
                const int32_t caretHeight = GetCharacterHeight();
                if (caretX >= fieldRect.x && caretX < fieldRect.Right()) UI::DrawSolidRect({ caretX, fieldRect.y + (fieldRect.height - caretHeight) / 2, 1, caretHeight }, CARET_COLOR);
            }
            return changed;
        }

        bool DrawFloatField(uint64_t objectId, const std::string& label, int32_t componentIndex, const EditorRect& fieldRect, float& value) {
            std::string text = FloatToString(value);
            if (!DrawTextField(objectId, label, componentIndex, fieldRect, text, true)) return false;

            float parsedValue = 0.0f;
            if (!StringToFloat(text, parsedValue)) return false;

            value = parsedValue;
            return true;
        }

        bool DrawUIntField(uint64_t objectId, const std::string& label, const EditorRect& fieldRect, uint32_t& value) {
            std::string text = std::to_string(value);
            if (!DrawTextField(objectId, label, 0, fieldRect, text, true, true)) return false;
            if (text.empty()) return false;

            char* end = nullptr;
            const unsigned long long parsedValue = std::strtoull(text.c_str(), &end, 10);
            if (end == text.c_str() || *end != '\0' || parsedValue > UINT32_MAX) return false;

            value = static_cast<uint32_t>(parsedValue);
            return true;
        }
    }

    PropertyList::PropertyList() {
        m_elements.reserve(16);
    }

    void PropertyList::String(uint64_t objectId, const char* label, std::string& value, std::function<void()> onChange) {
        Element& element = m_elements.emplace_back();
        element.label = label;
        element.render = [objectId, value = &value, onChange](const std::string& label) {
            if (InputElements::String(objectId, label, *value) && onChange) onChange();
        };
    }

    void PropertyList::CheckBox(const char* label, bool& value, std::function<void()> onChange) {
        Element& element = m_elements.emplace_back();
        element.label = label;
        element.render = [value = &value, onChange](const std::string& label) {
            if (InputElements::CheckBox(label, *value) && onChange) onChange();
        };
    }

    void PropertyList::DropDown(uint64_t objectId, const char* label, const std::vector<std::string>& options, std::string& value, std::function<void()> onChange) {
        Element& element = m_elements.emplace_back();
        element.label = label;
        element.render = [objectId, options = &options, value = &value, onChange](const std::string& label) {
            if (InputElements::DropDown(objectId, label, *options, *value) && onChange) onChange();
        };
    }

    void PropertyList::Float(uint64_t objectId, const char* label, float& value, std::function<void()> onChange) {
        Element& element = m_elements.emplace_back();
        element.label = label;
        element.render = [objectId, value = &value, onChange](const std::string& label) {
            if (InputElements::Float(objectId, label, *value) && onChange) onChange();
        };
    }

    void PropertyList::UInt(uint64_t objectId, const char* label, uint32_t& value, std::function<void()> onChange) {
        Element& element = m_elements.emplace_back();
        element.label = label;
        element.render = [objectId, value = &value, onChange](const std::string& label) {
            if (InputElements::UInt(objectId, label, *value) && onChange) onChange();
        };
    }

    void PropertyList::Vec2(uint64_t objectId, const char* label, glm::vec2& value, std::function<void()> onChange) {
        Element& element = m_elements.emplace_back();
        element.label = label;
        element.render = [objectId, value = &value, onChange](const std::string& label) {
            if (InputElements::Vec2(objectId, label, *value) && onChange) onChange();
        };
    }

    void PropertyList::Vec3(uint64_t objectId, const char* label, glm::vec3& value, std::function<void()> onChange) {
        Element& element = m_elements.emplace_back();
        element.label = label;
        element.render = [objectId, value = &value, onChange](const std::string& label) {
            if (InputElements::Vec3(objectId, label, *value) && onChange) onChange();
        };
    }

    void PropertyList::Render(const EditorRect& rect) {
        int32_t labelColumnWidth = 0;
        for (const Element& element : m_elements) {
            labelColumnWidth = std::max(labelColumnWidth, TextBlitter::GetTextSize(element.label, Style::FONT_NAME, Style::FONT_SCALE).x);
        }

        Begin(rect, labelColumnWidth);
        for (Element& element : m_elements) {
            element.render(element.label);
        }
        End();
        m_elements.clear();
    }

    void Begin(const EditorRect& rect, int32_t labelColumnWidth) {
        g_rect = rect;
        g_labelColumnWidth = labelColumnWidth;
        g_rowIndex = 0;
        g_activeInputWasDrawn = false;
        g_openDropDownWasDrawn = false;
        g_dropDownConsumedMousePress = false;
    }

    bool String(uint64_t objectId, const std::string& label, std::string& value) {
        const int32_t rowY = GetRowY();
        DrawLabel(label, rowY);
        const bool changed = DrawTextField(objectId, label, 0, GetFieldRect(rowY, 0, 1), value, false);
        g_rowIndex++;
        return changed;
    }

    bool CheckBox(const std::string& label, bool& value) {
        const int32_t rowY = GetRowY();
        const EditorRect fieldRect = GetFieldRect(rowY, 0, 1);
        const EditorRect checkBoxRect = { fieldRect.x, rowY + (ROW_HEIGHT - CHECKBOX_SIZE) / 2, CHECKBOX_SIZE, CHECKBOX_SIZE };
        const bool hovered = checkBoxRect.Contains(Coordinates::GetMousePositionUI());
        const bool changed = hovered && Hell::Input::LeftMousePressed() && !g_dropDownConsumedMousePress;

        if (changed) value = !value;
        DrawLabel(label, rowY);
        UI::DrawSolidRect(checkBoxRect, hovered ? FIELD_HOVER_COLOR : FIELD_COLOR);
        if (value) UIBackEnd::BlitText(UICanvas::NATIVE, std::string(Style::TEXT_COLOR_TAG) + "x", Style::FONT_NAME, glm::ivec2(checkBoxRect.x + checkBoxRect.width / 2, checkBoxRect.y + checkBoxRect.height / 2), Alignment::CENTERED, Style::FONT_SCALE, TextureFilter::NEAREST);

        g_rowIndex++;
        return changed;
    }

    bool DropDown(uint64_t objectId, const std::string& label, const std::vector<std::string>& options, std::string& value) {
        const int32_t rowY = GetRowY();
        const EditorRect fieldRect = GetFieldRect(rowY, 0, 1);
        const glm::ivec2 mousePosition = Coordinates::GetMousePositionUI();
        const bool hovered = fieldRect.Contains(mousePosition);
        const bool fieldPressed = hovered && Hell::Input::LeftMousePressed() && !g_dropDownConsumedMousePress;
        bool changed = false;

        if (fieldPressed) {
            if (IsDropDownOpen(objectId, label)) {
                CloseDropDown();
            }
            else if (!options.empty()) {
                StopEditing();
                CloseDropDown();
                g_openDropDownObjectId = objectId;
                g_openDropDownLabel = label;
                g_dropDownScrollBar = {};
            }
        }

        if (IsDropDownOpen(objectId, label)) {
            g_openDropDownValue = value;
            g_openDropDownOptions = options;

            const int32_t availableRows = std::max(1, (g_rect.Bottom() - fieldRect.Bottom()) / ROW_HEIGHT);
            g_dropDownVisibleOptionCount = std::min({ DROP_DOWN_MAX_VISIBLE_OPTIONS, static_cast<int32_t>(options.size()), availableRows });
            g_openDropDownPopupRect = { fieldRect.x, fieldRect.Bottom(), fieldRect.width, g_dropDownVisibleOptionCount * ROW_HEIGHT };

            if (g_openDropDownPopupRect.Contains(mousePosition)) {
                if (Hell::Input::MouseWheelUp()) g_dropDownScrollBar.value -= DROP_DOWN_ROWS_PER_SCROLL;
                else if (Hell::Input::MouseWheelDown()) g_dropDownScrollBar.value += DROP_DOWN_ROWS_PER_SCROLL;
            }

            const EditorRect scrollBarRect = { g_openDropDownPopupRect.Right() - DROP_DOWN_SCROLL_BAR_WIDTH, g_openDropDownPopupRect.y, DROP_DOWN_SCROLL_BAR_WIDTH, g_openDropDownPopupRect.height };
            ScrollBar::Update(g_dropDownScrollBar, scrollBarRect, static_cast<int32_t>(options.size()), g_dropDownVisibleOptionCount, true);

            g_hoveredDropDownOption = -1;
            for (int32_t i = 0; i < g_dropDownVisibleOptionCount; i++) {
                if (GetDropDownOptionRect(i).Contains(mousePosition)) g_hoveredDropDownOption = g_dropDownScrollBar.value + i;
            }

            if (!fieldPressed && Hell::Input::LeftMousePressed() && g_openDropDownPopupRect.Contains(mousePosition)) {
                g_dropDownConsumedMousePress = true;
                if (!ScrollBar::WantsMouseCapture(g_dropDownScrollBar) && g_hoveredDropDownOption >= 0 && g_hoveredDropDownOption < static_cast<int32_t>(options.size())) {
                    value = options[g_hoveredDropDownOption];
                    changed = true;
                    CloseDropDown();
                }
            }
            else if (!fieldPressed && Hell::Input::LeftMousePressed()) {
                CloseDropDown();
            }

            if (IsDropDownOpen(objectId, label) && Hell::Input::KeyPressed(HELL_KEY_ESCAPE)) CloseDropDown();
            if (IsDropDownOpen(objectId, label)) g_openDropDownWasDrawn = true;
        }

        if (hovered) Hell::BackEnd::SetCursor(HELL_CURSOR_ARROW);

        DrawLabel(label, rowY);
        UI::DrawSolidRect(fieldRect, hovered || IsDropDownOpen(objectId, label) ? FIELD_HOVER_COLOR : FIELD_COLOR);
        const int32_t textRight = std::max(fieldRect.x, fieldRect.Right() - FIELD_PADDING * 2 - DROP_DOWN_ARROW_SIZE);
        UIBackEnd::BlitText(UICanvas::NATIVE, std::string(Style::TEXT_COLOR_TAG) + value, Style::FONT_NAME, glm::ivec2(fieldRect.x + FIELD_PADDING, fieldRect.y + fieldRect.height / 2), Alignment::CENTERED_VERTICAL, Style::FONT_SCALE, TextureFilter::NEAREST, fieldRect.x, fieldRect.y, textRight, fieldRect.Bottom());
        UIBackEnd::BlitTexture(UICanvas::NATIVE, "DropDownArrow", glm::ivec2(fieldRect.Right() - FIELD_PADDING - DROP_DOWN_ARROW_SIZE / 2, fieldRect.y + fieldRect.height / 2), Alignment::CENTERED, CARET_COLOR, glm::ivec2(DROP_DOWN_ARROW_SIZE), TextureFilter::NEAREST);

        g_rowIndex++;
        return changed;
    }

    bool Float(uint64_t objectId, const std::string& label, float& value) {
        const int32_t rowY = GetRowY();
        DrawLabel(label, rowY);
        const bool changed = DrawFloatField(objectId, label, 0, GetFieldRect(rowY, 0, 1), value);
        g_rowIndex++;
        return changed;
    }

    bool UInt(uint64_t objectId, const std::string& label, uint32_t& value) {
        const int32_t rowY = GetRowY();
        DrawLabel(label, rowY);
        const bool changed = DrawUIntField(objectId, label, GetFieldRect(rowY, 0, 1), value);
        g_rowIndex++;
        return changed;
    }

    bool Vec2(uint64_t objectId, const std::string& label, glm::vec2& value) {
        const int32_t rowY = GetRowY();
        DrawLabel(label, rowY);

        bool changed = false;
        for (int32_t i = 0; i < 2; i++) {
            changed = DrawFloatField(objectId, label, i, GetFieldRect(rowY, i, 2), value[i]) || changed;
        }

        g_rowIndex++;
        return changed;
    }

    bool Vec3(uint64_t objectId, const std::string& label, glm::vec3& value) {
        const int32_t rowY = GetRowY();
        DrawLabel(label, rowY);

        bool changed = false;
        for (int32_t i = 0; i < 3; i++) {
            changed = DrawFloatField(objectId, label, i, GetFieldRect(rowY, i, 3), value[i]) || changed;
        }

        g_rowIndex++;
        return changed;
    }

    void End() {
        if (g_activeObjectId != 0 && !g_activeInputWasDrawn) StopEditing();
        if (g_openDropDownObjectId != 0 && !g_openDropDownWasDrawn) CloseDropDown();
        else RenderOpenDropDown();
    }

    void Reset() {
        StopEditing();
        CloseDropDown();
    }

    bool WantsKeyboardCapture() {
        return g_activeObjectId != 0 || g_openDropDownObjectId != 0;
    }
}
