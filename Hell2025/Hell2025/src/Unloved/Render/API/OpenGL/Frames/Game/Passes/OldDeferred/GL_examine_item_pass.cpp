#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Hell/Render/API/OpenGL/GL_back_end.h"
#include "Hell/Backend/BackEnd.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Viewport/ViewportManager.h"
#include "Unloved/Editor/Editor.h"
#include "Unloved/Render/RenderDataManager.h"
#include "World/LegacyWorld.h"

#include "Hell/ResourceManagement/ResourceManager.h"

namespace OpenGLRenderer {

    void InventoryGaussianPass() {
        ProfilerOpenGLZoneFunction();

        if (Unloved::Editor::IsOpen()) return;

        OpenGLFrameBuffer& gBuffer = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");

        for (int i = 0; i < Unloved::Session::GetLocalPlayerCount(); i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            Unloved::Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(i);

            if (!viewport->IsVisible()) continue;
            if (!player->InventoryIsOpen()) continue;

            Unloved::SpaceCoords gBufferSpaceCooords = viewport->GetGBufferSpaceCoords();

            BlitRect blitRect;
            blitRect.x0 = gBufferSpaceCooords.gpuLeftPixel;
            blitRect.x1 = gBufferSpaceCooords.gpuRightPixel;
            blitRect.y0 = gBufferSpaceCooords.gpuTopPixel;
            blitRect.y1 = gBufferSpaceCooords.gpuBottomPixel;

            GaussianBlur(gBuffer, gBuffer, "Lighting", "Lighting", blitRect, blitRect, 5, 1);
        }
    }

    void ExamineItemPass() {
        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer* gBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");
        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("ExamineItem");
        if (!gBuffer) return;
        if (!shader) return;

        gBuffer->Bind();
        gBuffer->DrawBuffers({ "Lighting" });

        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);

        gBuffer->ClearDepthAttachment(0.0f);

        OpenGLRasterizerStateManager::ForceRasterizerState("GeometryPass_Default");


        glm::vec3 cameraPosition = glm::vec3(0, 0, 1.5f); // Remember the item is rendered at (0,0,0)

        Transform cameraTransform;
        cameraTransform.position = cameraPosition;
        glm::mat4 viewMatrix = glm::inverse(cameraTransform.to_mat4());

        OpenGL::BindShader("ExamineItem");
        OpenGL::SetUniformMat4("u_model", glm::mat4(1));
        OpenGL::SetUniformMat4("u_viewMatrix", viewMatrix);
        OpenGL::SetUniformVec3("u_viewPos", cameraPosition);
        OpenGL::SetUniformBool("u_flipNormalMapY", ShouldFlipNormalMapY());

        glBindVertexArray(Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetVAO());

        // Non blended
        for (int i = 0; i < Unloved::Session::GetLocalPlayerCount(); i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            Unloved::Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(i);

            if (!viewport->IsVisible()) continue;
            if (player->InventoryIsClosed()) continue;
            if (player->GetInvetoryState() != InventoryState::EXAMINE_ITEM) continue;

            OpenGLRenderer::SetViewport(gBuffer, viewport);
                //float m_fov = glm::radians(20.0f);
                //float m_aspect = 1920.0f / 1080.0f;
                //float m_nearPlane = NEAR_PLANE;
                //float m_farPlane = FAR_PLANE;
                //glm::mat4 perspectiveMatrix = glm::perspective(m_fov, m_aspect, m_nearPlane, m_farPlane);
                //shader->SetMat4("u_poMatrix", perspectiveMatrix); // make this a per viewport "inventory perspective matrix"

                OpenGL::SetUniformInt("u_viewportIndex", 0);

                Unloved::Inventory& inventory = player->GetInventory();
                std::vector<RenderItem> m_renderItems = inventory.GetRenderItems();

                for (RenderItem& renderItem : m_renderItems) {
                    Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(renderItem.meshId);
                    OpenGL::SetUniformMat4("u_model", renderItem.modelMatrix);
                    OpenGL::SetUniformMat4("u_inverseModel", renderItem.inverseModelMatrix);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(renderItem.baseColorTextureIndex)->GetGLTexture().GetHandle());
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(renderItem.normalMapTextureIndex)->GetGLTexture().GetHandle());
                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(renderItem.rmaTextureIndex)->GetGLTexture().GetHandle());

                    glDrawElementsBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (GLvoid*)(mesh->baseIndex * sizeof(GLuint)), mesh->baseVertex);

                   //std::cout << mesh->name << "\n";
                   //std::cout << Hell::String::FormatMat4(renderItem.modelMatrix) << "\n\n";
                }
        }

    }
}


