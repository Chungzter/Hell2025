#include "World.h"

#include "Legacy/World/LegacyWorld.h"

namespace Unloved::World {

    void Init() {
        LegacyWorld::Init();
    }

    void BeginFrame() {
        LegacyWorld::BeginFrame();
    }

    void Update() {
        UpdateEnvironment();
    }

    void SubmitRenderItems() {
        LegacyWorld::SubmitRenderItems();
    }

    void EndFrame() {
        LegacyWorld::EndFrame();
    }

    void CleanUp() {
        CleanUpAll();
    }
}
