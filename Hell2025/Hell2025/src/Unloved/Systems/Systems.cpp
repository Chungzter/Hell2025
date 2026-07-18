#include "Systems.h"

#include "Hell/Time.h"

#include "Unloved/Systems/Blood/BloodSystem.h"
#include "Unloved/Systems/BloodOLD/BloodSystemOLD.h"
#include "Unloved/Systems/Bullets/BulletSystem.h"
#include "Unloved/Systems/DirtyTracker/DirtyTracker.h"
#include "Unloved/Systems/FeatureTest/FeatureTest.h"
#include "Unloved/Systems/GameAudio/GameAudio.h"
#include "Unloved/Systems/Mirrors/MirrorManager.h"
#include "Unloved/Systems/NavMesh/NavMesh.h"
#include "Unloved/Systems/Openables/OpenableManager.h"
#include "Unloved/Systems/PianoPlayback/PianoPlaybackManager.h"
#include "Unloved/Systems/ShadowMaps/ShadowMapManager.h"

namespace Unloved::Systems {
    void Init() {
        NavMeshManager::Init();
        PianoPlaybackManager::Init();
    }

    void BeginFrame() {
        BloodSystem::BeginFrame();
        DirtyTracker::BeginFrame();
        GameAudio::BeginFrame();
        ShadowMapManager::BeginFrame();
    }

    void PreWorldUpdate() {
        OpenableManager::Update(Hell::Time::DeltaTime());
        NavMeshManager::Update();
        PianoPlaybackManager::Update();
        GameAudio::Update();
        FeatureTest::Update();
        BloodSystemOLD::Update(Hell::Time::DeltaTime());
    }

    void PostWorldUpdate() {
        BloodSystem::Update();
        DirtyTracker::Update();
        MirrorManager::Update();
        ShadowMapManager::Update();
    }

    void CleanUp() {
        BloodSystemOLD::CleanUp();
        BulletSystem::CleanUp();
        PianoPlaybackManager::CleanUp();
    }
}
