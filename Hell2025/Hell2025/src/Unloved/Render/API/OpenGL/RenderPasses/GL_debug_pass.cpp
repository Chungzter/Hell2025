#include "../GL_renderer.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Render/Renderer.h"
#include "Hell/Physics/Physics.h"
#include "World/LegacyWorld.h"

#include "Hell/ResourceManagement/ResourceManager.h"

#include "Unloved/Debug/DebugDraw.h"
#include "Unloved/Editor/Editor.h"
#include "Unloved/Render/RendererTypes.h"
#include "Unloved/Viewport/ViewportManager.h"

using namespace Hell;

namespace OpenGLRenderer {

    void DebugPass() {
        const std::vector<ViewportData>& viewportData = Unloved::RenderDataManager::GetViewportData();

        DebugDraw::UploadVertexData(); // Calling here as the last possible moment to capture all debug geometry submitted within previous render passes

        GenericMesh& genericMeshLines2D = ResourceManager::GetGenericMesh("DebugLines2D");
        GenericMesh& genericMeshLines3D = ResourceManager::GetGenericMesh("DebugLines3D");
        GenericMesh& genericMeshPoints2D = ResourceManager::GetGenericMesh("DebugPoints2D");
        GenericMesh& genericMeshPoints3D = ResourceManager::GetGenericMesh("DebugPoints3D");
        GenericMesh& genericMeshExamineLines2D = ResourceManager::GetGenericMesh("DebugMeshItemExamineLines");

        OpenGLShader* shader2D = OpenGL::ResourceManager::GetShaderPtr("DebugVertex2D");
        OpenGLShader* shader3D = OpenGL::ResourceManager::GetShaderPtr("DebugVertex3D");
        OpenGLFrameBuffer* gBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");

        if (Unloved::Renderer::GetRendererMode() == RendererMode::RE_STYLE) {
            gBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GBufferRE");
        }

        if (!gBuffer) return;
        if (!shader2D) return;
        if (!shader3D) return;

        glEnable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glDisable(GL_BLEND);
        glPointSize(8.0f);

        // 3D
        OpenGL::BindShader("DebugVertex3D");
        gBuffer->Bind();
        gBuffer->DrawBuffer("Lighting");

        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGLRenderer::SetViewport(gBuffer, viewport);
            OpenGL::SetUniformInt("u_viewportIndex", i);
            OpenGL::SetUniformMat4("u_projectionView", viewportData[i].projectionViewReverseZ);

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
                    OpenGL::SetUniformInt("u_viewportIndex", i);
                    OpenGL::SetUniformMat4("u_projectionView", viewportData[i].projectionReverseZ * viewMatrix);
                    glBindVertexArray(genericMeshExamineLines2D.GetVAO());
                    glDrawArrays(GL_LINES, 0, genericMeshExamineLines2D.GetVertexCount());
                }
            }
        }

        // 2D
        gBuffer->SetViewport();
        OpenGL::BindShader("DebugVertex2D");
        OpenGL::SetUniformInt("u_viewportWidth", gBuffer->GetWidth());
        OpenGL::SetUniformInt("u_viewportHeight", gBuffer->GetHeight());

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
        RendererSettings& rendererSettings = Unloved::Renderer::GetCurrentRendererSettings();

        OpenGLFrameBuffer* gBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");
        OpenGLFrameBuffer* indirectDiffuseFbo = OpenGL::ResourceManager::GetFrameBufferPtr("IndirectDiffuse");
        OpenGLFrameBuffer* miscFullSizeFBO = OpenGL::ResourceManager::GetFrameBufferPtr("MiscFullSize");

        if (!gBuffer) return;
        if (!indirectDiffuseFbo) return;
        if (!miscFullSizeFBO) return;

        // Tile based deferred heat map
        if (rendererSettings.rendererOverrideState == RendererOverrideState::TILE_HEATMAP_LIGHTS ||
            rendererSettings.rendererOverrideState == RendererOverrideState::TILE_HEATMAP_BLOOD_DECALS ||
            rendererSettings.rendererOverrideState == RendererOverrideState::TILE_HEATMAP_CHRISTMAS_LIGHTS) {

            OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("DebugTileView");
            if (!shader) return;

            OpenGL::BindShader("DebugTileView");
            OpenGL::SetUniformFloat("u_viewportWidth", gBuffer->GetWidth());
            OpenGL::SetUniformFloat("u_viewportHeight", gBuffer->GetHeight());
            OpenGL::SetUniformInt("u_tileXCount", gBuffer->GetWidth() / TILE_SIZE);
            OpenGL::SetUniformInt("u_tileYCount", gBuffer->GetHeight() / TILE_SIZE);

            int debugMode = -1;
            if (rendererSettings.rendererOverrideState == RendererOverrideState::TILE_HEATMAP_LIGHTS)           debugMode = 0;
            if (rendererSettings.rendererOverrideState == RendererOverrideState::TILE_HEATMAP_BLOOD_DECALS)     debugMode = 1;
            if (rendererSettings.rendererOverrideState == RendererOverrideState::TILE_HEATMAP_CHRISTMAS_LIGHTS) debugMode = 2;

            OpenGL::SetUniformInt("u_debugMode", debugMode);

            OpenGL::BindSSBO(5, "TileLights");
            OpenGL::BindSSBO(6, "TileBloodDecals");
            OpenGL::BindSSBO(7, "TileChristmasLights");

			uint32_t attachmentHandle = 0;

			switch (Unloved::Renderer::GetRendererMode()) {
			    case RendererMode::OLD_DEFERRED: attachmentHandle = OpenGL::ResourceManager::GetFrameBuffer("GBuffer").GetColorAttachmentHandleByName("Lighting"); break;
			    case RendererMode::RE_STYLE:     attachmentHandle = OpenGL::ResourceManager::GetFrameBuffer("GBufferRE").GetColorAttachmentHandleByName("Lighting");    break;
			}

            OpenGL::BindImageTexture(0, attachmentHandle, GL_READ_WRITE, GL_RGBA16F);

            OpenGL::DispatchCompute(GetTileCountX(), GetTileCountY(), 1);
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

            OpenGLFrameBuffer& waterFrameBuffer = OpenGL::ResourceManager::GetFrameBuffer("Water");

            if (Unloved::Renderer::GetRendererMode() == RendererMode::RE_STYLE) {
				OpenGLFrameBuffer& gBufferRE = OpenGL::ResourceManager::GetFrameBuffer("GBufferRE");
				OpenGLShader& shader = OpenGL::ResourceManager::GetShader("DebugViewRE");

				OpenGL::BindShader("DebugViewRE");
				OpenGL::SetUniformFloat("u_brushSize", Unloved::Editor::GetMapHeightBrushSize());
				OpenGL::SetUniformBool("u_heightMapEditor", (Unloved::Editor::GetEditorMode() == EditorMode::MAP_HEIGHT_EDITOR) && Unloved::Editor::IsOpen());

				OpenGL::BindImageTexture(0, gBufferRE.GetColorAttachmentHandleByName("Lighting"), GL_READ_WRITE, GL_RGBA16F);
				OpenGL::BindTextureUnit(1, gBufferRE.GetColorAttachmentHandleByName("BaseColorMetallic"));
				OpenGL::BindTextureUnit(2, gBufferRE.GetColorAttachmentHandleByName("NormalXYRoughnessMisc"));
                OpenGL::BindTextureUnit(3, gBufferRE.GetColorAttachmentHandleByName("VelocityXYOcclusionSubSurface"));
                OpenGL::BindTextureUnit(8, indirectDiffuseFbo->GetColorAttachmentHandleByName("Color"));
                OpenGL::BindTextureUnit(9, gBufferRE.GetColorAttachmentHandleByName("Visibility"));
                // 10 is ocean flags
                OpenGL::BindTextureUnit(11, gBufferRE.GetDepthAttachmentHandle());
                OpenGL::BindTextureUnit(12, gBufferRE.GetColorAttachmentHandleByName("Emissive"));

                OpenGL::DispatchCompute(gBuffer->GetWidth() / TILE_SIZE, gBuffer->GetHeight() / TILE_SIZE, 1);
			}
			else {
                OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("DebugView");
				if (!shader) return;

				OpenGL::BindShader("DebugView");
				OpenGL::SetUniformFloat("u_brushSize", Unloved::Editor::GetMapHeightBrushSize());
				OpenGL::SetUniformBool("u_heightMapEditor", (Unloved::Editor::GetEditorMode() == EditorMode::MAP_HEIGHT_EDITOR) && Unloved::Editor::IsOpen());

                OpenGL::BindImageTexture(0, gBuffer->GetColorAttachmentHandleByName("Lighting"), GL_READ_WRITE, GL_RGBA16F);
                OpenGL::BindTextureUnit(1, gBuffer->GetColorAttachmentHandleByName("BaseColorMetallic"));
                OpenGL::BindTextureUnit(2, gBuffer->GetColorAttachmentHandleByName("NormalXYRoughnessMisc"));
                OpenGL::BindTextureUnit(3, gBuffer->GetColorAttachmentHandleByName("VelocityXYOcclusionSubSurface"));
                // 7 may be free???
                OpenGL::BindTextureUnit(8, indirectDiffuseFbo->GetColorAttachmentHandleByName("Color"));
                // 9 is visibility
                // 10 is ocean flags
                OpenGL::BindTextureUnit(11, gBuffer->GetDepthAttachmentHandle());
                OpenGL::BindTextureUnit(12, gBuffer->GetColorAttachmentHandleByName("Emissive"));

				OpenGL::DispatchCompute(gBuffer->GetWidth() / TILE_SIZE, gBuffer->GetHeight() / TILE_SIZE, 1);
            }

        }
    }
}
