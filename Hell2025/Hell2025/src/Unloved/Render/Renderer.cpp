#include "Renderer.h"

#include "Hell/Audio.h"
#include "Hell/Backend/BackEnd.h"
#include "Hell/Logging.h"
#include "Hell/Render/API/OpenGL/GL_back_end.h"
#include "Hell/Render/API/OpenGL/GL_resource_manager.h"
#include "Hell/ResourceManagement/ResourceManager.h"

#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Unloved/Config/Config.h"
#include "Unloved/Debug/Debug.h"
#include "Unloved/Editor/Editor.h"
#include "Unloved/Render/RendererConstants.h"

#include "Timer.hpp"

namespace Audio = Hell::Audio;

namespace Unloved::Renderer {

    std::vector<bool> g_freeWoundMaskIndices;

    bool g_gameIsRendering = false;

    void Init() {
        if (Hell::BackEnd::GetAPI() == API::OPENGL) {
            OpenGLRenderer::Init();
        }
        else if (Hell::BackEnd::GetAPI() == API::VULKAN) {
            Logging::ToDo() << "Vulkan TODO: Renderer::Init()";
        }
    }

    void CleanUp() {
        if (Hell::BackEnd::GetAPI() == API::OPENGL) {
            OpenGLRenderer::CleanUp();
        }
        else if (Hell::BackEnd::GetAPI() == API::VULKAN) {
            Logging::ToDo() << "Vulkan TODO: Renderer::CleanUp()";
        }
    }

    void InitMain() {
        if (Hell::BackEnd::GetAPI() == API::OPENGL) {
            OpenGLRenderer::InitMain();
        }
        else if (Hell::BackEnd::GetAPI() == API::VULKAN) {
            Logging::ToDo() << "Vulkan TODO: Renderer::InitMain()";
        }

        UploadVertexData();
        InitWoundMaskArray();
    }

    void RenderLoadingScreen() {
        if (Hell::BackEnd::GetAPI() == API::OPENGL) {
            OpenGLRenderer::RenderLoadingScreen();
        }
        else if (Hell::BackEnd::GetAPI() == API::VULKAN) {
            Logging::ToDo() << "Vulkan: RenderLoadingScreen()";
        }
    }

    void PreGameLogicComputePasses() {
        if (Hell::BackEnd::GetAPI() == API::OPENGL) {
            OpenGLRenderer::PreGameLogicComputePasses();
        }
        else if (Hell::BackEnd::GetAPI() == API::VULKAN) {
            Logging::ToDo() << "Vulkan: PreGameLogicComputePasses()";
        }
    }

    void RenderGame() {
        g_gameIsRendering = true;

        if (Hell::BackEnd::GetAPI() == API::OPENGL) {
            OpenGLRenderer::RenderGame();
        }
        else if (Hell::BackEnd::GetAPI() == API::VULKAN) {
            Logging::ToDo() << "Vulkan: RenderGame()";
        }
    }

	void HotloadShaders() {
		Audio::PlayAudio(AUDIO_SELECT, 1.00f);

        if (Hell::BackEnd::GetAPI() == API::OPENGL) {
            OpenGL::UnbindShader();
            OpenGL::ResourceManager::HotloadShaders();
        }
        else if (Hell::BackEnd::GetAPI() == API::VULKAN) {
            Logging::ToDo() << "Vulkan: HotloadShaders()";
        }
    }

    void RecalculateAllHeightMapData(bool blitWorldMap) {
        if (Hell::BackEnd::GetAPI() == API::OPENGL) {
            OpenGLRenderer::RecalculateAllHeightMapData(blitWorldMap);
        }
        else if (Hell::BackEnd::GetAPI() == API::VULKAN) {
            Logging::ToDo() << "Vulkan: RecalculateAllHeightMapData()";
        }
    }

    void ReadBackHeightMapData(Unloved::MapData* mapData) {
        if (Hell::BackEnd::GetAPI() == API::OPENGL) {
            OpenGLRenderer::ReadBackHeightMapData(mapData);
        }
        else if (Hell::BackEnd::GetAPI() == API::VULKAN) {
            Logging::ToDo() << "Vulkan: ReadBackHeightMapData()";
        }
    }

    int32_t GetNextFreeWoundMaskIndexAndMarkItTaken() {
        for (int i = 0; i < g_freeWoundMaskIndices.size(); i++) {
            if (g_freeWoundMaskIndices[i] == true) {
                g_freeWoundMaskIndices[i] = false;
                return i;
            }
        }

        // Should never happen, unless you ran out of array levels, in which case you need to increase the size of the array
        for (int i = 0; i < g_freeWoundMaskIndices.size(); i++) {
            Logging::Error() << "GetNextFreeWoundMaskIndexAndMarkItTaken() failed because you ran out of free wound mask textures";
            std::cout << i << ": " << g_freeWoundMaskIndices[i] << "\n";
        }
        return -1;
    }

    void UploadVertexData() {
        if (Hell::BackEnd::GetAPI() == API::OPENGL) {
            OpenGLRenderer::UploadVertexWeights();
        }
        else if (Hell::BackEnd::GetAPI() == API::VULKAN) {
            Logging::ToDo() << "Vulkan path for Renderer::UploadVertexData()";
        }
    }

    void InitWoundMaskArray() {
        // Create and init all wound mask indices to true, aka available
        g_freeWoundMaskIndices.assign(WOUND_MASK_TEXTURE_ARRAY_SIZE, true);
        if (Hell::BackEnd::GetAPI() == API::OPENGL) {
            OpenGLRenderer::ClearAllWoundMasks();
        }
        else if (Hell::BackEnd::GetAPI() == API::VULKAN) {
            Logging::ToDo() << "Vulkan: InitWoundMaskArray()";
        }
    }

    void MarkWoundMaskIndexAsAvailable(int32_t index) {
        if (index < 0 || index >= g_freeWoundMaskIndices.size()) {
            Logging::Error() << "Renderer::MarkWoundMaskIndexAsAvailable() failed. Index '" << index << "' is out of range of size '" << g_freeWoundMaskIndices.size() << "'";
            return;
        }
        g_freeWoundMaskIndices[index] = true;
    }

    const std::string& GetZoneNames() {
        if (Hell::BackEnd::GetAPI() == API::OPENGL) {
            return OpenGLRenderer::GetZoneNames();
        }
        else if (Hell::BackEnd::GetAPI() == API::VULKAN) {
            Logging::ToDo() << "Vulkan: GetZoneNames()";
        }

        static std::string empty = "";
        return empty;
    }

    const std::string& GetZoneGPUTimings() {
        if (Hell::BackEnd::GetAPI() == API::OPENGL) {
            return OpenGLRenderer::GetZoneGPUTimings();
        }
        else if (Hell::BackEnd::GetAPI() == API::VULKAN) {
            Logging::ToDo() << "Vulkan: GetZoneGPUTimings()";
        }

        static std::string empty = "";
        return empty;
    }

    const std::string& GetZoneCPUTimings() {
        if (Hell::BackEnd::GetAPI() == API::OPENGL) {
            return OpenGLRenderer::GetZoneCPUTimings();
        }
        else if (Hell::BackEnd::GetAPI() == API::VULKAN) {
            Logging::ToDo() << "Vulkan: GetZoneCPUTimings()";
        }

        static std::string empty = "";
        return empty;
    }

    const std::string& GetTotalGPUTime() {
        if (Hell::BackEnd::GetAPI() == API::OPENGL) {
            return OpenGLRenderer::GetTotalGPUTime();
        }
        else if (Hell::BackEnd::GetAPI() == API::VULKAN) {
            Logging::ToDo() << "Vulkan: GetTotalGPUTime()";
        }

        static std::string empty = "";
        return empty;
    }

    const std::string& GetTotalCPUTime() {
        if (Hell::BackEnd::GetAPI() == API::OPENGL) {
            return OpenGLRenderer::GetTotalCPUTime();
        }
        else if (Hell::BackEnd::GetAPI() == API::VULKAN) {
            Logging::ToDo() << "Vulkan: GetTotalGPUTime()";
        }

        static std::string empty = "";
        return empty;
    }

    uint32_t GetTileCount() {
        return GetTileCountX() * GetTileCountY();
    }

    uint32_t GetTileCountX() {
        const Resolutions& resolutions = Config::GetResolutions();
        return (resolutions.gBuffer.x + TILE_SIZE - 1) / TILE_SIZE;
    }

    uint32_t GetTileCountY() {
		const Resolutions& resolutions = Config::GetResolutions();
		return (resolutions.gBuffer.y + TILE_SIZE - 1) / TILE_SIZE;
    }

    bool GameIsRendering() {
        return g_gameIsRendering;
    }
}
