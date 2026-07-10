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

    void EndFrame() {
        LegacyWorld::EndFrame();
    }

    void CleanUp() {
        CleanUpAll();
    }

    void ResetWorld() {
        LegacyWorld::ResetWorld();
    }

    void ClearAllObjects() {
        LegacyWorld::ClearAllObjects();
    }
}
