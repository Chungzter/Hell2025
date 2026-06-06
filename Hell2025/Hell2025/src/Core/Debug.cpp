#include "Debug.h"
#include "Util.h"
#include "API/OpenGL/Renderer/GL_Renderer.h"
#include "BackEnd/BackEnd.h"
#include "Config/Config.h"
#include "Core/Game.h"
#include "Input/Input.h"
#include "Editor/Editor.h"
#include "Managers/MirrorManager.h"
#include "Renderer/Renderer.h"
#include "Renderer/RenderDataManager.h"
#include "Physics/Physics.h"
#include "Viewport/ViewportManager.h"
#include "UI/UIBackEnd.h"
#include "World/World.h"

#include "Audio/MidiFileManager.h"
#include <vector>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Debug {
    std::string g_text = "";
    bool g_showDebugText = false;
    DebugRenderMode g_debugRenderMode = DebugRenderMode::NONE;
    DebugTextMode g_debugTextMode = DebugTextMode::NONE;

    std::string g_quickMessage = UNDEFINED_STRING;
    float g_quickMessageTimer = 0;

    void UpdateDebugPointsAndLines();
    void UpdateDebugText();

    void Update() {
        UpdateDebugPointsAndLines();
        UpdateDebugText();

        // Quick message update
        if (g_quickMessageTimer > 0) {
            g_quickMessageTimer -= Game::GetDeltaTime();
            //UIBackEnd::BlitText(g_quickMessage, "StandardFont", Config::GetResolutions().gBuffer.x, 0, Alignment::TOP_RIGHT, 2.0f);
            UIBackEnd::BlitText(g_quickMessage, "StandardFont", 0, Config::GetResolutions().gBuffer.y, Alignment::BOTTOM_LEFT, 2.0f);
        }
    }

    void BlitQuickDebugMessage(const std::string& message) {
        g_quickMessageTimer = 2.0f;
        g_quickMessage = message;
    }

    void UpdateDebugText() {

        // Midi notes override
        if (MidiFileManager::IsPlaying()) {
            UIBackEnd::BlitText(MidiFileManager::GetDebugTextTime(), "StandardFont", 0, 0, Alignment::TOP_LEFT, 2.0f);
            UIBackEnd::BlitText(MidiFileManager::GetDebugTextEvents(), "StandardFont", 250, 0, Alignment::TOP_LEFT, 2.0f);
            UIBackEnd::BlitText(MidiFileManager::GetDebugTextVelocity(), "StandardFont", 500, 0, Alignment::TOP_LEFT, 2.0f);
            UIBackEnd::BlitText(MidiFileManager::GetDebugTextTimeDurations(), "StandardFont", 750, 0, Alignment::TOP_LEFT, 2.0f);
            return;
        }

        if (Debug::GetDebugTextMode() == DebugTextMode::PER_PLAYER) return;
        if (Debug::GetDebugTextMode() == DebugTextMode::NONE)       return;
        if (Editor::IsOpen())                                       return;

        // Regular global debug
        std::string text = "";

        // Mirrors
        if (false) {
            text += "Mirror count: " + std::to_string(MirrorManager::GetMirrors().size()) + "\n";
            for (Mirror& mirror : MirrorManager::GetMirrors()) {
                text += "- ";
                text += std::to_string(mirror.GetObjectId()) + " ";
                text += Util::Vec3ToString(mirror.GetWorldCenter()) + "\n";
            }

            Viewport* viewport = ViewportManager::GetViewportByIndex(0);

            text += " ";
            text += "Viewport Mirror ID: " + std::to_string(viewport->GetMirrorId()) + "\n";

        }


        UIBackEnd::BlitText(text, "StandardFont", 0, 0, Alignment::TOP_LEFT, 2.0f);

        return;


        const DebugRenderMode& debugRenderMode = Debug::GetDebugRenderMode();
        if (debugRenderMode != DebugRenderMode::NONE) {
            AddText("Line Mode: " + Util::DebugRenderModeToString(debugRenderMode));
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
        Viewport* viewport = ViewportManager::GetViewportByIndex(hoveredViewportIndex);

        int mouseX = Input::GetMouseX();
        int mouseY = Input::GetMouseY();
        int windowWidth = BackEnd::GetCurrentWindowWidth();
        int windowHeight = BackEnd::GetCurrentWindowHeight();
        int gBufferWidth = resolutions.gBuffer.x;
        int gBufferHeight = resolutions.gBuffer.y;
        int viewportWidth = gBufferWidth * viewport->GetSize().x;
        int viewportHeight = gBufferHeight * viewport->GetSize().y;
        float normalizedMouseX = Util::MapRange(mouseX, 0, windowWidth, 0, gBufferWidth);
        float normalizedMouseY = Util::MapRange(mouseY, 0, windowHeight, 0, gBufferHeight);

        float offsetX = viewport->GetPosition().x * gBufferWidth;
        float offsetY = (1 - viewport->GetPosition().y) * gBufferHeight;

        float localX = normalizedMouseX - offsetX;
        float localY = normalizedMouseY - offsetY + viewportHeight;


        float width = viewport->GetSize().x * BackEnd::GetCurrentWindowWidth();
        float height = viewport->GetSize().y * BackEnd::GetCurrentWindowHeight();
        float left = viewport->GetPosition().x * BackEnd::GetCurrentWindowWidth();
        float right = left + width;
        float top = BackEnd::GetCurrentWindowHeight() - (viewport->GetPosition().y * BackEnd::GetCurrentWindowHeight());
        float bottom = top - height;

        float viewportSpaceMouseX = Util::MapRange(mouseX, left, right, 0, viewportWidth);
        float viewportSpaceMouseY = Util::MapRange(mouseY, bottom, top, 0, viewportHeight);

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
        AddText("Hovered Viewport Index: " + std::to_string(hoveredViewportIndex));
        AddText("Mouse ray origin: " + Util::Vec3ToString(Editor::GetMouseRayOriginByViewportIndex(hoveredViewportIndex)));
        AddText("Mouse ray direction: " + Util::Vec3ToString(Editor::GetMouseRayDirectionByViewportIndex(hoveredViewportIndex)));
    }

    void UpdateDebugPointsAndLines() {

        if (g_debugRenderMode == DebugRenderMode::LIGHTS) {
            static uint32_t lightIndex = 2;
            static bool even = true;

            if (Input::KeyPressed(HELL_KEY_UP)) even =! even;
            if (Input::KeyPressed(HELL_KEY_LEFT)) lightIndex--;
            if (Input::KeyPressed(HELL_KEY_RIGHT)) lightIndex++;

            if (lightIndex < 0) lightIndex = World::GetLightCount() - 1;
            if (lightIndex == World::GetLightCount()) lightIndex = 0;

            Light* light = World::GetLightByIndex(lightIndex);

            AABB cullBounds = AABB(light->GetCullBoundsMin(), light->GetCullBoundsMax());
            glm::vec4 color = glm::vec4(light->GetColor(), 1.0f);
            glm::vec3 position = light->GetPosition();
            
            Renderer::DrawAABB(cullBounds, color);
            Renderer::DrawPoint(position, color);

            int numRays = 100;
            float spreadAngleRadians = 1;
            glm::vec3 targetDir = glm::vec3(0, -1, 0);

            std::vector<glm::vec3> dirs;
            
            if (even) dirs = Util::GenerateRayDirections(numRays);
            else dirs = Util::GenerateFibonacciCone(numRays, spreadAngleRadians, targetDir);

            for (const glm::vec3& dir : dirs) {
                glm::vec3 endPoint = position + (dir * light->GetRadius());
                Renderer::DrawLine(position, endPoint, color);
            }
        }

        if (g_debugRenderMode == DebugRenderMode::BONES) {
            for (AnimatedGameObject& animatedGameObject : World::GetAnimatedGameObjects()) {
                animatedGameObject.DrawBones();
            }
            for (int i = 0; i < Game::GetLocalPlayerCount(); i++) {
                Player* player = Game::GetLocalPlayerByIndex(i);
                //player->GetCharacterModelAnimatedGameObject()->DrawBones(RED, i);
                player->GetViewWeaponAnimatedGameObject()->DrawBones(i);
                //player->GetCharacterModelAnimatedGameObject()->DrawBones();
            }
        }
        if (g_debugRenderMode == DebugRenderMode::BONE_TANGENTS) {
            for (AnimatedGameObject& animatedGameObject : World::GetAnimatedGameObjects()) {
                //animatedGameObject.DrawBoneTangentVectors();
            }
            for (int i = 0; i < Game::GetLocalPlayerCount(); i++) {
                Player* player = Game::GetLocalPlayerByIndex(i);
                //player->GetCharacterModelAnimatedGameObject()->DrawBoneTangentVectors(0.001f, i);
                player->GetViewWeaponAnimatedGameObject()->DrawBoneTangentVectors(0.0025f, i);
                //player->GetCharacterModelAnimatedGameObject()->DrawBoneTangentVectors(0.001f, i);
            }
        }
        if (g_debugRenderMode == DebugRenderMode::CLIPPING_CUBES) {
            for (ClippingCube& clippingCube : World::GetClippingCubes()) {
                clippingCube.DrawDebugCorners(OUTLINE_COLOR);
                clippingCube.DrawDebugEdges(WHITE);
            }
        }
        if (g_debugRenderMode == DebugRenderMode::DECALS) {
            for (const Decal& decal : World::GetDecals()) {
                Renderer::DrawPoint(decal.GetPosition(), OUTLINE_COLOR);
                Renderer::DrawLine(decal.GetPosition(), decal.GetPosition() + decal.GetWorldNormal() * 0.05f, OUTLINE_COLOR);
            }
        }
        if (g_debugRenderMode == DebugRenderMode::PHYSX_ALL ||
            g_debugRenderMode == DebugRenderMode::PHYSX_COLLISION ||
            g_debugRenderMode == DebugRenderMode::RAGDOLLS ||
            g_debugRenderMode == DebugRenderMode::PHYSX_RAYCAST) {
            Physics::ForceZeroStepUpdate();
            Physics::SubmitDebugLinesToRenderer(g_debugRenderMode);
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
        std::cout << "Debug text mode: " << Util::DebugTextModeToString(g_debugTextMode) << "\n";

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
            CLIPPING_CUBES,
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
    }

    const DebugRenderMode& GetDebugRenderMode() {
        return g_debugRenderMode;
    }

    const DebugTextMode& GetDebugTextMode() {
        return g_debugTextMode;
    }
}