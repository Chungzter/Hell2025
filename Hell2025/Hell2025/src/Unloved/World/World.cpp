#include "World.h"

#include "Legacy/World/LegacyWorld.h"
#include "Unloved/World/Objects/Interior/Piano/PianoPlaybackManager.h"

namespace Unloved::World {

    void Init() {
        LegacyWorld::Init();
        PianoPlaybackManager::Init();
    }

    void BeginFrame() {
        LegacyWorld::BeginFrame();
    }

    void Update() {
        PianoPlaybackManager::Update();
    }

    void SubmitRenderItems() {
        LegacyWorld::SubmitRenderItems();
    }

    void EndFrame() {
        LegacyWorld::EndFrame();
    }

    void CleanUp() {
        PianoPlaybackManager::CleanUp();
    }
}
