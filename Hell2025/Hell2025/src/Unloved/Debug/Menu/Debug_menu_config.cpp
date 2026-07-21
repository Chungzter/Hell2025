#include "Debug_menu.h"

#include "Unloved/Config/FlashlightConfig.h"
#include "Unloved/Debug/Debug.h"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <string>
#include <vector>

namespace Debug::Menu::ConfigMenu {

    enum struct RootSetting : uint32_t {
        FLASHLIGHT,
    };

    enum struct FlashlightSetting : uint32_t {
        IES_PROFILE,
        IES_ENABLED,
        RANGE,
        FALLOFF_EXPONENT,
        BRIGHTNESS,
        COLOR_R,
        COLOR_G,
        COLOR_B,
        IES_CONE_SCALE,
        IES_INNER_ANGLE,
        IES_OUTER_ANGLE,
        IES_CONTRAST,
        CENTER_SPOT_ENABLED,
        CENTER_SPOT_RANGE,
        CENTER_SPOT_FALLOFF_EXPONENT,
        CENTER_SPOT_BRIGHTNESS,
        CENTER_SPOT_INNER_ANGLE,
        CENTER_SPOT_OUTER_ANGLE,
        SAVE_TO_DISK,
        LOAD_FROM_DISK,
    };

    PageId g_homePage = ROOT_PAGE_ID;
    PageId g_flashlightPage = ROOT_PAGE_ID;

    void BuildMainMenu();
    void BuildFlashlightMenu();
    void ApplyFlashlightEdit(uint32_t id, const Value& value);

    std::vector<std::string> GetIESProfileNames() {
        std::vector<std::string> names = { "ThreeJS_0", "ThreeJS_1", "ThreeJS_2", "ThreeJS_3" };
        const std::string& currentName = Config::Flashlight::GetSettings().iesProfile;
        if (std::find(names.begin(), names.end(), currentName) == names.end()) names.push_back(currentName);
        return names;
    }

    int32_t GetIESProfileIndex(const std::vector<std::string>& names, const std::string& profileName) {
        const auto it = std::find(names.begin(), names.end(), profileName);
        return it == names.end() ? 0 : static_cast<int32_t>(std::distance(names.begin(), it));
    }

    void RegisterMenu() {
        g_homePage = RegisterRootPage("Config", "CONFIG", BuildMainMenu, nullptr);
        g_flashlightPage = RegisterPage("FLASHLIGHT", g_homePage, BuildFlashlightMenu, ApplyFlashlightEdit);
    }

    Registrar g_registrar(RegisterMenu);

    void BuildMainMenu() {
        AddSubMenu(static_cast<uint32_t>(RootSetting::FLASHLIGHT), "Flashlight", g_flashlightPage);
    }

    void BuildFlashlightMenu() {
        const Config::Flashlight::Settings& settings = Config::Flashlight::GetSettings();
        const std::vector<std::string> profileNames = GetIESProfileNames();

        AddEnum(static_cast<uint32_t>(FlashlightSetting::IES_PROFILE), "IES Profile", GetIESProfileIndex(profileNames, settings.iesProfile), profileNames);
        AddBool(static_cast<uint32_t>(FlashlightSetting::IES_ENABLED), "IES Enabled", settings.iesEnabled);
        AddFloat(static_cast<uint32_t>(FlashlightSetting::RANGE), "Range", settings.range, 1.0f, 100.0f, 0.1f, 2);
        AddFloat(static_cast<uint32_t>(FlashlightSetting::FALLOFF_EXPONENT), "Falloff Exponent", settings.falloffExponent, 0.01f, 8.0f, 0.01f, 2);
        AddFloat(static_cast<uint32_t>(FlashlightSetting::BRIGHTNESS), "Brightness", settings.brightness, 0.0f, 10.0f, 0.01f, 2);
        AddFloat(static_cast<uint32_t>(FlashlightSetting::COLOR_R), "Color R", settings.color.r, 0.0f, 1.0f, 0.01f, 3);
        AddFloat(static_cast<uint32_t>(FlashlightSetting::COLOR_G), "Color G", settings.color.g, 0.0f, 1.0f, 0.01f, 3);
        AddFloat(static_cast<uint32_t>(FlashlightSetting::COLOR_B), "Color B", settings.color.b, 0.0f, 1.0f, 0.01f, 3);
        AddFloat(static_cast<uint32_t>(FlashlightSetting::IES_CONE_SCALE), "IES Cone Scale", settings.iesConeScale, 0.1f, 1.2f, 0.01f, 2);
        AddFloat(static_cast<uint32_t>(FlashlightSetting::IES_INNER_ANGLE), "IES Inner Angle", settings.iesInnerAngle, 0.0f, 89.0f, 0.01f, 2);
        AddFloat(static_cast<uint32_t>(FlashlightSetting::IES_OUTER_ANGLE), "IES Outer Angle", settings.iesOuterAngle, 0.0f, 89.0f, 0.01f, 2);
        AddFloat(static_cast<uint32_t>(FlashlightSetting::IES_CONTRAST), "IES Contrast", settings.iesContrast, 0.1f, 8.0f, 0.01f, 2);

        AddLineBreak();
        AddBool(static_cast<uint32_t>(FlashlightSetting::CENTER_SPOT_ENABLED), "Center Spot Enabled", settings.centerSpotEnabled);
        AddFloat(static_cast<uint32_t>(FlashlightSetting::CENTER_SPOT_RANGE), "Center Spot Range", settings.centerSpotRange, 0.1f, 100.0f, 0.1f, 2);
        AddFloat(static_cast<uint32_t>(FlashlightSetting::CENTER_SPOT_FALLOFF_EXPONENT), "Center Spot Falloff Exponent", settings.centerSpotFalloffExponent, 0.01f, 8.0f, 0.01f, 2);
        AddFloat(static_cast<uint32_t>(FlashlightSetting::CENTER_SPOT_BRIGHTNESS), "Center Spot Brightness", settings.centerSpotBrightness, 0.0f, 10.0f, 0.01f, 2);
        AddFloat(static_cast<uint32_t>(FlashlightSetting::CENTER_SPOT_INNER_ANGLE), "Center Spot Inner Angle", settings.centerSpotInnerAngle, 0.0f, 89.0f, 0.01f, 2);
        AddFloat(static_cast<uint32_t>(FlashlightSetting::CENTER_SPOT_OUTER_ANGLE), "Center Spot Outer Angle", settings.centerSpotOuterAngle, 0.0f, 89.0f, 0.01f, 2);

        AddLineBreak();
        AddAction(static_cast<uint32_t>(FlashlightSetting::SAVE_TO_DISK), "Save to disk");
        AddAction(static_cast<uint32_t>(FlashlightSetting::LOAD_FROM_DISK), "Load from disk");
    }

    void ApplyFlashlightEdit(uint32_t id, const Value& value) {
        const FlashlightSetting setting = static_cast<FlashlightSetting>(id);
        if (setting == FlashlightSetting::SAVE_TO_DISK) {
            const bool saved = Config::Flashlight::SaveToDisk();
            Debug::BlitQuickDebugMessage(saved ? "Saved " + Config::Flashlight::GetFilePath() : "Failed to save flashlight config");
            return;
        }
        if (setting == FlashlightSetting::LOAD_FROM_DISK) {
            const bool loaded = Config::Flashlight::LoadFromDisk();
            Debug::BlitQuickDebugMessage(loaded ? "Loaded " + Config::Flashlight::GetFilePath() : "Failed to load flashlight config");
            return;
        }

        Config::Flashlight::Settings settings = Config::Flashlight::GetSettings();
        switch (setting) {
            case FlashlightSetting::IES_PROFILE: {
                const std::vector<std::string> profileNames = GetIESProfileNames();
                if (value.intValue < 0 || value.intValue >= static_cast<int32_t>(profileNames.size())) return;
                settings.iesProfile = profileNames[value.intValue];
                break;
            }
            case FlashlightSetting::IES_ENABLED:        settings.iesEnabled = value.boolValue;       break;
            case FlashlightSetting::RANGE:              settings.range = value.floatValue;           break;
            case FlashlightSetting::FALLOFF_EXPONENT:   settings.falloffExponent = value.floatValue; break;
            case FlashlightSetting::BRIGHTNESS:         settings.brightness = value.floatValue;      break;
            case FlashlightSetting::COLOR_R:            settings.color.r = value.floatValue;         break;
            case FlashlightSetting::COLOR_G:            settings.color.g = value.floatValue;         break;
            case FlashlightSetting::COLOR_B:            settings.color.b = value.floatValue;         break;
            case FlashlightSetting::IES_CONE_SCALE:     settings.iesConeScale = value.floatValue;    break;
            case FlashlightSetting::IES_INNER_ANGLE:    settings.iesInnerAngle = value.floatValue;   break;
            case FlashlightSetting::IES_OUTER_ANGLE:    settings.iesOuterAngle = value.floatValue;   break;
            case FlashlightSetting::IES_CONTRAST:       settings.iesContrast = value.floatValue;     break;
            case FlashlightSetting::CENTER_SPOT_ENABLED:          settings.centerSpotEnabled = value.boolValue;                  break;
            case FlashlightSetting::CENTER_SPOT_RANGE:            settings.centerSpotRange = value.floatValue;                   break;
            case FlashlightSetting::CENTER_SPOT_FALLOFF_EXPONENT: settings.centerSpotFalloffExponent = value.floatValue;         break;
            case FlashlightSetting::CENTER_SPOT_BRIGHTNESS:       settings.centerSpotBrightness = value.floatValue;              break;
            case FlashlightSetting::CENTER_SPOT_INNER_ANGLE:      settings.centerSpotInnerAngle = value.floatValue;              break;
            case FlashlightSetting::CENTER_SPOT_OUTER_ANGLE:      settings.centerSpotOuterAngle = value.floatValue;              break;
            default: return;
        }

        Config::Flashlight::SetSettings(settings);
    }
}
