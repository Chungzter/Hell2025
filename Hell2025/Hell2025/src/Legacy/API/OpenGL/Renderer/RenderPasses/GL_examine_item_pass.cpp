#include "API/OpenGL/Renderer/GL_renderer.h" 
#include "API/OpenGL/GL_backend.h"
#include "Hell/Backend/BackEnd.h"
#include "Core/GameOLD.h"
#include "Viewport/ViewportManager.h"
#include "Editor/Editor.h"
#include "Renderer/RenderDataManager.h"
#include "Modelling/Clipping.h"
#include "Modelling/Unused/Modelling.h"
#include "World/World.h"

#include "Hell/ResourceManagement/ResourceManager.h"

namespace OpenGLRenderer {

    void InventoryGaussianPass() {
        ProfilerOpenGLZoneFunction();

        if (Editor::IsOpen()) return;

        OpenGLFrameBuffer& gBuffer = GetFrameBuffer("GBuffer");

        for (int i = 0; i < GameOLD::GetLocalPlayerCount(); i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            Player* player = GameOLD::GetLocalPlayerByIndex(i);

            if (!viewport->IsVisible()) continue;
            if (!player->InventoryIsOpen()) continue;

            SpaceCoords gBufferSpaceCooords = viewport->GetGBufferSpaceCoords();

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

        OpenGLFrameBuffer* gBuffer = GetFrameBufferOLD("GBuffer");
        OpenGLShader* shader = GetShaderOLD("ExamineItem");
        if (!gBuffer) return;
        if (!shader) return;

        gBuffer->Bind();
        gBuffer->DrawBuffers({ "Lighting" });

        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);

        gBuffer->ClearDepthAttachment(0.0f);

        ForceRasterizerState("GeometryPass_Default");


        glm::vec3 cameraPosition = glm::vec3(0, 0, 1.5f); // Remember the item is rendered at (0,0,0)

        Transform cameraTransform;
        cameraTransform.position = cameraPosition;
        glm::mat4 viewMatrix = glm::inverse(cameraTransform.to_mat4());

        shader->Bind();
        shader->SetMat4("u_model", glm::mat4(1));
        shader->SetMat4("u_viewMatrix", viewMatrix);
        shader->SetVec3("u_viewPos", cameraPosition); 
        shader->SetBool("u_flipNormalMapY", ShouldFlipNormalMapY());

        glBindVertexArray(Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetVAO());

        // Non blended
        for (int i = 0; i < GameOLD::GetLocalPlayerCount(); i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            Player* player = GameOLD::GetLocalPlayerByIndex(i);

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

                shader->SetInt("u_viewportIndex", 0);

                Inventory& inventory = player->GetInventory();
                std::vector<RenderItem> m_renderItems = inventory.GetRenderItems();

                for (RenderItem& renderItem : m_renderItems) {
                    Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(renderItem.meshId);
                    shader->SetMat4("u_model", renderItem.modelMatrix);
                    shader->SetMat4("u_inverseModel", renderItem.inverseModelMatrix);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(renderItem.baseColorTextureIndex)->GetGLTexture().GetHandle());
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(renderItem.normalMapTextureIndex)->GetGLTexture().GetHandle());
                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(renderItem.rmaTextureIndex)->GetGLTexture().GetHandle());

                    glDrawElementsBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (GLvoid*)(mesh->baseIndex * sizeof(GLuint)), mesh->baseVertex);
            
                   //std::cout << mesh->name << "\n";
                   //std::cout << Util::Mat4ToString(renderItem.modelMatrix) << "\n\n";
                }
        }

    }
}


