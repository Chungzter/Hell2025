#include "Renderer.h"
#include "Audio/Audio.h"
#include "API/OpenGL/GL_backend.h"
#include "API/OpenGL/Renderer/GL_renderer.h"
#include "API/Vulkan/Renderer/VK_renderer.h"
#include "AssetManagement/AssetManager.h"
#include "BackEnd/BackEnd.h"
#include "Config/Config.h"
#include "Core/Debug.h"
#include "Editor/Editor.h"
#include "Editor/Editor.h"
#include "Timer.hpp"

#include <Hell/Logging.h>

namespace Renderer {

    std::vector<bool> g_freeWoundMaskIndices;
    MeshBuffer g_proceduralMeshBuffer;

    bool g_gameIsRendering = false;

    void InitMain() {
        if (BackEnd::GetAPI() == API::OPENGL) {
            OpenGLRenderer::InitMain();
        }
        else if (BackEnd::GetAPI() == API::VULKAN) {
            Logging::ToDo() << "Vulkan TODO: Renderer::InitMain()";
        }

        //g_rendererSettingsSet.houseEditor.rendererOverrideState = RendererOverrideState::CAMERA_NDOTL;
        //g_rendererSettingsSet.houseEditor.drawGrass = false;
        //
        //g_rendererSettingsSet.mapHeightEditor.rendererOverrideState = RendererOverrideState::CAMERA_NDOTL;
        //g_rendererSettingsSet.mapHeightEditor.drawGrass = false;
        //
        //g_rendererSettingsSet.mapObjectEditor.rendererOverrideState = RendererOverrideState::CAMERA_NDOTL;
        //g_rendererSettingsSet.mapObjectEditor.drawGrass = true;
    }

    void RenderLoadingScreen() {
        if (BackEnd::GetAPI() == API::OPENGL) {
            OpenGLRenderer::RenderLoadingScreen();
        }
        else if (BackEnd::GetAPI() == API::VULKAN) {
            VulkanRenderer::RenderLoadingScreen();
        }
    }

    void PreGameLogicComputePasses() {
        if (BackEnd::GetAPI() == API::OPENGL) {
            OpenGLRenderer::PreGameLogicComputePasses();
        }
        else if (BackEnd::GetAPI() == API::VULKAN) {
            Logging::ToDo() << "Vulkan: PreGameLogicComputePasses()";
        }
    }

    void RenderGame() {
        g_gameIsRendering = true;

        if (BackEnd::GetAPI() == API::OPENGL) {
            OpenGLRenderer::RenderGame();
        }
        else if (BackEnd::GetAPI() == API::VULKAN) {
            Logging::ToDo() << "Vulkan: RenderGame()";
        }
    }

	void HotloadShaders() {
		Audio::PlayAudio(AUDIO_SELECT, 1.00f);

        if (BackEnd::GetAPI() == API::OPENGL) {
            OpenGLRenderer::HotloadShaders();
        }
        else if (BackEnd::GetAPI() == API::VULKAN) {
            Logging::ToDo() << "Vulkan: HotloadShaders()";
        }
    }

    void RecalculateAllHeightMapData(bool blitWorldMap) {
        if (BackEnd::GetAPI() == API::OPENGL) {
            OpenGLRenderer::RecalculateAllHeightMapData(blitWorldMap);
        }
        else if (BackEnd::GetAPI() == API::VULKAN) {
            Logging::ToDo() << "Vulkan: RecalculateAllHeightMapData()";
        }
    }

    void DrawPoint(const glm::vec3& position, const glm::vec4& color, bool obeyDepth, int exclusiveViewportIndex) {
        if (BackEnd::GetAPI() == API::OPENGL) {
            OpenGLRenderer::DrawPoint(position, color, obeyDepth, exclusiveViewportIndex);
        }
        else if (BackEnd::GetAPI() == API::VULKAN) {
            Logging::ToDo() << "Vulkan: DrawPoint()";
        }
    }

    void DrawLine(const glm::vec3& begin, const glm::vec3& end, const glm::vec4& color, bool obeyDepth, int exclusiveViewportIndex, int ignoredViewportIndex) {
        if (BackEnd::GetAPI() == API::OPENGL) {
            OpenGLRenderer::DrawLine(begin, end, color, obeyDepth, exclusiveViewportIndex, ignoredViewportIndex);
        }
        else if (BackEnd::GetAPI() == API::VULKAN) {
            Logging::ToDo() << "Vulkan: DrawLine()";
        }
    }

    void DrawItemExamineLine(const glm::vec3& begin, const glm::vec3& end, const glm::vec4& color) {
        if (BackEnd::GetAPI() == API::OPENGL) {
            OpenGLRenderer::DrawItemExamineLine(begin, end, color);
        }
        else if (BackEnd::GetAPI() == API::VULKAN) {
            Logging::ToDo() << "Vulkan: DrawItemExamineLine()";
        }
    }

    void DrawLine2D(const glm::ivec2& begin, const glm::ivec2& end, const glm::vec4& color) {
        if (BackEnd::GetAPI() == API::OPENGL) {
            OpenGLRenderer::DrawLine2D(begin, end, color);
        }
        else if (BackEnd::GetAPI() == API::VULKAN) {
            Logging::ToDo() << "Vulkan: DrawLine2D()";
        }
    }

    void DrawAABB(const AABB& aabb, const glm::vec4& color) {
        if (BackEnd::GetAPI() == API::OPENGL) {
            OpenGLRenderer::DrawAABB(aabb, color);
        }
        else if (BackEnd::GetAPI() == API::VULKAN) {
            Logging::ToDo() << "Vulkan: DrawAABB()";
        }
    }

    void DrawItemExamineAABB(const AABB& aabb, const glm::vec4& color) {
        if (BackEnd::GetAPI() == API::OPENGL) {
            OpenGLRenderer::DrawItemExamineAABB(aabb, color);
        }
        else if (BackEnd::GetAPI() == API::VULKAN) {
            Logging::ToDo() << "Vulkan: DrawItemExamineAABB()";
        }
    }

    void DrawAABB(const AABB& aabb, const glm::vec4& color, const glm::mat4& worldTransform) {
        if (BackEnd::GetAPI() == API::OPENGL) {
            OpenGLRenderer::DrawAABB(aabb, color, worldTransform);
        }
        else if (BackEnd::GetAPI() == API::VULKAN) {
            Logging::ToDo() << "Vulkan: DrawAABB()";
        }
    }

    void DrawOBB(const OBB& obb, const glm::vec4& color) {
        if (BackEnd::GetAPI() == API::OPENGL) {
            OpenGLRenderer::DrawOBB(obb, color);
        }
        else if (BackEnd::GetAPI() == API::VULKAN) {
            Logging::ToDo() << "Vulkan: DrawOBB()";
        }
    }

    void DrawFrustum(const Frustum& frustum, const glm::vec4& color) {
        if (BackEnd::GetAPI() == API::OPENGL) {
            OpenGLRenderer::DrawFrustum(frustum, color);
        }
        else if (BackEnd::GetAPI() == API::VULKAN) {
            Logging::ToDo() << "Vulkan: DrawFrustum()";
        }
    }

    void DrawSphere(const glm::vec3& position, float radius, const glm::vec4& color) {
        if (BackEnd::GetAPI() == API::OPENGL) {
            OpenGLRenderer::DrawSphere(position, radius, color);
        }
        else if (BackEnd::GetAPI() == API::VULKAN) {
            Logging::ToDo() << "Vulkan: DrawSphere()";
        }
    }

    void ReadBackHeightMapData(Map* map) {
        if (BackEnd::GetAPI() == API::OPENGL) {
            OpenGLRenderer::ReadBackHeightMapData(map);
        }
        else if (BackEnd::GetAPI() == API::VULKAN) {
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
        if (BackEnd::GetAPI() == API::OPENGL) {
            OpenGLBackEnd::CleanUpBakingPBOs();
            OpenGLBackEnd::UploadVertexData(AssetManager::GetVertices(), AssetManager::GetIndices());
            OpenGLRenderer::UploadVertexWeights();
        }
        else if (BackEnd::GetAPI() == API::VULKAN) {
            Logging::ToDo() << "Vulkan path for Renderer::UploadVertexData()";
        }
    }

    void InitWoundMaskArray() {
        // Create and init all wound mask indices to true, aka available
        g_freeWoundMaskIndices.assign(WOUND_MASK_TEXTURE_ARRAY_SIZE, true);
        if (BackEnd::GetAPI() == API::OPENGL) {
            OpenGLRenderer::ClearAllWoundMasks();
        }
        else if (BackEnd::GetAPI() == API::VULKAN) {
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
        if (BackEnd::GetAPI() == API::OPENGL) {
            return OpenGLRenderer::GetZoneNames();
        }
        else if (BackEnd::GetAPI() == API::VULKAN) {
            Logging::ToDo() << "Vulkan: GetZoneNames()";
        }

        static std::string empty = "";
        return empty;
    }

    const std::string& GetZoneGPUTimings() {
        if (BackEnd::GetAPI() == API::OPENGL) {
            return OpenGLRenderer::GetZoneGPUTimings();
        }
        else if (BackEnd::GetAPI() == API::VULKAN) {
            Logging::ToDo() << "Vulkan: GetZoneGPUTimings()";
        }

        static std::string empty = "";
        return empty;
    }

    const std::string& GetZoneCPUTimings() {
        if (BackEnd::GetAPI() == API::OPENGL) {
            return OpenGLRenderer::GetZoneCPUTimings();
        }
        else if (BackEnd::GetAPI() == API::VULKAN) {
            Logging::ToDo() << "Vulkan: GetZoneCPUTimings()";
        }

        static std::string empty = "";
        return empty;
    }

    const std::string& GetTotalGPUTime() {
        if (BackEnd::GetAPI() == API::OPENGL) {
            return OpenGLRenderer::GetTotalGPUTime();
        }
        else if (BackEnd::GetAPI() == API::VULKAN) {
            Logging::ToDo() << "Vulkan: GetTotalGPUTime()";
        }

        static std::string empty = "";
        return empty;
    }

    const std::string& GetTotalCPUTime() {
        if (BackEnd::GetAPI() == API::OPENGL) {
            return OpenGLRenderer::GetTotalCPUTime();
        }
        else if (BackEnd::GetAPI() == API::VULKAN) {
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

    MeshBuffer& GetProceduralMeshBuffer() {
        return g_proceduralMeshBuffer;
    }

    Mesh* GetProcedualMeshByMeshId(uint64_t meshId) {
        return g_proceduralMeshBuffer.GetMeshById(meshId);
    }

    uint64_t AddProcedualMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, const std::string& name) {
        return g_proceduralMeshBuffer.AddMesh(vertices, indices, name);
    }

    void RemoveProcedualMeshByMeshId(uint64_t meshId) {
        g_proceduralMeshBuffer.RemoveMesh(meshId);
    }

    void ResetProcedualMeshBuffer() {
        g_proceduralMeshBuffer.Reset();
    }
}