#include "../GL_renderer.h"
#include "Editor/Editor.h"
#include "Viewport/ViewportManager.h"
#include "Renderer/RenderDataManager.h"

#include "Editor/Gizmo.h"

namespace OpenGLRenderer {

    void EditorPass() {
        const std::vector<ViewportData>& viewportData = RenderDataManager::GetViewportData();
        OpenGLFrameBuffer* gBuffer = GetFrameBufferOLD("GBuffer");
        OpenGLShader* shader = GetShaderOLD("SolidColor");

        if (!shader) return;
        if (!Editor::IsOpen()) return;

        OpenGLRasterizerState state;
        state.depthMask = true;
        state.depthTestEnabled = true;
        state.blendEnable = true;
        state.depthFunc = GL_GREATER;
        ForceRasterizerState(state);

        gBuffer->Bind();
        gBuffer->DrawBuffers({ "Lighting" });
        gBuffer->SetViewport();
        gBuffer->ClearDepthAttachment(0.0f);

        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (viewport->IsVisible()) {

                OpenGLRenderer::SetViewport(gBuffer, viewport);

                shader->Bind();
                shader->SetMat4("projection", viewportData[i].projectionReverseZ);
                shader->SetMat4("view", viewportData[i].view);
                shader->SetBool("useUniformColor", true);

                if (Editor::GetSelectedObjectType() != ObjectType::NO_TYPE) {
                    for (GizmoRenderItem& renderItem : Gizmo::GetRenderItemsByViewportIndex(i)) {
                        MeshBuffer* mesh = Gizmo::GetMeshBufferByIndex(renderItem.meshIndex);
                        if (mesh) {
                            OpenGLMeshBuffer glMesh = mesh->GetGLMeshBuffer();
                            shader->SetMat4("model", renderItem.modelMatrix);
                            shader->SetVec4("uniformColor", renderItem.color);
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