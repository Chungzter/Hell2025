#include "Debug.h"

#include "Hell/Common/Enum.h"
#include "Hell/Common/String.h"
#include "Hell/Logging.h"
#include "Hell/Math/Range.h"
#include "Hell/MemoryTracker/MemoryTracker.h"
#include "Hell/ResourceManagement/ResourceManager.h"

#include "Hell/Backend/BackEnd.h"
#include "Unloved/Config/Config.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Debug/DebugDraw.h"
#include "Unloved/Editor/Editor.h"
#include "Unloved/Systems/Mirrors/MirrorManager.h"
#include "Renderer/Renderer.h"
#include "Renderer/RenderDataManager.h"
#include "Hell/Physics/Physics.h"
#include "Unloved/Viewport/ViewportManager.h"
#include "Hell/UI/UIBackEnd.h"
#include "World/LegacyWorld.h"
#include "Hell/UI/TextBlitter.h"
#include "Unloved/World/World.h"

#include "Unloved/Systems/PianoPlayback/PianoPlaybackManager.h"

#include <cstdint>
#include <vector>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Hell/Input.h"
#include "Hell/Audio.h"
#include "Hell/Time.h"
namespace Audio = Hell::Audio;
namespace Input = Hell::Input;


namespace Debug {
    using namespace Unloved;

    std::string g_text = "";
    bool g_showDebugText = false;
    DebugRenderMode g_debugRenderMode = DebugRenderMode::NONE;
    DebugTextMode g_debugTextMode = DebugTextMode::NONE;

    std::string g_quickMessage = UNDEFINED_STRING;
    float g_quickMessageTimer = 0;

    Hell::Physics::DebugMode ToPhysicsDebugMode(DebugRenderMode debugRenderMode) {
        switch (debugRenderMode) {
            case DebugRenderMode::PHYSX_ALL:       return Hell::Physics::DebugMode::ALL;
            case DebugRenderMode::PHYSX_RAYCAST:   return Hell::Physics::DebugMode::RAYCAST_SHAPES;
            case DebugRenderMode::PHYSX_COLLISION: return Hell::Physics::DebugMode::COLLISION_SHAPES;
            case DebugRenderMode::RAGDOLLS:        return Hell::Physics::DebugMode::RAGDOLLS;
            default:                               return Hell::Physics::DebugMode::NONE;
        }
    }

    void DisplayGlobalDebugText();
    void DisplayMemoryTrackerInfo();
    void DisplayProfilingInfo();
    void DisplayQuickMessage();

    void UpdateDebugPointsAndLines();
    void UpdateDebugText();

    void Update() {
        UpdateDebugPointsAndLines();
        UpdateDebugText();

        if (Debug::GetDebugTextMode() == DebugTextMode::GLOBAL)         DisplayGlobalDebugText();
        if (Debug::GetDebugTextMode() == DebugTextMode::MEMORY_TRACKER) DisplayMemoryTrackerInfo();
        if (Debug::GetDebugTextMode() == DebugTextMode::PROFILING)      DisplayProfilingInfo();

        DisplayQuickMessage();
    }

    void UpdateDebugText() {

        // Midi notes override
        if (Unloved::PianoPlaybackManager::IsPlaying()) {
            UIBackEnd::BlitText(Unloved::PianoPlaybackManager::GetDebugTextTime(), "StandardFont", 0, 0, Alignment::TOP_LEFT, 2.0f);
            UIBackEnd::BlitText(Unloved::PianoPlaybackManager::GetDebugTextEvents(), "StandardFont", 250, 0, Alignment::TOP_LEFT, 2.0f);
            UIBackEnd::BlitText(Unloved::PianoPlaybackManager::GetDebugTextVelocity(), "StandardFont", 500, 0, Alignment::TOP_LEFT, 2.0f);
            UIBackEnd::BlitText(Unloved::PianoPlaybackManager::GetDebugTextTimeDurations(), "StandardFont", 750, 0, Alignment::TOP_LEFT, 2.0f);
            return;
        }

        if (Debug::GetDebugTextMode() == DebugTextMode::PER_PLAYER) return;
        if (Debug::GetDebugTextMode() == DebugTextMode::NONE)       return;
        if (Editor::IsOpen())                                       return;

        // Regular global debug
        std::string text = "";

        // Mirrors
        if (false) {
            text += "Mirror count: " + std::to_string(Unloved::MirrorManager::GetMirrors().size()) + "\n";
            for (Mirror& mirror : Unloved::MirrorManager::GetMirrors()) {
                text += "- ";
                text += std::to_string(mirror.GetObjectId()) + " ";
                text += Hell::String::FormatVec3(mirror.GetWorldCenter()) + "\n";
            }

            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(0);

            text += " ";
            text += "Unloved::Viewport Mirror ID: " + std::to_string(viewport->GetMirrorId()) + "\n";

        }


        UIBackEnd::BlitText(text, "StandardFont", 0, 0, Alignment::TOP_LEFT, 2.0f);

        return;


        const DebugRenderMode& debugRenderMode = Debug::GetDebugRenderMode();
        if (debugRenderMode != DebugRenderMode::NONE) {
            AddText("Line Mode: " + Hell::Enum::ToString(debugRenderMode));
        }

        std::string text2 = "SHIT\n";
        UIBackEnd::BlitText(text, "StandardFont", 0, 0, Alignment::TOP_LEFT, 2.0f);

        return;

        const Resolutions& resolutions = Config::GetResolutions();
        const std::vector<ViewportData>& viewportData = RenderDataManager::GetViewportData();

        int i = 0;
        glm::mat4 projectionMatrix = viewportData[i].projection;
        glm::mat4 viewMatrix = viewportData[i].view;
        glm::mat4 inverseViewMatrix = viewportData[i].inverseView;
        glm::vec3 viewPos = inverseViewMatrix[3];
        glm::vec3 rayOrigin = viewPos;

        int hoveredViewportIndex = Editor::GetHoveredViewportIndex();
        Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(hoveredViewportIndex);

        int mouseX = Input::GetMouseX();
        int mouseY = Input::GetMouseY();
        int windowWidth = Hell::BackEnd::GetCurrentWindowWidth();
        int windowHeight = Hell::BackEnd::GetCurrentWindowHeight();
        int gBufferWidth = resolutions.gBuffer.x;
        int gBufferHeight = resolutions.gBuffer.y;
        int viewportWidth = gBufferWidth * viewport->GetSize().x;
        int viewportHeight = gBufferHeight * viewport->GetSize().y;
        float normalizedMouseX = Hell::Math::MapRange(mouseX, 0, windowWidth, 0, gBufferWidth);
        float normalizedMouseY = Hell::Math::MapRange(mouseY, 0, windowHeight, 0, gBufferHeight);

        float offsetX = viewport->GetPosition().x * gBufferWidth;
        float offsetY = (1 - viewport->GetPosition().y) * gBufferHeight;

        float localX = normalizedMouseX - offsetX;
        float localY = normalizedMouseY - offsetY + viewportHeight;


        float width = viewport->GetSize().x * Hell::BackEnd::GetCurrentWindowWidth();
        float height = viewport->GetSize().y * Hell::BackEnd::GetCurrentWindowHeight();
        float left = viewport->GetPosition().x * Hell::BackEnd::GetCurrentWindowWidth();
        float right = left + width;
        float top = Hell::BackEnd::GetCurrentWindowHeight() - (viewport->GetPosition().y * Hell::BackEnd::GetCurrentWindowHeight());
        float bottom = top - height;

        float viewportSpaceMouseX = Hell::Math::MapRange(mouseX, left, right, 0, viewportWidth);
        float viewportSpaceMouseY = Hell::Math::MapRange(mouseY, bottom, top, 0, viewportHeight);

        AddText("");
        AddText("viewportSpaceMouseX: " + std::to_string(viewportSpaceMouseX));
        AddText("viewportSpaceMouseY: " + std::to_string(viewportSpaceMouseY));
        AddText("");
        AddText("WindowWidth: " + std::to_string(windowWidth));
        AddText("WindowHeight: " + std::to_string(windowHeight));
        AddText("gBufferWidth: " + std::to_string(gBufferWidth));
        AddText("gBufferHeight: " + std::to_string(gBufferHeight));
        AddText("viewportWidth: " + std::to_string(viewportWidth));
        AddText("viewportHeight: " + std::to_string(viewportHeight));
        AddText("mouseX: " + std::to_string(mouseX));
        AddText("mouseY: " + std::to_string(mouseY));
        AddText("localX: " + std::to_string(localX));
        AddText("localY: " + std::to_string(localY));
        AddText("normalizedMouseX: " + std::to_string(normalizedMouseX));
        AddText("normalizedMouseY: " + std::to_string(normalizedMouseY));
        AddText("Hovered Unloved::Viewport Index: " + std::to_string(hoveredViewportIndex));
        AddText("Mouse ray origin: " + Hell::String::FormatVec3(Editor::GetMouseRayOriginByViewportIndex(hoveredViewportIndex)));
        AddText("Mouse ray direction: " + Hell::String::FormatVec3(Editor::GetMouseRayDirectionByViewportIndex(hoveredViewportIndex)));
    }

    void BlitQuickDebugMessage(const std::string& message) {
        g_quickMessageTimer = 2.0f;
        g_quickMessage = message;
    }

    void DisplayGlobalDebugText() {
        std::string text = Debug::GetText();
        UIBackEnd::BlitText(text, "StandardFont", 0, 0, Alignment::TOP_LEFT, 2.0f);
    }

    void DisplayMemoryTrackerInfo() {
        using namespace Hell::MemoryTracker;

        constexpr char fontName[] = "StandardFont";
        constexpr float scale = 2.0f;
        constexpr uint32_t spacing = 50;

        const std::string headingColor = "[COL=0.56,0.93,0.56,1.0]";
        const std::string rowColor = "[COL=1.0,0.65,0.0,1.0]";

        std::string names = "\n";
        std::string cpuBytes = headingColor + "CPU\n";
        std::string gpuBytes = headingColor + "GPU\n";

        MemoryReport memoryReport = GetMemoryReport();

        for (const MemoryReportCategory& category : memoryReport.categories) {
            names += rowColor + category.name + "\n";
            gpuBytes += rowColor + FormatMemorySize(category.GetTotalGPUBytes()) + "\n";
            cpuBytes += rowColor + FormatMemorySize(category.GetTotalCPUBytes()) + "\n";
        }

        names += "\n" + headingColor + "Total\n";
        gpuBytes += "\n" + headingColor + FormatMemorySize(memoryReport.GetTotalGPUBytes()) + "\n";
        cpuBytes += "\n" + headingColor + FormatMemorySize(memoryReport.GetTotalCPUBytes()) + "\n";

        const int namesWidth = TextBlitter::GetTextSize(names, fontName, scale).x;
        const int gpuWidth = TextBlitter::GetTextSize(gpuBytes, fontName, scale).x;
        const int gpuX = namesWidth + static_cast<int>(spacing);
        const int cpuX = gpuX + gpuWidth + static_cast<int>(spacing);

        UIBackEnd::BlitText(names, fontName, 0, 0, Alignment::TOP_LEFT, scale);
        UIBackEnd::BlitText(gpuBytes, fontName, gpuX, 0, Alignment::TOP_LEFT, scale);
        UIBackEnd::BlitText(cpuBytes, fontName, cpuX, 0, Alignment::TOP_LEFT, scale);
    }

    void DisplayProfilingInfo() {
        float scale = 1.4f;
        int margin = 35;
        TextureFilter textureFilter = TextureFilter::LINEAR;

        const std::string white = "[COL=1.0,1.0,1.0,1.0]";
        const std::string orange = "[COL=1.0,0.65,0.0,1.0]";
        const std::string yellow = "[COL=1.0,1.0,0.0,1.0]";
        const std::string red = "[COL=1.0,0.0,0.0,1.0]";
        const std::string lightBlue = "[COL=0.68,0.85,0.9,1.0]";
        const std::string lightGreen = "[COL=0.56,0.93,0.56,1.0]";

        const std::string headingColor = lightGreen;

        std::string names = "\n" + Renderer::GetZoneNames();
        std::string timingsGPU = headingColor + "GPU\n" + Renderer::GetZoneGPUTimings();
        std::string timingsCPU = headingColor + "CPU\n" + Renderer::GetZoneCPUTimings();
        glm::ivec2 namesSize = TextBlitter::GetTextSize(names, "StandardFont", scale);
        glm::ivec2 timingsGPUSize = TextBlitter::GetTextSize(timingsGPU, "StandardFont", scale);
        glm::ivec2 timingsCPUSize = TextBlitter::GetTextSize(timingsCPU, "StandardFont", scale);

        if (names.length() != 0) {
            timingsGPU += "\n" + headingColor + "Total GPU : " + Renderer::GetTotalGPUTime();
            UIBackEnd::BlitText(timingsGPU, "StandardFont", 0, 0, Alignment::TOP_LEFT, scale, textureFilter);
            UIBackEnd::BlitText(timingsCPU, "StandardFont", timingsGPUSize.x + margin, 0, Alignment::TOP_LEFT, scale, textureFilter);
            UIBackEnd::BlitText(names, "StandardFont", timingsGPUSize.x + margin + timingsCPUSize.x + margin, 0, Alignment::TOP_LEFT, scale, textureFilter);
        }
    }

    void DisplayQuickMessage() {
        if (g_quickMessageTimer > 0) {
            g_quickMessageTimer -= Hell::Time::DeltaTime();
            UIBackEnd::BlitText(g_quickMessage, "StandardFont", 0, Config::GetResolutions().gBuffer.y, Alignment::BOTTOM_LEFT, 2.0f);
        }
    }

    void UpdateDebugPointsAndLines() {

        if (g_debugRenderMode == DebugRenderMode::LIGHTS) {
            static uint32_t lightIndex = 2;

            if (Input::KeyPressed(HELL_KEY_LEFT)) lightIndex--;
            if (Input::KeyPressed(HELL_KEY_RIGHT)) lightIndex++;

            if (lightIndex < 0) lightIndex = Unloved::World::GetLightCount() - 1;
            if (lightIndex == Unloved::World::GetLightCount()) lightIndex = 0;

            if (Light* light = Unloved::World::GetLightByIndex(lightIndex)) {
                AABB worldBounds = AABB(light->GetWorldBoundsMin(), light->GetWorldBoundsMax());
                DebugDraw::DrawAABB(worldBounds, glm::vec4(light->GetColor(), 1.0f));
            }
        }

        if (g_debugRenderMode == DebugRenderMode::BONES) {
            for (AnimatedGameObject& animatedGameObject : Unloved::World::GetAnimatedGameObjects()) {
                animatedGameObject.DrawBones();
            }
            for (int i = 0; i < Unloved::Session::GetLocalPlayerCount(); i++) {
                Unloved::Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(i);
                //player->GetCharacterModelAnimatedGameObject()->DrawBones(RED, i);
                player->GetViewWeaponAnimatedGameObject()->DrawBones(i);
                //player->GetCharacterModelAnimatedGameObject()->DrawBones();
            }
        }
        if (g_debugRenderMode == DebugRenderMode::BONE_TANGENTS) {
            for (AnimatedGameObject& animatedGameObject : Unloved::World::GetAnimatedGameObjects()) {
                //animatedGameObject.DrawBoneTangentVectors();
            }
            for (int i = 0; i < Unloved::Session::GetLocalPlayerCount(); i++) {
                Unloved::Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(i);
                //player->GetCharacterModelAnimatedGameObject()->DrawBoneTangentVectors(0.001f, i);
                player->GetViewWeaponAnimatedGameObject()->DrawBoneTangentVectors(0.0025f, i);
                //player->GetCharacterModelAnimatedGameObject()->DrawBoneTangentVectors(0.001f, i);
            }
        }
        if (g_debugRenderMode == DebugRenderMode::CLIPPING_VOLUMES) {
            for (const Door& door : Unloved::World::GetDoors()) {
                door.GetClippingVolume().DrawDebugCorners(OUTLINE_COLOR);
                door.GetClippingVolume().DrawDebugEdges(WHITE);
            }
            for (const Window& window : Unloved::World::GetWindows()) {
                window.GetClippingVolume().DrawDebugCorners(OUTLINE_COLOR);
                window.GetClippingVolume().DrawDebugEdges(WHITE);
            }
        }
        if (g_debugRenderMode == DebugRenderMode::BLOCKING_VOLUMES) {
            for (const Fireplace& fireplace : Unloved::World::GetFireplaces()) {
                fireplace.GetBlockingVolume().DrawDebugCorners(OUTLINE_COLOR);
                fireplace.GetBlockingVolume().DrawDebugEdges(WHITE);
            }
        }
        if (g_debugRenderMode == DebugRenderMode::DECALS) {
            for (const Decal& decal : LegacyWorld::GetDecals()) {
                DebugDraw::DrawPoint(decal.GetPosition(), OUTLINE_COLOR);
                DebugDraw::DrawLine(decal.GetPosition(), decal.GetPosition() + decal.GetWorldNormal() * 0.05f, OUTLINE_COLOR);
            }
        }
        Hell::Physics::DebugMode physicsDebugMode = ToPhysicsDebugMode(g_debugRenderMode);
        if (physicsDebugMode != Hell::Physics::DebugMode::NONE) {
            Hell::Physics::ForceZeroStepUpdate();
            for (const Hell::Physics::PhysicsDebugLine& line : Hell::Physics::GetPhysicsDebugLines(physicsDebugMode)) {
                DebugDraw::DrawLine(line.p1, line.p2, line.color);
            }
        }
    }

    void AddText(const std::string& text) {
        g_text += text + "\n";
    }

    const std::string& GetText() {
        return g_text;
    }

    void EndFrame() {
        g_text = "";
    }

    void NextDebugTextMode() {
        Audio::PlayAudio(AUDIO_SELECT, 1.00f);

        g_debugTextMode = (DebugTextMode)(int(g_debugTextMode) + 1);
        if (g_debugTextMode == DebugTextMode::DEBUG_TEXT_MODE_COUNT) {
            g_debugTextMode = (DebugTextMode)0;
        }
        std::cout << "Debug text mode: " << Hell::Enum::ToString(g_debugTextMode) << "\n";

        if (g_debugTextMode == DebugTextMode::GLOBAL) {
            std::cout << ".. skipping DebugTextMode::GLOBAL\n";
            NextDebugTextMode();
        }
    }

    void SetDebugRenderMode(DebugRenderMode mode) {
        g_debugRenderMode = mode;
    }

    void NextDebugRenderMode() {
        std::vector<DebugRenderMode> allowedDebugRenderModes = {
            NONE,
            PHYSX_ALL,
            RAGDOLLS,
            CLIPPING_VOLUMES,
            BLOCKING_VOLUMES,
            //HOUSE_GEOMETRY,
            DECALS,
            BONES,
            BONE_TANGENTS,
            LIGHTS,
            BVH_CPU_PLAYER_RAYS
            //PATHFINDING,
            //PHYSX_COLLISION,
            //PATHFINDING_RECAST,
            //RTX_LAND_TOP_LEVEL_ACCELERATION_STRUCTURE,
            //RTX_LAND_BOTTOM_LEVEL_ACCELERATION_STRUCTURES,
            //BOUNDING_BOXES,
        };

        g_debugRenderMode = (DebugRenderMode)(int(g_debugRenderMode) + 1);
        if (g_debugRenderMode == DEBUG_LINE_MODE_COUNT) {
            g_debugRenderMode = (DebugRenderMode)0;
        }
        // If mode isn't in available modes list, then go to next
        bool allowed = false;
        for (auto& avaliableMode : allowedDebugRenderModes) {
            if (g_debugRenderMode == avaliableMode) {
                allowed = true;
                break;
            }
        }
        if (!allowed && g_debugRenderMode != DebugRenderMode::NONE) {
            NextDebugRenderMode();
        }

        Debug::BlitQuickDebugMessage("Debug Render Mode: " + Hell::Enum::ToString(g_debugRenderMode));
        Audio::PlayAudio(AUDIO_SELECT, 1.00f);
    }

    void PrintModelMeshNames(const std::string& name) {
        Model* model = Hell::ResourceManager::GetModelByName(name);
        if (!model) {
            Logging::Error() << "Debug::PrintModelMeshNames(..) failed coz model param was nullptr\n";
            return;
        }

        std::cout << model->GetName() << "\n";
        for (const uint32_t& meshId : model->GetMeshIndices()) {
            Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(meshId);
            if (mesh) {
                std::cout << " - " << mesh->name << "\n";
            }
            else {
                std::cout << " - INVALID MESH SOMEHOW\n";
            }
        }
    }

    const DebugRenderMode& GetDebugRenderMode() {
        return g_debugRenderMode;
    }

    const DebugTextMode& GetDebugTextMode() {
        return g_debugTextMode;
    }
}
