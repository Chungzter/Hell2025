#include "Unloved.h"

#include "Legacy/Renderer/Renderer.h"
#include "Legacy/Renderer/RenderDataManager.h"

#include "Hell/Physics/Physics.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/UI/UIBackEnd.h"
#include "Hell/Time.h"

#include "Unloved/Bible/Bible.h"
#include "Unloved/Config/Config.h"
#include "Unloved/Debug/Debug.h"
#include "Unloved/Debug/DebugDraw.h"
#include "Unloved/Editor/Editor.h"
#include "Unloved/Editor/Gizmo.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Systems/Bullets/BulletSystem.h"
#include "Unloved/Systems/GameAudio/GameAudio.h"
#include "Unloved/Systems/House/HouseManager.h"
#include "Unloved/Systems/Map/MapManager.h"
#include "Unloved/Systems/Pathfinding/AStarMap.h"
#include "Unloved/Systems/Systems.h"
#include "Unloved/UI/Imgui/ImguiBackEnd.h"
#include "Unloved/Viewport/ViewportManager.h"
#include "Unloved/World/World.h"

namespace Time = Hell::Time;


namespace Unloved {
    bool Init() {
        Renderer::Init();
        const Resolutions& resolutions = Config::GetResolutions();
        UIBackEnd::SetUIResolution(resolutions.ui.x, resolutions.ui.y);
        UIBackEnd::Init();
        Bible::Init();
        Gizmo::Init();
        Unloved::ViewportManager::Init();
        Editor::Init();
        Hell::Physics::Init();
        ImGuiBackEnd::Init();

        Systems::Init();

        return true;
    }

    void BeginFrame() {
        UpdateLazyKeypresses();
        DebugDraw::BeginFrame();
        Unloved::Session::BeginFrame();
        Systems::BeginFrame();
        RenderDataManager::BeginFrame();
        UIBackEnd::BeginFrame();
        World::BeginFrame();
    }

    void UpdateLoadingScreen() {
        UIBackEnd::Update();
        RenderDataManager::UpdateDrawCommandsUI();
    }

    void OnAssetLoadingComplete() {
        Renderer::UploadVertexData();
        HouseManager::Init();
        MapManager::Init();
        Renderer::InitWoundMaskArray();

        World::Init();

        // Free all cpu texture data
        for (auto& [name, texture] : Hell::ResourceManager::GetTextures()) {
            texture.FreeCPUMemory();
        }

        Renderer::InitMain();
        Unloved::Session::Create();
        GameAudio::PlayGlockEquipAudio();
    }

    void Update() {
        // Pre World Update
        Systems::PreWorldUpdate();

        Renderer::PreGameLogicComputePasses();

        float deltaTime = Time::DeltaTime();

        Unloved::ViewportManager::Update();

        if (Editor::IsOpen()) {
            Editor::Update(deltaTime);
        }

        AStarMap::Update();
        World::UpdateBvhs();
        Unloved::Session::Update();
        World::UpdatePlayers();
        BulletSystem::Update();
        World::UpdateObjects();

        // World Update
        World::Update();

        // Physics
        if (Editor::IsClosed()) {
            Hell::Physics::StepSimulation();
        }
        Hell::Physics::SyncRuntimeState();
        if (Editor::IsClosed()) {
            Hell::Physics::UpdateHeightFields();
        }
        else {
            Hell::Physics::ActivateAllHeightFields();
        }

        // Post World Update
        Session::PostWorldUpdate();
        Systems::PostWorldUpdate();

        // Render Submit
        World::SubmitRenderItems();

        Debug::Update();
        UIBackEnd::Update();
        RenderDataManager::Update();
        ImGuiBackEnd::Update();
    }

    void Render() {
        Renderer::RenderGame();
    }

    void EndFrame() {
        Debug::EndFrame();
        World::EndFrame();
    }

    void CleanUp() {
        Systems::CleanUp();
        World::CleanUp();
        Renderer::CleanUp();
    }
}
