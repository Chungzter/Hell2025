#pragma once
#include "Hell/Time.h"

#include "Unloved/SubSystems/GameAudio/GameAudio.h"
#include "Unloved/SubSystems/Mirrors/MirrorManager.h"
#include "Unloved/SubSystems/NavMesh/NavMesh.h"
#include "Unloved/SubSystems/Openables/OpenableManager.h"
#include "Unloved/SubSystems/PianoPlayback/PianoPlaybackManager.h"

namespace Unloved::SubSystems {
    void Init() {
        Unloved::NavMeshManager::Init();
        PianoPlaybackManager::Init();
    }

    void BeginFrame() {
        GameAudio::BeginFrame();
    }

    void PreWorldUpdate() {
        Unloved::OpenableManager::Update(Hell::Time::DeltaTime());
        Unloved::NavMeshManager::Update();
        PianoPlaybackManager::Update();
        GameAudio::Update();
    }

    void PostWorldUpdate() {
        Unloved::MirrorManager::Update();
    }

    void CleanUp() {
        PianoPlaybackManager::CleanUp();
    }
}
