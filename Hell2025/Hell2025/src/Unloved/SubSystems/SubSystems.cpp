#pragma once
#include "Unloved/SubSystems/GameAudio.h"
#include "Unloved/SubSystems/PianoPlaybackManager.h"

namespace Unloved::SubSystems {
    void Init() {
        PianoPlaybackManager::Init();
    }

    void BeginFrame() {
        GameAudio::BeginFrame();
    }

    void Update() {
        PianoPlaybackManager::Update();
    }

    void UpdatePostSession() {
        GameAudio::Update();
    }

    void CleanUp() {
        PianoPlaybackManager::CleanUp();
    }
}
