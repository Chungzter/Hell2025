#include "../GL_renderer.h"
#include "Editor/Editor.h"
#include "Viewport/ViewportManager.h"
#include "Renderer/RenderDataManager.h"
#include "Renderer/Renderer.h"

#include "Editor/Gizmo.h"

namespace OpenGLRenderer {

    void EditorPass() {
        const std::vector<ViewportData>& viewportData = RenderDataManager::GetViewportData();

        std::string gBufferName = (Renderer::GetRendererMode() == RendererMode::RE_STYLE) ? "GBufferRE" : "GBuffer";
        OpenGLFrameBuffer& gBuffer = OpenGL::ResourceManager::GetFrameBuffer(gBufferName);

        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("SolidColor");

        if (!shader) return;
        if (!Editor::IsOpen()) return;

        OpenGLRasterizerState state;
        state.depthMask = true;
        state.depthTestEnabled = true;
        state.blendEnable = true;
        state.depthFunc = GL_GREATER;
        OpenGLRasterizerStateManager::ForceRasterizerState(state);

        gBuffer.Bind();
        gBuffer.DrawBuffers({ "Lighting" });
        gBuffer.SetViewport();
        gBuffer.ClearDepthAttachment(0.0f);

        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (viewport->IsVisible()) {

                OpenGLRenderer::SetViewport(&gBuffer, viewport);

                OpenGL::BindShader("SolidColor");
                OpenGL::SetUniformMat4("projection", viewportData[i].projectionReverseZ);
                OpenGL::SetUniformMat4("view", viewportData[i].view);
                OpenGL::SetUniformBool("useUniformColor", true);

                if (Editor::GetSelectedObjectType() != ObjectType::NO_TYPE) {
                    for (GizmoRenderItem& renderItem : Gizmo::GetRenderItemsByViewportIndex(i)) {
                        MeshBufferOLD* mesh = Gizmo::GetMeshBufferByIndex(renderItem.meshIndex);
                        if (mesh) {
                            OpenGLMeshBufferOLD glMesh = mesh->GetGLMeshBuffer();
                            OpenGL::SetUniformMat4("model", renderItem.modelMatrix);
                            OpenGL::SetUniformVec4("uniformColor", renderItem.color);
                            glBindVertexArray(glMesh.GetVAO());
                            glDrawElements(GL_TRIANGLES, glMesh.GetIndexCount(), GL_UNSIGNED_INT, 0);
                        }
                    }
                }
            }
        }

        // Cleanup
        glDisable(GL_BLEND);
    }
}