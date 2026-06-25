#include "../GL_renderer.h"
#include "Editor/Editor.h"
#include "Viewport/ViewportManager.h"
#include "Renderer/RenderDataManager.h"
#include "Renderer/Renderer.h"
#include "Hell/Physics/Physics.h"
#include "World/LegacyWorld.h"

#include "Debug/DebugDraw.h"
#include "Hell/ResourceManagement/ResourceManager.h"

using namespace Hell;

namespace OpenGLRenderer {

    void DebugPass() {
        const std::vector<ViewportData>& viewportData = RenderDataManager::GetViewportData();

        DebugDraw::UploadVertexData(); // Calling here as the last possible moment to capture all debug geometry submitted within previous render passes

        GenericMesh& genericMeshLines2D = ResourceManager::GetGenericMesh("DebugLines2D");
        GenericMesh& genericMeshLines3D = ResourceManager::GetGenericMesh("DebugLines3D");
        GenericMesh& genericMeshPoints2D = ResourceManager::GetGenericMesh("DebugPoints2D");
        GenericMesh& genericMeshPoints3D = ResourceManager::GetGenericMesh("DebugPoints3D");
        GenericMesh& genericMeshExamineLines2D = ResourceManager::GetGenericMesh("DebugMeshItemExamineLines");

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

            if (genericMeshLines3D.GetVertexCount() > 0) {
                glBindVertexArray(genericMeshLines3D.GetVAO());
                glDrawArrays(GL_LINES, 0, genericMeshLines3D.GetVertexCount());
            }

            if (genericMeshPoints3D.GetVertexCount() > 0) {
                glBindVertexArray(genericMeshPoints3D.GetVAO());
                glDrawArrays(GL_POINTS, 0, genericMeshPoints3D.GetVertexCount());
            }

            // No render item inspect debug lines, but only for player 1
            if (i == 0) {
                if (genericMeshExamineLines2D.GetVertexCount() > 0) {
                    Transform cameraTransform;
                    cameraTransform.position = glm::vec3(0, 0, 1.5f);
                    glm::mat4 viewMatrix = glm::inverse(cameraTransform.to_mat4());
                    shader3D->SetInt("u_viewportIndex", i);
                    shader3D->SetMat4("u_projectionView", viewportData[i].projectionReverseZ * viewMatrix);
                    glBindVertexArray(genericMeshExamineLines2D.GetVAO());
                    glDrawArrays(GL_LINES, 0, genericMeshExamineLines2D.GetVertexCount());
                }
            }
        }

        // 2D
        gBuffer->SetViewport();
        shader2D->Bind();
        shader2D->SetInt("u_viewportWidth", gBuffer->GetWidth());
        shader2D->SetInt("u_viewportHeight", gBuffer->GetHeight());

        if (genericMeshLines2D.GetVertexCount() > 0) {
            glBindVertexArray(genericMeshLines2D.GetVAO());
            glDrawArrays(GL_LINES, 0, genericMeshLines2D.GetVertexCount());
        }

        if (genericMeshPoints2D.GetVertexCount() > 0) {
            glBindVertexArray(genericMeshPoints2D.GetVAO());
            glDrawArrays(GL_POINTS, 0, genericMeshPoints2D.GetVertexCount());
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

            if (Renderer::GetRendererMode() == RendererMode::RE_STYLE) {
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
                BindTextureUnit(1, gBuffer->GetColorAttachmentHandleByName("BaseColorMetallic"));
                BindTextureUnit(2, gBuffer->GetColorAttachmentHandleByName("NormalXYRoughnessMisc"));
                BindTextureUnit(3, gBuffer->GetColorAttachmentHandleByName("VelocityXYOcclusionSubSurface"));
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
}
