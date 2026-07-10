#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Unloved/Editor/Editor.h"
#include "Unloved/Viewport/ViewportManager.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Render/Renderer.h"

#include "Unloved/Editor/Gizmo.h"

namespace OpenGL::Renderer {
    using namespace Unloved;


    void EditorPass() {
        const std::vector<ViewportData>& viewportData = Unloved::RenderDataManager::GetViewportData();

        std::string gBufferName = (Unloved::Renderer::GetRendererMode() == RendererMode::RE_STYLE) ? "GBuffer" : "GBuffer";
        OpenGLFrameBuffer& gBuffer = OpenGL::ResourceManager::GetFrameBuffer(gBufferName);

        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("SolidColor");

        if (!shader) return;
        if (!Editor::IsOpen()) return;

        OpenGLRasterizerState state;
        state.depthMask = true;
        state.depthTestEnabled = true;
        state.blendEnable = true;
        state.depthFunc = GL_GREATER;
        OpenGL::RasterizerStateManager::ForceRasterizerState(state);

        gBuffer.Bind();
        gBuffer.DrawBuffers({ "Lighting" });
        gBuffer.SetViewport();
        gBuffer.ClearDepthAttachment(0.0f);

        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (viewport->IsVisible()) {

                OpenGL::Renderer::SetViewport(&gBuffer, viewport);

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
