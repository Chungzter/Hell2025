#include "../GL_renderer.h"
#include "Editor/Editor.h"
#include "Viewport/ViewportManager.h"
#include "Renderer/RenderDataManager.h"
#include "Renderer/Renderer.h"
#include "Physics/Physics.h"
#include "World/World.h"

#include "API/OpenGL/Types/GL_mesh.h"
#include "API/OpenGL/Types/GL_mesh_buffer.h"

#include "Pathfinding/AStarMap.h"

#include "Core/Game.h"

#include "Input/Input.h"


#include "Types/Renderer/MeshBufferV2.h"

namespace OpenGLRenderer {
    OpenGLMesh g_debugMeshPoints2D;
    OpenGLMesh g_debugMeshPoints3D;
    OpenGLMesh g_debugMeshLines2D;
    OpenGLMesh g_debugMeshLines3D;
    OpenGLMesh g_debugMeshItemExamineLines;

    inline int Index1D(int x, int y, int mapWidth) { return y * mapWidth + x; }

    void RenderAStarDebugMesh();

    void MeshBufferDebugPass() {
        return;

        const std::vector<ViewportData>& viewportData = RenderDataManager::GetViewportData();

        OpenGLShader* shader3D = GetShaderOLD("DebugSolidColor");
        OpenGLFrameBuffer* gBuffer = GetFrameBufferOLD("GBuffer");

        //static MeshBufferV2 meshBuffer;
        //static bool runOnce = true;
        //static std::vector<uint64_t> meshIds;
        //
        //if (runOnce) {
        //    runOnce = false;
        //
        //    std::vector<Vertex> vertices = Util::GenerateCubeVertices();
        //    std::vector<uint32_t> indices = Util::GenerateCubeIndices();
        //
        //    uint64_t meshId = meshBuffer.AddMesh(vertices, indices);
        //    meshIds.push_back(meshId);
        //
        //    std::cout << "\nAdded cube\n";
        //    meshBuffer.PrintDebugInfo();
        //}
        //
        //if (Input::KeyPressed(HELL_KEY_U)) {
        //    static uint32_t count = 0;
        //    count++;
        //
        //    std::vector<Vertex> vertices = Util::GenerateCubeVertices();
        //    std::vector<uint32_t> indices = Util::GenerateCubeIndices();
        //
        //    for (Vertex& vertex : vertices) {
        //        vertex.position.z += (1.5f * count);
        //    }
        //
        //    uint64_t meshId = meshBuffer.AddMesh(vertices, indices);
        //    meshIds.push_back(meshId);
        //
        //    std::cout << "\nAdded cube\n";
        //    meshBuffer.PrintDebugInfo();
        //}
        //
        //if (Input::KeyPressed(HELL_KEY_J) && meshBuffer.GetMeshCount()) {
        //    int meshIdIndex = Util::RandomInt(0, meshIds.size() - 1);
        //    uint64_t meshId = meshIds[meshIdIndex];
        //    meshBuffer.RemoveMesh(meshId);
        //    meshIds.erase(meshIds.begin() + meshIdIndex);
        //
        //    std::cout << "\nRemoved random cube\n";
        //    meshBuffer.PrintDebugInfo();
        //}

        MeshBufferV2& meshBuffer = Renderer::GetProceduralMeshBuffer();

        // Draw
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glDisable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);

        shader3D->Bind();
        gBuffer->Bind();
        gBuffer->DrawBuffer("Lighting");

        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGLRenderer::SetViewport(gBuffer, viewport);
            shader3D->SetMat4("u_projectionView", viewportData[i].projectionView);
            shader3D->SetMat4("u_model", glm::mat4(1.0f));
            shader3D->SetVec3("u_color", YELLOW);

            //for (uint64_t meshId : meshIds) {
            for (Wall& wall : World::GetWalls()) {
                for (WallSegment& wallSegment : wall.GetWallSegments()) {
                    uint64_t meshId = wallSegment.GetMeshId();

                    Mesh* mesh = meshBuffer.GetMeshById(meshId);
                    if (!mesh || mesh->indexCount == 0) continue;

                    glBindVertexArray(meshBuffer.GetVAO());
                    glDrawElementsBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(mesh->baseIndex * sizeof(uint32_t)), mesh->baseVertex);
                }
            }
        }
    }

    void DebugPass() {
        MeshBufferDebugPass();

        const std::vector<ViewportData>& viewportData = RenderDataManager::GetViewportData();

        OpenGLShader* shader2D = GetShaderOLD("DebugVertex2D");
        OpenGLShader* shader3D = GetShaderOLD("DebugVertex3D");
        OpenGLFrameBuffer* gBuffer = GetFrameBufferOLD("GBuffer");

        if (Renderer::GetRendererMode() == RendererMode::RE_STYLE) {
            gBuffer = GetFrameBufferOLD("GBufferRE");
        }

        if (!gBuffer) return;
        if (!shader2D) return;
        if (!shader3D) return;

        glEnable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glDisable(GL_BLEND);
        glPointSize(8.0f);

        UpdateDebugMesh();

        // 3D
        shader3D->Bind();
        gBuffer->Bind();
        gBuffer->DrawBuffer("Lighting");

        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGLRenderer::SetViewport(gBuffer, viewport);
            shader3D->SetInt("u_viewportIndex", i);
            shader3D->SetMat4("u_projectionView", viewportData[i].projectionViewReverseZ);

            if (g_debugMeshLines3D.GetVertexCount() > 0) {
                glBindVertexArray(g_debugMeshLines3D.GetVAO());
                glDrawArrays(GL_LINES, 0, g_debugMeshLines3D.GetVertexCount());
            }

            if (g_debugMeshPoints3D.GetVertexCount() > 0) {
                glBindVertexArray(g_debugMeshPoints3D.GetVAO());
                glDrawArrays(GL_POINTS, 0, g_debugMeshPoints3D.GetVertexCount());
            }

            // No render item inspect debug lines, but only for player 1
            if (i == 0) {
                if (g_debugMeshItemExamineLines.GetVertexCount() > 0) {
                    Transform cameraTransform;
                    cameraTransform.position = glm::vec3(0, 0, 1.5f);
                    glm::mat4 viewMatrix = glm::inverse(cameraTransform.to_mat4());
                    shader3D->SetInt("u_viewportIndex", i);
                    shader3D->SetMat4("u_projectionView", viewportData[i].projectionReverseZ * viewMatrix);
                    glBindVertexArray(g_debugMeshItemExamineLines.GetVAO());
                    glDrawArrays(GL_LINES, 0, g_debugMeshItemExamineLines.GetVertexCount());
                }
            }
        }

        if (Debug::GetDebugRenderMode() == DebugRenderMode::LIGHTS) {
            RenderAStarDebugMesh();
        }

        // 2D
        gBuffer->SetViewport();
        shader2D->Bind();
        shader2D->SetInt("u_viewportWidth", gBuffer->GetWidth());
        shader2D->SetInt("u_viewportHeight", gBuffer->GetHeight());

        if (g_debugMeshLines2D.GetVertexCount() > 0) {
            glBindVertexArray(g_debugMeshLines2D.GetVAO());
            glDrawArrays(GL_LINES, 0, g_debugMeshLines2D.GetVertexCount());
        }

        if (g_debugMeshPoints2D.GetVertexCount() > 0) {
            glBindVertexArray(g_debugMeshPoints2D.GetVAO());
            glDrawArrays(GL_POINTS, 0, g_debugMeshPoints2D.GetVertexCount());
        }
    }

    void RenderAStarDebugMesh() {
        return;
        const std::vector<ViewportData>& viewportData = RenderDataManager::GetViewportData();
        OpenGLMeshBuffer& debugGridMesh = AStarMap::GetDebugGridMeshBuffer().GetGLMeshBuffer();
        OpenGLMeshBuffer& debugSolidMesh = AStarMap::GetDebugSolidMeshBuffer().GetGLMeshBuffer();

        OpenGLShader* solidColorShader = GetShaderOLD("DebugSolidColor");
        if (!solidColorShader) return;

        solidColorShader->Bind();
        solidColorShader->SetMat4("u_model", glm::mat4(1));
        solidColorShader->SetVec3("u_color", WHITE);

        // Line mesh
        if (debugGridMesh.GetIndexCount() > 0) {
            glDisable(GL_DEPTH_TEST);
            glBindVertexArray(debugGridMesh.GetVAO());
            for (int i = 0; i < 4; i++) {
                Viewport* viewport = ViewportManager::GetViewportByIndex(i);
                if (!viewport->IsVisible()) continue;

                solidColorShader->SetMat4("u_projectionView", viewportData[i].projectionViewReverseZ);
                glDrawElements(GL_LINES, debugGridMesh.GetIndexCount(), GL_UNSIGNED_INT, 0);
            }
        }

        int mapWidth = AStarMap::GetMapWidth();
        int mapHeight = AStarMap::GetMapHeight();

        std::vector<glm::ivec2> walls = AStarMap::GetWallCells();
        std::vector<glm::ivec2> path;

        if (World::GetKangaroos().size()) {
            Kangaroo& kangaroo = World::GetKangaroos()[0];
            path = kangaroo.GetPath();
        }

        // Render player cell
        //Player* player = Game::GetLocalPlayerByIndex(0);;
        //glm::vec3 position = player->GetCameraPosition();
        //glm::ivec2 playerCoords = AStarMap::GetCellCoordsFromWorldSpacePosition(position);
        //walls.push_back(playerCoords);
        //
        //// Render roo cell
        //Kangaroo& kangaroo = World::GetKangaroos()[0];
        //walls.push_back(kangaroo.GetGridPosition());



        // Solid mesh
        if (debugSolidMesh.GetIndexCount() > 0) {
            //glEnable(GL_DEPTH_TEST);
            glBindVertexArray(debugSolidMesh.GetVAO());

            for (int i = 0; i < 4; i++) {
                Viewport* viewport = ViewportManager::GetViewportByIndex(i);
                if (!viewport->IsVisible()) continue;

                solidColorShader->SetMat4("u_projectionView", viewportData[i].projectionViewReverseZ);
                //glDrawElements(GL_TRIANGLES, debugSolidMesh.GetIndexCount(), GL_UNSIGNED_INT, 0);
                //glDrawElements(GL_POINTS, debugSolidMesh.GetIndexCount(), GL_UNSIGNED_INT, 0);

                solidColorShader->SetBool("u_isPath", true);

                for (glm::ivec2& cell : path) {
                    int cellX = cell.x;
                    int cellY = cell.y;
                    int baseVertex = Index1D(cellX, cellY, mapWidth + 1);
                    int cellID = cellY * mapWidth + cellX;
                    int baseIndex = cellID * 6;
                    void* offset = (void*)(baseIndex * sizeof(uint32_t));
                    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, offset);
                }

                solidColorShader->SetBool("u_isPath", false);
                for (glm::ivec2& cell : walls) {
                    int cellX = cell.x;
                    int cellY = cell.y;
                    int baseVertex = Index1D(cellX, cellY, mapWidth + 1);
                    int cellID = cellY * mapWidth + cellX;
                    int baseIndex = cellID * 6;
                    void* offset = (void*)(baseIndex * sizeof(uint32_t));
                    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, offset);
                }
            }
        }
    }

    void DebugViewPass() {
        RendererSettings& rendererSettings = Renderer::GetCurrentRendererSettings();

        OpenGLFrameBuffer* gBuffer = GetFrameBufferOLD("GBuffer");
        OpenGLFrameBuffer* indirectDiffuseFbo = GetFrameBufferOLD("IndirectDiffuse");
        OpenGLFrameBuffer* miscFullSizeFBO = GetFrameBufferOLD("MiscFullSize");

        if (!gBuffer) return;
        if (!indirectDiffuseFbo) return;
        if (!miscFullSizeFBO) return;

        // Tile based deferred heat map
        if (rendererSettings.rendererOverrideState == RendererOverrideState::TILE_HEATMAP_LIGHTS ||
            rendererSettings.rendererOverrideState == RendererOverrideState::TILE_HEATMAP_BLOOD_DECALS ||
            rendererSettings.rendererOverrideState == RendererOverrideState::TILE_HEATMAP_CHRISTMAS_LIGHTS) {

            OpenGLShader* shader = GetShaderOLD("DebugTileView");
            if (!shader) return;

            shader->Bind();
            shader->SetFloat("u_viewportWidth", gBuffer->GetWidth());
            shader->SetFloat("u_viewportHeight", gBuffer->GetHeight());
            shader->SetInt("u_tileXCount", gBuffer->GetWidth() / TILE_SIZE);
            shader->SetInt("u_tileYCount", gBuffer->GetHeight() / TILE_SIZE);

            int debugMode = -1;
            if (rendererSettings.rendererOverrideState == RendererOverrideState::TILE_HEATMAP_LIGHTS)           debugMode = 0;
            if (rendererSettings.rendererOverrideState == RendererOverrideState::TILE_HEATMAP_BLOOD_DECALS)     debugMode = 1;
            if (rendererSettings.rendererOverrideState == RendererOverrideState::TILE_HEATMAP_CHRISTMAS_LIGHTS) debugMode = 2;

            shader->SetInt("u_debugMode", debugMode);

            BindSSBO(5, "TileLights");
            BindSSBO(6, "TileBloodDecals");
            BindSSBO(7, "TileChristmasLights");

			uint32_t attachmentHandle = 0;

			switch (Renderer::GetRendererMode()) {
			    case RendererMode::OLD_DEFERRED: attachmentHandle = GetFrameBuffer("GBuffer").GetColorAttachmentHandleByName("Lighting"); break;
			    case RendererMode::MSAA:         attachmentHandle = GetFrameBuffer("Resolve").GetColorAttachmentHandleByName("Lighting");      break;
			    case RendererMode::RE_STYLE:     attachmentHandle = GetFrameBuffer("GBufferRE").GetColorAttachmentHandleByName("Lighting");    break;
			}

            BindImageTexture(0, attachmentHandle, GL_READ_WRITE, GL_RGBA16F);

            glDispatchCompute(GetTileCountX(), GetTileCountY(), 1);
            return;
        }

        // Other modes
        if (rendererSettings.rendererOverrideState == RendererOverrideState::BASE_COLOR ||
            rendererSettings.rendererOverrideState == RendererOverrideState::NORMALS ||
            rendererSettings.rendererOverrideState == RendererOverrideState::RMA ||
            rendererSettings.rendererOverrideState == RendererOverrideState::METALIC ||
            rendererSettings.rendererOverrideState == RendererOverrideState::AO ||
            rendererSettings.rendererOverrideState == RendererOverrideState::CAMERA_NDOTL ||
            rendererSettings.rendererOverrideState == RendererOverrideState::ROUGHNESS ||
            rendererSettings.rendererOverrideState == RendererOverrideState::INDIRECT_DIFFUSE ||
            rendererSettings.rendererOverrideState == RendererOverrideState::VELOCITY ||
            rendererSettings.rendererOverrideState == RendererOverrideState::VIS_BUFFER ||
            rendererSettings.rendererOverrideState == RendererOverrideState::DEPTH ||
            rendererSettings.rendererOverrideState == RendererOverrideState::WORLD_POSITION ||
            rendererSettings.rendererOverrideState == RendererOverrideState::EMISSIVE) {

            OpenGLFrameBuffer& waterFrameBuffer = GetFrameBuffer("Water");

            if (Renderer::GetRendererMode() == RendererMode::MSAA) {
				OpenGLShader* shader = GetShaderOLD("DebugViewMSAA");
				if (!shader) return;

				shader->Bind();
				shader->SetFloat("u_brushSize", Editor::GetMapHeightBrushSize());
				shader->SetBool("u_heightMapEditor", (Editor::GetEditorMode() == EditorMode::MAP_HEIGHT_EDITOR) && Editor::IsOpen());

				OpenGLFrameBuffer* msaaFbo = GetFrameBufferOLD("MSAA");
				OpenGLFrameBuffer* resolveFbo = GetFrameBufferOLD("Resolve");
                BindImageTexture(0, resolveFbo->GetColorAttachmentHandleByName("Lighting"), GL_READ_WRITE, GL_RGBA16F);
                BindTextureUnit(1, msaaFbo->GetColorAttachmentHandleByName("BaseColor"));
                BindTextureUnit(2, msaaFbo->GetColorAttachmentHandleByName("Normal"));
                BindTextureUnit(3, msaaFbo->GetColorAttachmentHandleByName("Material"));
                BindTextureUnit(8, indirectDiffuseFbo->GetColorAttachmentHandleByName("Color"));
                // 9 is visibility
                BindTextureUnit(10, waterFrameBuffer.GetColorAttachmentHandleByName("OceanFlags"));
                // 11 is single sampled depth. NA here
                // 12 is single sample emissive. NA here.

				glDispatchCompute(resolveFbo->GetWidth() / TILE_SIZE, resolveFbo->GetHeight() / TILE_SIZE, 1);
			}
			else if (Renderer::GetRendererMode() == RendererMode::RE_STYLE) {
				OpenGLFrameBuffer& gBufferRE = GetFrameBuffer("GBufferRE");
				OpenGLShader& shader = GetShader("DebugViewRE");

				shader.Bind();
				shader.SetFloat("u_brushSize", Editor::GetMapHeightBrushSize());
				shader.SetBool("u_heightMapEditor", (Editor::GetEditorMode() == EditorMode::MAP_HEIGHT_EDITOR) && Editor::IsOpen());

				BindImageTexture(0, gBufferRE.GetColorAttachmentHandleByName("Lighting"), GL_READ_WRITE, GL_RGBA16F);
				BindTextureUnit(1, gBufferRE.GetColorAttachmentHandleByName("BaseColorMetallic"));
				BindTextureUnit(2, gBufferRE.GetColorAttachmentHandleByName("NormalXYRoughnessMisc"));
                BindTextureUnit(3, gBufferRE.GetColorAttachmentHandleByName("VelocityXYOcclusionSubSurface"));
                BindTextureUnit(8, indirectDiffuseFbo->GetColorAttachmentHandleByName("Color"));
                BindTextureUnit(9, gBufferRE.GetColorAttachmentHandleByName("Visibility"));
                // 10 is ocean flags
                BindTextureUnit(11, gBufferRE.GetDepthAttachmentHandle());
                BindTextureUnit(12, gBufferRE.GetColorAttachmentHandleByName("Emissive"));

                glDispatchCompute(gBuffer->GetWidth() / TILE_SIZE, gBuffer->GetHeight() / TILE_SIZE, 1);
			}
			else {
                OpenGLShader* shader = GetShaderOLD("DebugView");
				if (!shader) return;

				shader->Bind();
				shader->SetFloat("u_brushSize", Editor::GetMapHeightBrushSize());
				shader->SetBool("u_heightMapEditor", (Editor::GetEditorMode() == EditorMode::MAP_HEIGHT_EDITOR) && Editor::IsOpen());

                BindImageTexture(0, gBuffer->GetColorAttachmentHandleByName("Lighting"), GL_READ_WRITE, GL_RGBA16F);
                BindTextureUnit(1, gBuffer->GetColorAttachmentHandleByName("BaseColor"));
                BindTextureUnit(2, gBuffer->GetColorAttachmentHandleByName("Normal"));
                BindTextureUnit(3, gBuffer->GetColorAttachmentHandleByName("RMA"));
                BindTextureUnit(4, gBuffer->GetColorAttachmentHandleByName("VelocityOcclusionSubSurface"));
                // 7 may be free???
                BindTextureUnit(8, indirectDiffuseFbo->GetColorAttachmentHandleByName("Color"));
                // 9 is visibility
                // 10 is ocean flags
                BindTextureUnit(11, gBuffer->GetDepthAttachmentHandle());
                BindTextureUnit(12, gBuffer->GetColorAttachmentHandleByName("Emissive"));

				glDispatchCompute(gBuffer->GetWidth() / TILE_SIZE, gBuffer->GetHeight() / TILE_SIZE, 1);
            }

        }
    }

    void DrawLine(const glm::vec3& begin, const glm::vec3& end, const glm::vec4& color, bool depthEnabled, int exclusiveViewportIndex, int ignoredViewportIndex) {
        DebugVertex3D v0 = DebugVertex3D(begin, color, glm::ivec2(0, 0), int(depthEnabled), exclusiveViewportIndex);
        DebugVertex3D v1 = DebugVertex3D(end, color, glm::ivec2(0, 0), int(depthEnabled), exclusiveViewportIndex);
        g_lines3D.push_back(v0);
        g_lines3D.push_back(v1);
    }

    void DrawItemExamineLine(const glm::vec3& begin, const glm::vec3& end, const glm::vec4& color) {
        bool depthEnabled = true;
        int exclusiveViewportIndex = -1;
        DebugVertex3D v0 = DebugVertex3D(begin, color, glm::ivec2(0, 0), int(depthEnabled), exclusiveViewportIndex);
        DebugVertex3D v1 = DebugVertex3D(end, color, glm::ivec2(0, 0), int(depthEnabled), exclusiveViewportIndex);
        g_itemExaminelines.push_back(v0);
        g_itemExaminelines.push_back(v1);
    }


    void DrawLine2D(const glm::ivec2& begin, const glm::ivec2& end, const glm::vec4& color) {
        g_lines2D.emplace_back(DebugVertex2D(begin, color));
        g_lines2D.emplace_back(DebugVertex2D(end, color));
    }

    void DrawPoint(const glm::vec3& position, const glm::vec4& color, bool depthEnabled, int exclusiveViewportIndex) {
        g_points3D.push_back(DebugVertex3D(position, color, glm::ivec2(0, 0), int(depthEnabled), exclusiveViewportIndex));
    }

    void DrawPoint2D(const glm::ivec2& position, const glm::vec4& color) {
        g_points2D.emplace_back(DebugVertex2D(position, color));
    }

    void DrawAABB(const AABB& aabb, const glm::vec4& color) {
        glm::vec3 FrontTopLeft = glm::vec3(aabb.GetBoundsMin().x, aabb.GetBoundsMax().y, aabb.GetBoundsMax().z);
        glm::vec3 FrontTopRight = glm::vec3(aabb.GetBoundsMax().x, aabb.GetBoundsMax().y, aabb.GetBoundsMax().z);
        glm::vec3 FrontBottomLeft = glm::vec3(aabb.GetBoundsMin().x, aabb.GetBoundsMin().y, aabb.GetBoundsMax().z);
        glm::vec3 FrontBottomRight = glm::vec3(aabb.GetBoundsMax().x, aabb.GetBoundsMin().y, aabb.GetBoundsMax().z);
        glm::vec3 BackTopLeft = glm::vec3(aabb.GetBoundsMin().x, aabb.GetBoundsMax().y, aabb.GetBoundsMin().z);
        glm::vec3 BackTopRight = glm::vec3(aabb.GetBoundsMax().x, aabb.GetBoundsMax().y, aabb.GetBoundsMin().z);
        glm::vec3 BackBottomLeft = glm::vec3(aabb.GetBoundsMin().x, aabb.GetBoundsMin().y, aabb.GetBoundsMin().z);
        glm::vec3 BackBottomRight = glm::vec3(aabb.GetBoundsMax().x, aabb.GetBoundsMin().y, aabb.GetBoundsMin().z);
        DrawLine(FrontTopLeft, FrontTopRight, color);
        DrawLine(FrontBottomLeft, FrontBottomRight, color);
        DrawLine(BackTopLeft, BackTopRight, color);
        DrawLine(BackBottomLeft, BackBottomRight, color);
        DrawLine(FrontTopLeft, FrontBottomLeft, color);
        DrawLine(FrontTopRight, FrontBottomRight, color);
        DrawLine(BackTopLeft, BackBottomLeft, color);
        DrawLine(BackTopRight, BackBottomRight, color);
        DrawLine(FrontTopLeft, BackTopLeft, color);
        DrawLine(FrontTopRight, BackTopRight, color);
        DrawLine(FrontBottomLeft, BackBottomLeft, color);
        DrawLine(FrontBottomRight, BackBottomRight, color);
    }

    void DrawItemExamineAABB(const AABB& aabb, const glm::vec4& color) {
        glm::vec3 FrontTopLeft = glm::vec3(aabb.GetBoundsMin().x, aabb.GetBoundsMax().y, aabb.GetBoundsMax().z);
        glm::vec3 FrontTopRight = glm::vec3(aabb.GetBoundsMax().x, aabb.GetBoundsMax().y, aabb.GetBoundsMax().z);
        glm::vec3 FrontBottomLeft = glm::vec3(aabb.GetBoundsMin().x, aabb.GetBoundsMin().y, aabb.GetBoundsMax().z);
        glm::vec3 FrontBottomRight = glm::vec3(aabb.GetBoundsMax().x, aabb.GetBoundsMin().y, aabb.GetBoundsMax().z);
        glm::vec3 BackTopLeft = glm::vec3(aabb.GetBoundsMin().x, aabb.GetBoundsMax().y, aabb.GetBoundsMin().z);
        glm::vec3 BackTopRight = glm::vec3(aabb.GetBoundsMax().x, aabb.GetBoundsMax().y, aabb.GetBoundsMin().z);
        glm::vec3 BackBottomLeft = glm::vec3(aabb.GetBoundsMin().x, aabb.GetBoundsMin().y, aabb.GetBoundsMin().z);
        glm::vec3 BackBottomRight = glm::vec3(aabb.GetBoundsMax().x, aabb.GetBoundsMin().y, aabb.GetBoundsMin().z);
        DrawItemExamineLine(FrontTopLeft, FrontTopRight, color);
        DrawItemExamineLine(FrontBottomLeft, FrontBottomRight, color);
        DrawItemExamineLine(BackTopLeft, BackTopRight, color);
        DrawItemExamineLine(BackBottomLeft, BackBottomRight, color);
        DrawItemExamineLine(FrontTopLeft, FrontBottomLeft, color);
        DrawItemExamineLine(FrontTopRight, FrontBottomRight, color);
        DrawItemExamineLine(BackTopLeft, BackBottomLeft, color);
        DrawItemExamineLine(BackTopRight, BackBottomRight, color);
        DrawItemExamineLine(FrontTopLeft, BackTopLeft, color);
        DrawItemExamineLine(FrontTopRight, BackTopRight, color);
        DrawItemExamineLine(FrontBottomLeft, BackBottomLeft, color);
        DrawItemExamineLine(FrontBottomRight, BackBottomRight, color);
    }

    void DrawAABB(const AABB& aabb, const glm::vec4& color, const glm::mat4& worldTransform) {
        glm::vec3 FrontTopLeft = worldTransform * glm::vec4(aabb.GetBoundsMin().x, aabb.GetBoundsMax().y, aabb.GetBoundsMax().z, 1.0f);
        glm::vec3 FrontTopRight = worldTransform * glm::vec4(aabb.GetBoundsMax().x, aabb.GetBoundsMax().y, aabb.GetBoundsMax().z, 1.0f);
        glm::vec3 FrontBottomLeft = worldTransform * glm::vec4(aabb.GetBoundsMin().x, aabb.GetBoundsMin().y, aabb.GetBoundsMax().z, 1.0f);
        glm::vec3 FrontBottomRight = worldTransform * glm::vec4(aabb.GetBoundsMax().x, aabb.GetBoundsMin().y, aabb.GetBoundsMax().z, 1.0f);
        glm::vec3 BackTopLeft = worldTransform * glm::vec4(aabb.GetBoundsMin().x, aabb.GetBoundsMax().y, aabb.GetBoundsMin().z, 1.0f);
        glm::vec3 BackTopRight = worldTransform * glm::vec4(aabb.GetBoundsMax().x, aabb.GetBoundsMax().y, aabb.GetBoundsMin().z, 1.0f);
        glm::vec3 BackBottomLeft = worldTransform * glm::vec4(aabb.GetBoundsMin().x, aabb.GetBoundsMin().y, aabb.GetBoundsMin().z, 1.0f);
        glm::vec3 BackBottomRight = worldTransform * glm::vec4(aabb.GetBoundsMax().x, aabb.GetBoundsMin().y, aabb.GetBoundsMin().z, 1.0f);
        DrawLine(FrontTopLeft, FrontTopRight, color);
        DrawLine(FrontBottomLeft, FrontBottomRight, color);
        DrawLine(BackTopLeft, BackTopRight, color);
        DrawLine(BackBottomLeft, BackBottomRight, color);
        DrawLine(FrontTopLeft, FrontBottomLeft, color);
        DrawLine(FrontTopRight, FrontBottomRight, color);
        DrawLine(BackTopLeft, BackBottomLeft, color);
        DrawLine(BackTopRight, BackBottomRight, color);
        DrawLine(FrontTopLeft, BackTopLeft, color);
        DrawLine(FrontTopRight, BackTopRight, color);
        DrawLine(FrontBottomLeft, BackBottomLeft, color);
        DrawLine(FrontBottomRight, BackBottomRight, color);
    }

    void DrawOBB(const OBB& obb, const glm::vec4& color) {
        const std::vector<glm::vec3>& corners = obb.GetCorners();
        if (corners.size() < 8) return;

        // Bottom Face
        DrawLine(corners[0], corners[1], color);
        DrawLine(corners[1], corners[5], color);
        DrawLine(corners[5], corners[4], color);
        DrawLine(corners[4], corners[0], color);

        // Top Face
        DrawLine(corners[2], corners[3], color);
        DrawLine(corners[3], corners[7], color);
        DrawLine(corners[7], corners[6], color);
        DrawLine(corners[6], corners[2], color);

        DrawLine(corners[0], corners[2], color); // Front Left
        DrawLine(corners[1], corners[3], color); // Front Right
        DrawLine(corners[4], corners[6], color); // Back Left
        DrawLine(corners[5], corners[7], color); // Back Right
    }

    void DrawFrustum(const Frustum& frustum, const glm::vec4& color) {
        glm::vec3 ntl = frustum.GetCorner(0);
        glm::vec3 ntr = frustum.GetCorner(1);
        glm::vec3 nbl = frustum.GetCorner(2);
        glm::vec3 nbr = frustum.GetCorner(3);

        glm::vec3 ftl = frustum.GetCorner(4);
        glm::vec3 ftr = frustum.GetCorner(5);
        glm::vec3 fbl = frustum.GetCorner(6);
        glm::vec3 fbr = frustum.GetCorner(7);

        // near face
        DrawLine(ntl, ntr, color);
        DrawLine(ntr, nbr, color);
        DrawLine(nbr, nbl, color);
        DrawLine(nbl, ntl, color);

        // far face
        DrawLine(ftl, ftr, color);
        DrawLine(ftr, fbr, color);
        DrawLine(fbr, fbl, color);
        DrawLine(fbl, ftl, color);

        // connect near to far
        DrawLine(ntl, ftl, color);
        DrawLine(ntr, ftr, color);
        DrawLine(nbl, fbl, color);
        DrawLine(nbr, fbr, color);
    }

    void DrawCircle(const glm::vec3& center, float radius, const glm::vec3 axisU, const glm::vec3 axisV, int segments, const glm::vec4& color) {
        const float step = glm::two_pi<float>() / float(segments);
        glm::vec3 prev = center + radius * axisU;
        for (int i = 1; i <= segments; ++i) {
            float a = step * float(i);
            glm::vec3 p = center + radius * (axisU * cos(a) + axisV * sin(a));
            DrawLine(prev, p, color);
            prev = p;
        }
    }

    void DrawSphere(const glm::vec3& position, float radius, const glm::vec4& color) {
        const int numLongitudes = 12;
        const int numLatitudes = 8;
        const int segsPerCircle = 96;

        const glm::vec3 up = glm::vec3(0, 1, 0);
        glm::vec3 right = glm::vec3(1, 0, 0);
        glm::vec3 axisA = glm::normalize(glm::cross(up, right));
        glm::vec3 axisB = glm::normalize(glm::cross(up, axisA));

        // Longitudes
        for (int m = 0; m < numLongitudes; ++m) {
            float theta = glm::two_pi<float>() * float(m) / float(numLongitudes);
            glm::vec3 dir = axisA * cos(theta) + axisB * sin(theta);
            DrawCircle(position, radius, up, dir, segsPerCircle, color);
        }

        // Latitudes
        for (int j = 1; j < numLatitudes; ++j) {
            float t = float(j) / float(numLatitudes);
            float phi = glm::pi<float>() * (t - 0.5f);
            float z = radius * sin(phi);
            float r = radius * cos(phi);
            DrawCircle(position + up * z, r, axisA, axisB, segsPerCircle, color);
        }
    }

    void UpdateDebugMesh() {
        g_debugMeshLines2D.UpdateVertexData(g_lines2D);
        g_debugMeshLines3D.UpdateVertexData(g_lines3D);
        g_debugMeshPoints2D.UpdateVertexData(g_points2D);
        g_debugMeshPoints3D.UpdateVertexData(g_points3D);
        g_debugMeshItemExamineLines.UpdateVertexData(g_itemExaminelines);

        g_lines3D.clear();
        g_lines2D.clear();
        g_points2D.clear();
        g_points3D.clear();
        g_itemExaminelines.clear();
    }

    void RenderPointCloud() {

    }
}
