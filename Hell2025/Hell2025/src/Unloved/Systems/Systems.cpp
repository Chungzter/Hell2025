#include "Systems.h"

#include "Hell/Time.h"

#include "Unloved/Systems/Blood/BloodSystem.h"
#include "Unloved/Systems/Bullets/BulletSystem.h"
#include "Unloved/Systems/GameAudio/GameAudio.h"
#include "Unloved/Systems/Mirrors/MirrorManager.h"
#include "Unloved/Systems/NavMesh/NavMesh.h"
#include "Unloved/Systems/Openables/OpenableManager.h"
#include "Unloved/Systems/PianoPlayback/PianoPlaybackManager.h"

namespace Unloved::Systems {
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
        BloodSystem::CleanUp();
        BulletSystem::CleanUp();
        PianoPlaybackManager::CleanUp();
    }
}
