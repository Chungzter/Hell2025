#include "Hell/Render/API/OpenGL/GL_back_end.h"
#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Unloved/Debug/DebugDraw.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Viewport/ViewportManager.h"
#include "World/LegacyWorld.h"
#include "Unloved/World/World.h"

#include "Unloved/Session/Session.h"
#include "Unloved/Render/Renderer.h"

#include "Hell/ResourceManagement/ResourceManager.h"

namespace OpenGLRenderer {
    using namespace Unloved;


    void SpriteSheetPass() {
        //ProfilerOpenGLZoneFunction();

        const std::vector<ViewportData>& viewportData = Unloved::RenderDataManager::GetViewportData();
        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("SpriteSheet");
        Model* primitives = Hell::ResourceManager::GetModelByName("Primitives");
        if (!primitives || primitives->GetMeshIndices().empty()) return;
        if (primitives->GetMeshCount() == 0) return;

        uint32_t meshId = primitives->GetMeshIndices()[0];
        Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(meshId);
        if (!mesh) return;

        std::string gBufferName = (Unloved::Renderer::GetRendererMode() == RendererMode::RE_STYLE) ? "GBufferRE" : "GBuffer";
        OpenGLFrameBuffer& gBuffer = OpenGL::ResourceManager::GetFrameBuffer(gBufferName);

        gBuffer.Bind();
        gBuffer.DrawBuffer("Lighting");
        OpenGL::BindShader("SpriteSheet");
        OpenGLRasterizerStateManager::ForceRasterizerState("SpriteSheetPass");

        glBindVertexArray(Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetVAO());

        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGLRenderer::SetViewport(&gBuffer, viewport);

            Unloved::Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(i);
            if (!player) continue;

            const std::vector<SpriteSheetRenderItem>& renderItems = player->GetSpriteSheetRenderItems();
            for (const SpriteSheetRenderItem& renderItem : renderItems) {

                Texture* texture = Hell::ResourceManager::GetTextureByBindlessIndex(renderItem.textureIndex);
                if (!texture) {
                    std::cout << "Spritesheet pass had a null ptr texture from index " << renderItem.textureIndex << "\n";
                    continue;
                }

                OpenGL::SetUniformInt("u_rowCount", renderItem.rowCount);
                OpenGL::SetUniformInt("u_columnCount", renderItem.columnCount);
                OpenGL::SetUniformInt("u_frameIndex", renderItem.frameIndex);
                OpenGL::SetUniformInt("u_frameNextIndex", renderItem.frameIndexNext);
                OpenGL::SetUniformFloat("u_mixFactor", renderItem.mixFactor);
                OpenGL::SetUniformVec4("u_position", renderItem.position);
                OpenGL::SetUniformVec4("u_rotation", renderItem.rotation);
                OpenGL::SetUniformVec4("u_scale", renderItem.scale);
                OpenGL::SetUniformInt("u_billboard", false); // check this shit, on the muzzleflash createinfo coz u have wrong uniform name here!!!!!!!
                OpenGL::SetUniformFloat("u_uOffset", renderItem.uOffset);
                OpenGL::SetUniformFloat("u_vOffset", renderItem.vOffset);
                OpenGL::SetUniformVec4("u_worldBoundsMin", renderItem.aabbMin);
                OpenGL::SetUniformVec4("u_worldBoundsMax", renderItem.aabbMax);
                OpenGL::SetUniformBool("u_useFireClipHeight", false);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, texture->GetGLTexture().GetHandle());
                glDrawElementsInstancedBaseVertexBaseInstance(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (GLvoid*)(mesh->baseIndex * sizeof(GLuint)), 1, mesh->baseVertex, i);
            }

            // No depth test for fire
            //glDisable(GL_DEPTH_TEST);
            for (Fireplace& fireplace : Unloved::World::GetFireplaces()) {
                const SpriteSheetRenderItem& renderItem = fireplace.GetFireSpriteSheetRenderItem();
                Texture* texture = Hell::ResourceManager::GetTextureByBindlessIndex(renderItem.textureIndex);
                if (!texture) continue;

                OpenGL::SetUniformInt("u_rowCount", renderItem.rowCount);
                OpenGL::SetUniformInt("u_columnCount", renderItem.columnCount);
                OpenGL::SetUniformInt("u_frameIndex", renderItem.frameIndex);
                OpenGL::SetUniformInt("u_frameNextIndex", renderItem.frameIndexNext);
                OpenGL::SetUniformFloat("u_mixFactor", renderItem.mixFactor);
                OpenGL::SetUniformVec4("u_position", renderItem.position);
                OpenGL::SetUniformVec4("u_rotation", renderItem.rotation);
                OpenGL::SetUniformVec4("u_scale", renderItem.scale);
                OpenGL::SetUniformInt("u_billboard", renderItem.isBillboard);
                OpenGL::SetUniformFloat("u_uOffset", renderItem.uOffset);
                OpenGL::SetUniformFloat("u_vOffset", renderItem.vOffset);
                OpenGL::SetUniformVec4("u_worldBoundsMin", renderItem.aabbMin);
                OpenGL::SetUniformVec4("u_worldBoundsMax", renderItem.aabbMax);
                OpenGL::SetUniformBool("u_useFireClipHeight", fireplace.m_useFireClipHeight);


                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, texture->GetGLTexture().GetHandle());
                glDrawElementsInstancedBaseVertexBaseInstance(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (GLvoid*)(mesh->baseIndex * sizeof(GLuint)), 1, mesh->baseVertex, i);
            }
        }
    }
}
