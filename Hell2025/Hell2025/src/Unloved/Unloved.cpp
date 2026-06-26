#include "Unloved.h"

#include "Hell/Audio.h"
#include "Hell/Backend/BackEnd.h"
#include "Hell/Physics/Physics.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/UI/UIBackEnd.h"
#include "Hell/Input.h"
#include "Hell/Time.h"

#include "Legacy/Bible/Bible.h"
#include "Legacy/Callbacks/Callbacks.h"
#include "Legacy/Config/Config.h"
#include "Legacy/Debug/Debug.h"
#include "Legacy/Debug/DebugDraw.h"
#include "Legacy/Editor/Editor.h"
#include "Legacy/Editor/Gizmo.h"
#include "Legacy/Imgui/ImguiBackEnd.h"
#include "Legacy/Managers/HouseManager.h"
#include "Legacy/Managers/MapManager.h"
#include "Legacy/Pathfinding/AStarMap.h"
#include "Legacy/Renderer/Renderer.h"
#include "Legacy/Renderer/RenderDataManager.h"
#include "Legacy/Viewport/ViewportManager.h"

#include "Unloved/Session/Session.h"
#include "Unloved/SubSystems/GameAudio/GameAudio.h"
#include "Unloved/SubSystems/SubSystems.h"
#include "Unloved/World/World.h"

namespace Input = Hell::Input;
namespace Time = Hell::Time;


namespace Unloved {
    void UpdateLazyKeypresses();

    bool Init() {
        Renderer::Init();
        const Resolutions& resolutions = Config::GetResolutions();
        UIBackEnd::SetUIResolution(resolutions.ui.x, resolutions.ui.y);
        UIBackEnd::Init();
        Bible::Init();
        Gizmo::Init();
        ViewportManager::Init();
        Editor::Init();
        Hell::Physics::Init();
        ImGuiBackEnd::Init();

        SubSystems::Init();

        return true;
    }

    void BeginFrame() {
        UpdateLazyKeypresses();
        DebugDraw::BeginFrame();
        Unloved::Session::BeginFrame();
        SubSystems::BeginFrame();
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
        SubSystems::PreWorldUpdate();

        Renderer::PreGameLogicComputePasses();

        float deltaTime = Time::DeltaTime();

        ViewportManager::Update();

        if (Editor::IsOpen()) {
            Editor::Update(deltaTime);
        }

        AStarMap::Update();
        World::UpdateBvhs();
        Unloved::Session::Update();
        World::UpdatePlayers();
        World::ProcessBullets();
        World::UpdateLegacyObjects();
        World::Update();

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

        SubSystems::PostWorldUpdate();

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
        SubSystems::CleanUp();
        World::CleanUp();
        Renderer::CleanUp();
    }

    void UpdateLazyKeypresses() {
        // Bail early if ImGui is using the keyboard
        if (ImGuiBackEnd::HasKeyboardFocus()) return;

        // Function keys
        if (Input::KeyPressed(HELL_KEY_F1)) Callbacks::NewRun();
        if (Input::KeyPressed(HELL_KEY_F4)) Callbacks::OpenHouseEditor();
        if (Input::KeyPressed(HELL_KEY_F6)) Callbacks::OpenMapHeightEditor();
        if (Input::KeyPressed(HELL_KEY_F5)) Callbacks::OpenMapObjectEditor();

        // Core
        if (Input::KeyPressed(HELL_KEY_ESCAPE))       Hell::BackEnd::ForceCloseWindow();
        if (Input::KeyPressed(HELL_KEY_X))            Hell::BackEnd::ToggleFullscreen();
        if (Input::KeyPressed(HELL_KEY_GRAVE_ACCENT)) Debug::NextDebugTextMode();

        // Game
        if (Input::KeyPressed(HELL_KEY_K)) Unloved::Session::RespawnPlayers();

        // Renderer
        if (Renderer::GameIsRendering()) {
            if (Input::KeyPressed(HELL_KEY_H))            Renderer::HotloadShaders();
            if (Input::KeyPressed(HELL_KEY_I))            Renderer::ToggleRagdollRendering();
            if (Input::KeyPressed(HELL_KEY_M))            Renderer::ToggleScreenSpaceReflections();
            if (Input::KeyPressed(HELL_KEY_O))            Renderer::ToggleDebugDraw();
            if (Input::KeyPressed(HELL_KEY_L))            Renderer::ToggleLighting();
            if (Input::KeyPressed(HELL_KEY_SEMICOLON))    Renderer::ToggleSphericalHarmonics();
            if (Input::KeyPressed(HELL_KEY_COMMA))        Renderer::TogglePointCloud();
            if (Input::KeyPressed(HELL_KEY_PERIOD))       Renderer::NextProbeDebugState();
            if (Input::KeyPressed(HELL_KEY_SLASH))        Renderer::ToggleIrradianceProbeSampling();
            if (Input::KeyPressed(HELL_KEY_RIGHT_SHIFT))  Renderer::ToggleOverrideState(RendererOverrideState::INDIRECT_DIFFUSE);
            //if (Input::KeyPressed(HELL_KEY_RIGHT_SHIFT))  Renderer::ToggleOverrideState(RendererOverrideState::DEPTH);
            if (Input::KeyPressed(HELL_KEY_ENTER))        Renderer::ToggleOverrideState(RendererOverrideState::WORLD_POSITION);
            if (Input::KeyPressed(HELL_KEY_V))            Renderer::ToggleOverrideState(RendererOverrideState::VIS_BUFFER);
            if (Input::KeyPressed(HELL_KEY_DELETE))       Renderer::ToggleOverrideState(RendererOverrideState::VELOCITY);
            if (Input::KeyPressed(HELL_KEY_APOSTROPHE))   Renderer::TogglePointCloudGrid();
            if (Input::KeyPressed(HELL_KEY_BACKSLASH))    Renderer::NextRendererOverrideState();
            if (Input::KeyPressed(HELL_KEY_LEFT_BRACKET)) Renderer::NextRendererMode();
        }

        // Editor only
        if (!Editor::IsOpen()) {
            if (Input::KeyPressed(HELL_KEY_C)) {
                Unloved::Session::NextSplitScreenMode();
            }
            if (Input::KeyPressed(HELL_KEY_1) && Unloved::Session::GetLocalPlayerCount() >= 1) {
                Unloved::Session::SetPlayerKeyboardAndMouseIndex(0, 0, 0);
                Unloved::Session::SetPlayerKeyboardAndMouseIndex(1, 1, 1);
                Unloved::Session::SetPlayerKeyboardAndMouseIndex(2, 1, 1);
                Unloved::Session::SetPlayerKeyboardAndMouseIndex(3, 1, 1);
            }
            if (Input::KeyPressed(HELL_KEY_2) && Unloved::Session::GetLocalPlayerCount() >= 2) {
                Unloved::Session::SetPlayerKeyboardAndMouseIndex(0, 1, 1);
                Unloved::Session::SetPlayerKeyboardAndMouseIndex(1, 0, 0);
                Unloved::Session::SetPlayerKeyboardAndMouseIndex(2, 1, 1);
                Unloved::Session::SetPlayerKeyboardAndMouseIndex(3, 1, 1);
            }
            if (Input::KeyPressed(HELL_KEY_3) && Unloved::Session::GetLocalPlayerCount() >= 3) {
                Unloved::Session::SetPlayerKeyboardAndMouseIndex(0, 1, 1);
                Unloved::Session::SetPlayerKeyboardAndMouseIndex(1, 1, 1);
                Unloved::Session::SetPlayerKeyboardAndMouseIndex(2, 0, 0);
                Unloved::Session::SetPlayerKeyboardAndMouseIndex(3, 1, 1);
            }
            if (Input::KeyPressed(HELL_KEY_4) && Unloved::Session::GetLocalPlayerCount() >= 4) {
                Unloved::Session::SetPlayerKeyboardAndMouseIndex(0, 1, 1);
                Unloved::Session::SetPlayerKeyboardAndMouseIndex(1, 1, 1);
                Unloved::Session::SetPlayerKeyboardAndMouseIndex(2, 1, 1);
                Unloved::Session::SetPlayerKeyboardAndMouseIndex(3, 0, 0);
            }
            if (Input::KeyPressed(HELL_KEY_B)) {
                Hell::Audio::PlayAudio(AUDIO_SELECT, 1.00f);
                Debug::NextDebugRenderMode();
            }
        }
    }
}
