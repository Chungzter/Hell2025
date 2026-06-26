#include "Session.h"

#include "Hell/Logging.h"

#include <algorithm>

namespace Unloved::Session {

    float g_sessionTime = 0;
    SplitscreenMode g_splitscreenMode = SplitscreenMode::FULLSCREEN;

    void Create() {
        AddLocalPlayer(glm::vec3(12.82, 0.5f, 18.27f), glm::vec3(-0.13f, -1.46f, 0.0f));
        AddLocalPlayer(glm::vec3(15.21, 0.5f, 19.57), glm::vec3(-0.49f, -0.74f, 0.0f));

        SetPlayerKeyboardAndMouseIndex(0, 0, 0);
        SetPlayerKeyboardAndMouseIndex(1, 1, 1);

        SetSplitscreenMode(SplitscreenMode::FULLSCREEN);
    }

    float GetSessionTime() {
        return g_sessionTime;
    }

    void NextSplitScreenMode() {
        int nextSplitscreenMode = ((int)(g_splitscreenMode) + 1) % ((int)(SplitscreenMode::SPLITSCREEN_MODE_COUNT));
        SetSplitscreenMode((SplitscreenMode)nextSplitscreenMode);

        if (nextSplitscreenMode >= 2) {
            NextSplitScreenMode();
        }
    }

    void SetSplitscreenMode(SplitscreenMode mode) {
        g_splitscreenMode = mode;
    }

    const SplitscreenMode& GetSplitscreenMode() {
        return g_splitscreenMode;
    }

    int32_t GetActiveViewportCount() {
        switch (g_splitscreenMode) {
            case SplitscreenMode::FULLSCREEN:  return std::min(GetLocalPlayerCount(), 1);
            case SplitscreenMode::TWO_PLAYER:  return std::min(GetLocalPlayerCount(), 2);
            case SplitscreenMode::FOUR_PLAYER: return std::min(GetLocalPlayerCount(), 4);
            default: return 1;
        }
    }
}
