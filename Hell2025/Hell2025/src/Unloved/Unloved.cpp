#include "Unloved.h"

#include "Hell/Audio.h"
namespace Audio = Hell::Audio;
#include "Hell/Backend/BackEnd.h"
#include "Bible/Bible.h"
#include "Callbacks/Callbacks.h"
#include "Core/GameOLD.h"
#include "Debug/Debug.h"
#include "Debug/DebugDraw.h"
#include "Editor/Editor.h"
#include "Editor/Gizmo.h"
#include "Imgui/ImguiBackEnd.h"
#include "Managers/HouseManager.h"
#include "Managers/MapManager.h"
#include "Managers/MirrorManager.h"
#include "Pathfinding/AStarMap.h"
#include "Pathfinding/NavMesh.h"
#include "Physics/Physics.h"
#include "Ragdoll/RagdollManager.h"
#include "Renderer/Renderer.h"
#include "Renderer/RenderDataManager.h"
#include "UI/UIBackEnd.h"
#include "Viewport/ViewportManager.h"
#include "Unloved/World/World.h"

#include "Hell/AssetCompiler/AssetCompiler.h"
#include "Hell/AssetLoader/AssetLoader.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/Input.h"
#include "Hell/Time.h"
namespace Input = Hell::Input;
namespace Time = Hell::Time;


namespace Unloved {
    void UpdateLazyKeypresses();

    bool Init() {
        UIBackEnd::Init();
        Bible::Init();
        Gizmo::Init();
        ViewportManager::Init();
        Editor::Init();
        Physics::Init();
        RagdollManager::Init();
        ImGuiBackEnd::Init();
        NavMeshManager::Init();
        Hell::AssetCompiler::CompileOutOfDateAssets();
        Hell::AssetLoader::Init();

        return true;
    }

    void UpdateSubSystems() {
        //glfwSwapInterval(0);

        World::Update();
    }

    void BeginFrame() {
        UpdateLazyKeypresses();
        DebugDraw::BeginFrame();
        GameOLD::BeginFrame();
        Physics::BeginFrame();
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
        GameOLD::Create();
    }

    void Update() {
        Renderer::PreGameLogicComputePasses();

        float deltaTime = Time::DeltaTime();

        ViewportManager::Update();

        if (Editor::IsOpen()) {
            Editor::Update(deltaTime);
        }

        AStarMap::Update();
        GameOLD::Update();
        MirrorManager::Update();

        Physics::UpdateAllRigidDynamics(deltaTime);
        Physics::UpdateActiveRigidDynamicAABBList();
        Physics::UpdateHeightFields();

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
        World::CleanUp();
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
        if (Input::KeyPressed(HELL_KEY_K)) GameOLD::RespawnPlayers();

        // Renderer
        if (Renderer::GameIsRendering()) {
            if (Input::KeyPressed(HELL_KEY_H))            Renderer::HotloadShaders();
            if (Input::KeyPressed(HELL_KEY_M))            Renderer::ToggleScreenSpaceReflections();
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
                GameOLD::NextSplitScreenMode();
            }
            if (Input::KeyPressed(HELL_KEY_1) && GameOLD::GetLocalPlayerCount() >= 1) {
                GameOLD::SetPlayerKeyboardAndMouseIndex(0, 0, 0);
                GameOLD::SetPlayerKeyboardAndMouseIndex(1, 1, 1);
                GameOLD::SetPlayerKeyboardAndMouseIndex(2, 1, 1);
                GameOLD::SetPlayerKeyboardAndMouseIndex(3, 1, 1);
            }
            if (Input::KeyPressed(HELL_KEY_2) && GameOLD::GetLocalPlayerCount() >= 2) {
                GameOLD::SetPlayerKeyboardAndMouseIndex(0, 1, 1);
                GameOLD::SetPlayerKeyboardAndMouseIndex(1, 0, 0);
                GameOLD::SetPlayerKeyboardAndMouseIndex(2, 1, 1);
                GameOLD::SetPlayerKeyboardAndMouseIndex(3, 1, 1);
            }
            if (Input::KeyPressed(HELL_KEY_3) && GameOLD::GetLocalPlayerCount() >= 3) {
                GameOLD::SetPlayerKeyboardAndMouseIndex(0, 1, 1);
                GameOLD::SetPlayerKeyboardAndMouseIndex(1, 1, 1);
                GameOLD::SetPlayerKeyboardAndMouseIndex(2, 0, 0);
                GameOLD::SetPlayerKeyboardAndMouseIndex(3, 1, 1);
            }
            if (Input::KeyPressed(HELL_KEY_4) && GameOLD::GetLocalPlayerCount() >= 4) {
                GameOLD::SetPlayerKeyboardAndMouseIndex(0, 1, 1);
                GameOLD::SetPlayerKeyboardAndMouseIndex(1, 1, 1);
                GameOLD::SetPlayerKeyboardAndMouseIndex(2, 1, 1);
                GameOLD::SetPlayerKeyboardAndMouseIndex(3, 0, 0);
            }
            if (Input::KeyPressed(HELL_KEY_B)) {
                Audio::PlayAudio(AUDIO_SELECT, 1.00f);
                Debug::NextDebugRenderMode();
            }
        }
    }
}
