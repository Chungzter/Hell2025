#pragma once

#include "EditorSessionTypes.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Unloved::EditorSession::InputElements {

    struct PropertyList {
        PropertyList();

        void String(uint64_t objectId, const char* label, std::string& value, std::function<void()> onChange = {});
        void CheckBox(const char* label, bool& value, std::function<void()> onChange = {});
        void DropDown(uint64_t objectId, const char* label, const std::vector<std::string>& options, std::string& value, std::function<void()> onChange = {});
        void Float(uint64_t objectId, const char* label, float& value, std::function<void()> onChange = {});
        void UInt(uint64_t objectId, const char* label, uint32_t& value, std::function<void()> onChange = {});
        void Vec2(uint64_t objectId, const char* label, glm::vec2& value, std::function<void()> onChange = {});
        void Vec3(uint64_t objectId, const char* label, glm::vec3& value, std::function<void()> onChange = {});
        void Render(const EditorRect& rect);

    private:
        struct Element {
            std::string label;
            std::function<void(const std::string&)> render;
        };

        std::vector<Element> m_elements;
    };

    void Reset();

    bool WantsKeyboardCapture();
}
