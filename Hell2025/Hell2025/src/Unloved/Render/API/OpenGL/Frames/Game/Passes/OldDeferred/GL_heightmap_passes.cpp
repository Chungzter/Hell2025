#include "Hell/Logging.h"
#include "Hell/Render/API/OpenGL/GL_resource_manager.h"
#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Hell/Render/API/OpenGL/Types/GL_texture_readback.h"
#include "Unloved/Config/Config.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Editor/Editor.h"
#include "Unloved/Editor/Gizmo.h"
#include "Unloved/Maps/MapData.h"
#include "Unloved/Systems/HeightMap/HeightMap.h"
#include "Unloved/UI/Imgui/ImguiBackEnd.h"
#include "Unloved/Viewport/ViewportManager.h"
#include "World/LegacyWorld.h"

#include "Unloved/Systems/Pathfinding/AStarMap.h"
#include "lodepng/lodepng.h"

#include "Hell/Audio.h"

#include "Hell/Physics/Physics.h"

#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/Input.h"
namespace Input = Hell::Input;


namespace OpenGL::Renderer {
    using namespace Unloved;

    void UploadWorldHeightData();
    void GenerateHeightMapVertexData();
    void GeneratePhysXTextures();
    void DrawHeightMap();

    void RecalculateAllHeightMapData(bool uploadWorldHeightData) {
        if (uploadWorldHeightData) {
            UploadWorldHeightData();
        }
        GenerateHeightMapVertexData();
        GeneratePhysXTextures();
        AStarMap::Init();
        AStarMap::UpdateDebugMeshesFromHeightField();
    }

    void HeightMapPass() {

        //if (Input::KeyPressed(HELL_KEY_SPACE))
        //BlitHeightMapWorld();

        DrawHeightMap();

        if (Editor::IsOpen() && Editor::GetEditorMode() == EditorMode::MAP_HEIGHT_EDITOR) {

            //if (Input::KeyPressed(HELL_KEY_L)) {
            //    HeightMapData heightMapData = File::LoadHeightMap("TEST.heightmap");
            //    OpenGLFrameBuffer* heightmapFBO = OpenGL::ResourceManager::GetFrameBufferPtr("HeightMap");
            //    GLuint textureHandle = heightmapFBO->GetColorAttachmentHandleByName("Color");
            //}
            //if (Input::KeyPressed(HELL_KEY_S)) {
            //    SaveHeightMap();
            //}
        }

        //if (Input::KeyPressed(HELL_KEY_U)) {
        //    if (Hell::File::Rename("res/shit.txt", "res/fuck.txt")) {
        //        std::cout << "rename successful\n";
        //    }
        //}
    }

    void UploadWorldHeightData() {
        OpenGLFrameBuffer* worldFramebuffer = OpenGL::ResourceManager::GetFrameBufferPtr("World");
        OpenGLFrameBuffer* roadFramebuffer = OpenGL::ResourceManager::GetFrameBufferPtr("Road");
        if (!worldFramebuffer) return;
        if (!roadFramebuffer) return;

        const std::vector<float>& worldHeightData = HeightMap::GetWorldHeightData();
        const uint32_t textureWidth = HeightMap::GetWorldTextureWidth();
        const uint32_t textureHeight = HeightMap::GetWorldTextureHeight();
        if (worldHeightData.empty() || textureWidth == 0 || textureHeight == 0) return;

        // Resize the runtime textures to the assembled world
        if (worldFramebuffer->GetWidth() != textureWidth || worldFramebuffer->GetHeight() != textureHeight) {
            worldFramebuffer->Resize(textureWidth, textureHeight);

            const int roadScale = 4;
            roadFramebuffer->Resize(textureWidth * roadScale, textureHeight * roadScale);
        }

        GLuint heightMapHandle = worldFramebuffer->GetColorAttachmentHandleByName("HeightMap");
        glTextureSubImage2D(heightMapHandle, 0, 0, 0, textureWidth, textureHeight, GL_RED, GL_FLOAT, worldHeightData.data());
        glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }

    void PaintHeightMap() {
        if (!Editor::IsOpen()) return;
        if (Editor::GetEditorMode() != EditorMode::MAP_HEIGHT_EDITOR) return;
        if (ImGuiBackEnd::OwnsMouse()) return;

        OpenGLFrameBuffer* worldFramebuffer = OpenGL::ResourceManager::GetFrameBufferPtr("World");
        OpenGLFrameBuffer* gBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");
        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("HeightMapPaint");

        // Bail if no mouse hit with height map
        if (!Editor::HeightMapMouseHitFound()) return;

        if (Input::LeftMouseDown() || Input::RightMouseDown()) {
            OpenGL::BindShader("HeightMapPaint");
            OpenGL::SetUniformVec3("u_mouseHitWorldPos", Editor::GetHeightMapMouseHitPosition());
            OpenGL::SetUniformFloat("u_brushSize", Editor::GetMapHeightBrushSize());
            OpenGL::SetUniformFloat("u_brushStrength", Editor::GetMapHeightBrushStrength() * (Input::RightMouseDown() ? -1.0f : 1.0f));
            OpenGL::SetUniformFloat("u_noiseStrength", Editor::GetMapHeightNoiseStrength());
            OpenGL::SetUniformFloat("u_noiseScale", Editor::GetMapHeightNoiseScale());
            OpenGL::SetUniformFloat("u_minPaintHeight", Editor::GetMapHeightMinPaintHeight());
            OpenGL::SetUniformFloat("u_maxPaintHeight", Editor::GetMapHeightMaxPaintHeight());

            glMemoryBarrier(GL_ALL_BARRIER_BITS);
            glBindImageTexture(0, worldFramebuffer->GetColorAttachmentHandleByName("HeightMap"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_R16F);
            OpenGL::DispatchCompute(worldFramebuffer->GetWidth() / 32, worldFramebuffer->GetHeight() / 32, 1);

            GenerateHeightMapVertexData();
        }
    }

    void GenerateHeightMapVertexData() {
        std::vector<HeightMapChunk>& chunks = LegacyWorld::GetHeightMapChunks();
        if (chunks.empty()) return;

        OpenGLFrameBuffer* worldFramebuffer = OpenGL::ResourceManager::GetFrameBufferPtr("World");
        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("HeightMapVertexGeneration");
        if (!worldFramebuffer) return;
        if (!shader) return;

        Hell::MeshBuffer& heightMapMeshBuffer = Hell::ResourceManager::GetMeshBuffer("HeightMapGeometry");
        OpenGLMeshBuffer& glHeightMapMeshBuffer = OpenGL::ResourceManager::GetMeshBuffer("HeightMapGeometry");

        OpenGL::BindShader("HeightMapVertexGeneration");
        glBindImageTexture(0, worldFramebuffer->GetColorAttachmentHandleByName("HeightMap"), 0, GL_FALSE, 0, GL_READ_ONLY, GL_R16F);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SSBO_IDX_HEIGHTMAP_VERTEX_OUTPUT, glHeightMapMeshBuffer.GetVBO());

        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

        for (HeightMapChunk& chunk : chunks) {
            Mesh* mesh = heightMapMeshBuffer.GetMeshById(chunk.meshId);
            if (!mesh) continue;

            OpenGL::SetUniformInt("u_baseVertex", mesh->baseVertex);
            OpenGL::SetUniformInt("u_chunkX", chunk.coord.x);
            OpenGL::SetUniformInt("u_chunkZ", chunk.coord.z);
            int chunkSize = HEIGHT_MAP_SIZE / 8;
            int chunkWidth = chunkSize + 1;
            int chunkDepth = chunkSize + 1;
            int groupSizeX = (chunkWidth + 16 - 1) / 16;
            int groupSizeY = (chunkDepth + 16 - 1) / 16;
            OpenGL::DispatchCompute(groupSizeX, groupSizeY, 1);
        }

        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
    }

    void GeneratePhysXTextures() {
        OpenGLFrameBuffer* worldFramebuffer = OpenGL::ResourceManager::GetFrameBufferPtr("World");

        GLuint handle = worldFramebuffer->GetColorAttachmentHandleByName("HeightMap");
        GLint level = 0;
        GLint zOffset = 0;
        GLsizei width = 33;
        GLsizei height = 33;
        GLsizei depth = 1;
        GLenum format = GL_RED;
        GLenum type = GL_FLOAT;
        GLsizei numPixels = width * height * depth;
        GLsizei dataSize = numPixels * sizeof(float);
        std::vector<float> pixels(numPixels);

        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

        struct ChunkReadBackData {
            float vertices[VERTICES_PER_CHUNK];
        };

        int chunkCount = LegacyWorld::GetChunkCount();
        std::vector<ChunkReadBackData> chunkReadBackDataSet(chunkCount);

        // Readback height chunk data from gpu
        std::vector<HeightMapChunk>& chunks = LegacyWorld::GetHeightMapChunks();
        for (int i = 0; i < chunkCount; i++) {
            HeightMapChunk& chunk = chunks[i];
            GLint xOffset = chunk.coord.x * 32;
            GLint yOffset = chunk.coord.z * 32;

            if (xOffset + width > worldFramebuffer->GetWidth() ||
                yOffset + height > worldFramebuffer->GetHeight()) {
                std::cout << "YOU HAVE PROBLEMS: \n";
                std::cout << " - worldFramebuffer->GetWidth(): " << worldFramebuffer->GetWidth() << "\n";
                std::cout << " - worldFramebuffer->GetHeight(): " << worldFramebuffer->GetHeight() << "\n";
                std::cout << " - xOffset: " << xOffset << "\n";
                std::cout << " - yOffset: " << yOffset << "\n";
                std::cout << " - width: " << width << "\n";
                std::cout << " - height: " << height << "\n";
                std::cout << " - chunkCount: " << chunkCount << "\n";
            }

            glGetTextureSubImage(handle, level, xOffset, yOffset, zOffset, width, height, depth, GL_RED, GL_FLOAT, dataSize, chunkReadBackDataSet[i].vertices);
        }

        Hell::Physics::MarkAllHeightFieldsForRemoval();

        // For each chunk determine the AABB
        for (int i = 0; i < chunkCount; i++) {
            HeightMapChunk& chunk = chunks[i];
            glm::vec3 aabbMin(std::numeric_limits<float>::max());
            glm::vec3 aabbMax(std::numeric_limits<float>::lowest());

            for (size_t j = 0; j < VERTICES_PER_CHUNK; j++) {
                float x = ((j % 33) + (chunk.coord.x * 32)) * HEIGHTMAP_SCALE_XZ;
                float y = chunkReadBackDataSet[i].vertices[j] * HEIGHTMAP_SCALE_Y;
                float z = ((j / 33) + (chunk.coord.z * 32)) * HEIGHTMAP_SCALE_XZ;

                glm::vec3 position(x, y, z);
                aabbMin = glm::min(aabbMin, position);
                aabbMax = glm::max(aabbMax, position);
            }
            chunk.aabbMin = aabbMin;
            chunk.aabbMax = aabbMax;

            Hell::vecXZ worldSpaceOffest = Hell::vecXZ(chunk.coord.x * HEIGHT_MAP_CHUNK_WORLD_SPACE_SIZE, chunk.coord.z * HEIGHT_MAP_CHUNK_WORLD_SPACE_SIZE);
            Hell::Physics::CreateHeightField(worldSpaceOffest, chunkReadBackDataSet[i].vertices, HEIGHTMAP_SCALE_Y, HEIGHTMAP_SCALE_XZ, HEIGHTMAP_SCALE_XZ);
       }
    }

    void DrawHeightMap() {
        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer* gBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");
        OpenGLFrameBuffer* roadFramebuffer = OpenGL::ResourceManager::GetFrameBufferPtr("Road");
        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("HeightMapColor");

        if (!gBuffer) return;
        if (!roadFramebuffer) return;
        if (!shader) return;

        OpenGLMeshBuffer& glHeightMapMeshBuffer = OpenGL::ResourceManager::GetMeshBuffer("HeightMapGeometry");
        const DrawCommandsSet& drawInfoSet = Unloved::RenderDataManager::GetDrawInfoSet();

        gBuffer->Bind();
        gBuffer->DrawBuffers({ "BaseColorMetallic", "NormalXYRoughnessMisc", "Emissive", "VelocityXYOcclusionSubSurface" });

        OpenGL::BindShader("HeightMapColor");
        OpenGL::BindSSBO(SSBO_IDX_SAMPLERS, "Samplers");
        OpenGL::BindSSBO(SSBO_IDX_MATERIALS, "Materials");
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
        OpenGL::BindSSBO(SSBO_IDX_SCENE_RENDER_ITEMS, "SceneRenderItems");
        OpenGL::BindSSBO(SSBO_IDX_DRAW_RENDER_ITEM_INDICES, "DrawRenderItemIndices");
        OpenGL::SetUniformFloat("u_textureScaling", 1);

        OpenGL::RasterizerStateManager::ForceRasterizerState("GeometryPass_Default");

        Material* dirtRoadMaterial = Hell::ResourceManager::GetMaterialByName("DirtRoad");

        if (Editor::IsOpen() && Editor::GetEditorMode() == EditorMode::MAP_HEIGHT_EDITOR) {
            OpenGL::SetUniformFloat("u_textureScaling", 0.1);
        }

        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(dirtRoadMaterial->m_basecolor)->GetGLTexture().GetHandle());
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(dirtRoadMaterial->m_normal)->GetGLTexture().GetHandle());
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(dirtRoadMaterial->m_rma)->GetGLTexture().GetHandle());
        glBindTextureUnit(6, roadFramebuffer->GetColorAttachmentHandleByName("RoadMask"));

        glBindVertexArray(glHeightMapMeshBuffer.GetVAO());

        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGL::Renderer::SetViewport(gBuffer, viewport);
            OpenGL::SetUniformInt("u_viewportIndex", i);
            MultiDrawIndirect(drawInfoSet.heightMap[i]);
        }
        glBindVertexArray(0);
    }


    void ReadBackHeightMapData(Unloved::MapData* mapData) {
        if (!mapData) {
            Logging::Error() << "OpenGL::Renderer::ReadBackHeightMapData() failed coz mapData was nullptr";
            return;
        }

        OpenGLFrameBuffer* worldFramebuffer = OpenGL::ResourceManager::GetFrameBufferPtr("World");
        if (!worldFramebuffer) {
            Logging::Error() << "OpenGL::Renderer::ReadBackHeightMapData() failed coz could not retrieve World framebuffer";
            return;
        }

        GLuint textureHandle = worldFramebuffer->GetColorAttachmentHandleByName("HeightMap");

        GLuint width = mapData->GetTextureWidth();
        GLuint height = mapData->GetTextureHeight();
        size_t dataSize = width * height * sizeof(float);

        mapData->GetHeightMapData().resize(width * height);

        // Readback
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
        glGetTextureSubImage(textureHandle, 0, 0, 0, 0, width, height, 1, GL_RED, GL_FLOAT, dataSize, mapData->GetHeightMapData().data());

        Logging::Debug() << "ReadBackHeightMapData() width: " << width;
        Logging::Debug() << "ReadBackHeightMapData() height: " << height;
        Logging::Debug() << "ReadBackHeightMapData() dataSize: " << dataSize;
    }
}
