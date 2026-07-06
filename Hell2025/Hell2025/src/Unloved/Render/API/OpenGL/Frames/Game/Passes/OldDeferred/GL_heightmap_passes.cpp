#include "Hell/Logging.h"
#include "Hell/Render/API/OpenGL/GL_back_end.h"
#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Hell/Render/API/OpenGL/Types/GL_heightmap_mesh.h"
#include "Hell/Render/API/OpenGL/Types/GL_texture_readback.h"
#include "Hell/Backend/BackEnd.h"
#include "Unloved/Config/Config.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Editor/Editor.h"
#include "Unloved/Editor/Gizmo.h"
#include "Unloved/UI/Imgui/ImguiBackEnd.h"
#include "Unloved/Viewport/ViewportManager.h"
#include "World/LegacyWorld.h"

#include "Unloved/Systems/Pathfinding/AStarMap.h"
#include "lodepng/lodepng.h"

#include "Hell/Audio.h"

#include "Hell/Physics/Physics.h"

#include "Unloved/Systems/Map/MapManager.h"
#include "World/LegacyWorld.h"

#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/Input.h"
namespace Input = Hell::Input;


namespace OpenGL::Renderer {
    using namespace Unloved;

    void BlitWorldMap();
    void GenerateHeightMapVertexData();
    void GeneratePhysXTextures();
    void DrawHeightMap();

    void RecalculateAllHeightMapData(bool blitWorldMap) {
        if (blitWorldMap) {
            BlitWorldMap();
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

    void BlitWorldMap() {
        OpenGLFrameBuffer* worldFramebuffer = OpenGL::ResourceManager::GetFrameBufferPtr("World");
        OpenGLFrameBuffer* roadFramebuffer = OpenGL::ResourceManager::GetFrameBufferPtr("Road");
        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("HeightMapToWorldBlit");

        if (!shader) return;
        if (!worldFramebuffer) return;

        int textureWidth = (LegacyWorld::GetChunkCountX() * HEIGHT_MAP_CHUNK_PIXEL_SIZE) + 1;
        int textureHeight = (LegacyWorld::GetChunkCountZ() * HEIGHT_MAP_CHUNK_PIXEL_SIZE) + 1;

        const glm::uvec2 textureSize = glm::uvec2(textureWidth, textureHeight);

        // Resize world framebuffer if it is too small for the heightmap
        if (worldFramebuffer->GetWidth() != textureSize.x || worldFramebuffer->GetHeight() != textureSize.y) {
            worldFramebuffer->Resize(textureSize.x, textureSize.y);

            int roadScale = 4;
            roadFramebuffer->Resize(textureSize.x * roadScale, textureSize.y * roadScale);
        }

        // Blit height maps
        OpenGL::BindShader("HeightMapToWorldBlit");
        for (Map& map : LegacyWorld::GetMaps()) {
            MapData* mapData = MapManager::GetMapDataByIndex(map.m_mapIndex);
            if (!mapData) continue;

            int offsetX = map.spawnOffsetChunkX * HEIGHT_MAP_CHUNK_PIXEL_SIZE;
            int offsetZ = map.spawnOffsetChunkZ * HEIGHT_MAP_CHUNK_PIXEL_SIZE;
            int heightMapTextureWidth = map.GetChunkCountX() * HEIGHT_MAP_CHUNK_PIXEL_SIZE;
            int heightMapTextureHeight = map.GetChunkCountZ() * HEIGHT_MAP_CHUNK_PIXEL_SIZE;
            OpenGL::SetUniformInt("u_offsetX", offsetX);
            OpenGL::SetUniformInt("u_offsetZ", offsetZ);
            glBindImageTexture(0, worldFramebuffer->GetColorAttachmentHandleByName("HeightMap"), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R16F);
            glBindImageTexture(1, mapData->GetHeightMapGLTexture().GetHandle(), 0, GL_FALSE, 0, GL_READ_ONLY, GL_R16F);
            OpenGL::DispatchCompute(heightMapTextureWidth / 8, heightMapTextureHeight / 8, 1);
        }

        // Blit roads
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
        OpenGLFrameBuffer* worldFramebuffer = OpenGL::ResourceManager::GetFrameBufferPtr("World");
        OpenGLHeightMapMesh& heightMapMesh = OpenGL::BackEnd::GetHeightMapMesh();
        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("HeightMapVertexGeneration");

        int heightMapWidth = 256;
        int heightMapDepth = 512;

        std::vector<HeightMapChunk>& chunks = LegacyWorld::GetHeightMapChunks();

        heightMapMesh.AllocateMemory(chunks.size());

        OpenGL::BindShader("HeightMapVertexGeneration");
        glBindImageTexture(0, worldFramebuffer->GetColorAttachmentHandleByName("HeightMap"), 0, GL_FALSE, 0, GL_READ_ONLY, GL_R16F);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, heightMapMesh.GetVBO());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, heightMapMesh.GetEBO());

        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

        for (HeightMapChunk& chunk : chunks) {
            OpenGL::SetUniformInt("u_baseIndex", chunk.baseIndex);
            OpenGL::SetUniformInt("u_baseVertex", chunk.baseVertex);
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

        OpenGLHeightMapMesh& heightMapMesh = OpenGL::BackEnd::GetHeightMapMesh();

        Transform transform;
        transform.scale = glm::vec3(HEIGHTMAP_SCALE_XZ, HEIGHTMAP_SCALE_Y, HEIGHTMAP_SCALE_XZ);
        glm::mat4 modelMatrix = transform.to_mat4();
        glm::mat4 inverseModelMatrix = glm::inverse(modelMatrix);

        gBuffer->Bind();
        gBuffer->DrawBuffers({ "BaseColorMetallic", "NormalXYRoughnessMisc", "Emissive", "VelocityXYOcclusionSubSurface" });

        OpenGL::BindShader("HeightMapColor");
        OpenGL::SetUniformMat4("modelMatrix", modelMatrix);
        OpenGL::SetUniformMat4("inverseModelMatrix", inverseModelMatrix);
        OpenGL::SetUniformFloat("u_textureScaling", 1);
        OpenGL::SetUniformFloat("u_worldWidth", LegacyWorld::GetWorldSpaceWidth());
        OpenGL::SetUniformFloat("u_worldDepth", LegacyWorld::GetWorldSpaceDepth());

        OpenGL::RasterizerStateManager::ForceRasterizerState("GeometryPass_Default");

        Material* material = Hell::ResourceManager::GetDefaultMaterial();
        int materialIndex = Hell::ResourceManager::GetMaterialIndexByName("Ground_MudVeg");
        material = Hell::ResourceManager::GetMaterialByIndex(materialIndex);

        Material* dirtRoadMaterial = Hell::ResourceManager::GetMaterialByName("DirtRoad");

        if (Editor::IsOpen() && Editor::GetEditorMode() == EditorMode::MAP_HEIGHT_EDITOR) {
            material = Hell::ResourceManager::GetDefaultMaterial();
            OpenGL::SetUniformFloat("u_textureScaling", 0.1);
        }

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(material->m_basecolor)->GetGLTexture().GetHandle());
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(material->m_normal)->GetGLTexture().GetHandle());
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(material->m_rma)->GetGLTexture().GetHandle());
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(dirtRoadMaterial->m_basecolor)->GetGLTexture().GetHandle());
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(dirtRoadMaterial->m_normal)->GetGLTexture().GetHandle());
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(dirtRoadMaterial->m_rma)->GetGLTexture().GetHandle());;
        glBindTextureUnit(6, roadFramebuffer->GetColorAttachmentHandleByName("RoadMask"));

        glBindVertexArray(heightMapMesh.GetVAO());

        int verticesPerChunk = 33 * 33;
        int verticesPerHeightMap = verticesPerChunk * 8 * 8;
        int indicesPerChunk = 32 * 32 * 6;
        int indicesPerHeightMap = indicesPerChunk * 8 * 8;

        int culled = 0;

        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            Unloved::Frustum& frustum = viewport->GetFrustum();

            int test = 0;
            if (viewport->IsVisible()) {
                OpenGL::Renderer::SetViewport(gBuffer, viewport);
                std::vector<HeightMapChunk>& chunks = LegacyWorld::GetHeightMapChunks();

                //std::cout << "chunks.size(): " << chunks.size() << "\n";

                for (HeightMapChunk& chunk : chunks) {

                    if (Editor::IsClosed()) {
                        if (!frustum.IntersectsAABBFast(AABB(chunk.aabbMin, chunk.aabbMax))) {
                            culled++;
                            continue;
                        }
                    }

                    int indexCount = INDICES_PER_CHUNK;
                    int baseVertex = 0;
                    int baseIndex = chunk.baseIndex;
                    void* indexOffset = (GLvoid*)(baseIndex * sizeof(GLuint));
                    int instanceCount = 1;
                    int viewportIndex = i;
                    glDrawElementsInstancedBaseVertexBaseInstance(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, indexOffset, instanceCount, baseVertex, viewportIndex);
                }
            }
        }
        glBindVertexArray(0);
        //std::cout << "Culled: " << culled << "\n";
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
