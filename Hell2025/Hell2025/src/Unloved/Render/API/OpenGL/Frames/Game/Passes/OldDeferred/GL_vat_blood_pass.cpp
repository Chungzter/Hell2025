#include "Hell/Render/API/OpenGL/GL_back_end.h"
#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Systems/Blood/BloodSystem.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Viewport/ViewportManager.h"
#include "World/LegacyWorld.h"

#include "Hell/ResourceManagement/ResourceManager.h"

#include <cstdint>
#include <string>

namespace OpenGL::Renderer {

    void VatBloodPass() {
        //ProfilerOpenGLZoneFunction();

        OpenGL::RasterizerStateManager::ForceRasterizerState("GeometryPass_Default");

        const std::vector<ViewportData>& viewportData = Unloved::RenderDataManager::GetViewportData();

        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("VatBlood");
        OpenGLFrameBuffer* gBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");

        if (!shader) return;
        if (!gBuffer) return;

        OpenGL::BindShader("VatBlood");

        gBuffer->Bind();
        //gBuffer->DrawBuffers({ "BaseColor", "Normal", "RMA", "Emissive" });
        gBuffer->DrawBuffers({ "BaseColorMetallic", "NormalXYRoughnessMisc", "Emissive", "VelocityXYOcclusionSubSurface" });

        static int textureIndexBloodPos4 = Hell::ResourceManager::GetTextureBindlessIndexByName("blood_pos4");
        static int textureIndexBloodPos6 = Hell::ResourceManager::GetTextureBindlessIndexByName("blood_pos6");
        static int textureIndexBloodPos7 = Hell::ResourceManager::GetTextureBindlessIndexByName("blood_pos7");
        static int textureIndexBloodPos9 = Hell::ResourceManager::GetTextureBindlessIndexByName("blood_pos9");
        static int textureIndexBloodNorm4 = Hell::ResourceManager::GetTextureBindlessIndexByName("blood_norm4");
        static int textureIndexBloodNorm6 = Hell::ResourceManager::GetTextureBindlessIndexByName("blood_norm6");
        static int textureIndexBloodNorm7 = Hell::ResourceManager::GetTextureBindlessIndexByName("blood_norm7");
        static int textureIndexBloodNorm9 = Hell::ResourceManager::GetTextureBindlessIndexByName("blood_norm9");

        auto getFirstMeshId = [](const std::string& modelName) -> uint32_t {
            Model* model = Hell::ResourceManager::GetModelByName(modelName);
            if (!model || model->GetMeshIndices().empty()) {
                return 0;
            }

            return model->GetMeshIndices()[0];
        };

        static uint32_t meshId4 = getFirstMeshId("blood_mesh4");
        static uint32_t meshId6 = getFirstMeshId("blood_mesh6");
        static uint32_t meshId7 = getFirstMeshId("blood_mesh7");
        static uint32_t meshId9 = getFirstMeshId("blood_mesh9");

        std::vector<BloodVAT>& bloodVATItems = Unloved::BloodSystem::GetBloodVAT();

        static std::vector<RenderItem> renderItems;
        renderItems.clear();

        for (BloodVAT& bloodVAT : bloodVATItems) {
            RenderItem& renderItem = renderItems.emplace_back();
            renderItem.modelMatrix = bloodVAT.GetModelMatrix();
            renderItem.inverseModelMatrix = glm::inverse(renderItem.modelMatrix);
            renderItem.emissiveR = bloodVAT.GetLifeTime();

            if (bloodVAT.GetType() == 4) {
                renderItem.baseColorTextureIndex = textureIndexBloodPos4;
                renderItem.normalMapTextureIndex = textureIndexBloodNorm4;
                renderItem.meshId = meshId4;
            }
            else if (bloodVAT.GetType() == 6) {
                renderItem.baseColorTextureIndex = textureIndexBloodPos6;
                renderItem.normalMapTextureIndex = textureIndexBloodNorm6;
                renderItem.meshId = meshId6;
            }
            else if (bloodVAT.GetType() == 7) {
                renderItem.baseColorTextureIndex = textureIndexBloodPos7;
                renderItem.normalMapTextureIndex = textureIndexBloodNorm7;
                renderItem.meshId = meshId7;
            }
            else if (bloodVAT.GetType() == 9) {
                renderItem.baseColorTextureIndex = textureIndexBloodPos9;
                renderItem.normalMapTextureIndex = textureIndexBloodNorm9;
                renderItem.meshId = meshId9;
            }
        }

        glBindVertexArray(OpenGL::ResourceManager::GetMeshBuffer("AssetGeometry").GetVAO());

        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGL::Renderer::SetViewport(gBuffer, viewport);
            OpenGL::SetUniformInt("u_viewportIndex", i);

            for (RenderItem& renderItem: renderItems) {

                Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(renderItem.meshId);
                if (!mesh) continue;

                OpenGL::SetUniformMat4("u_modelMatrix", renderItem.modelMatrix);
                OpenGL::SetUniformMat4("u_inverseModelMatrix", renderItem.inverseModelMatrix);
                OpenGL::SetUniformFloat("u_time", renderItem.emissiveR);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(renderItem.baseColorTextureIndex)->GetGLTexture().GetHandle());
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(renderItem.normalMapTextureIndex)->GetGLTexture().GetHandle());

                glDrawElementsBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), mesh->baseVertex);
            }
        }

    }
}
