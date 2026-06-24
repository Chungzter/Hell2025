#include "../GL_renderer.h"
#include "API/OpenGL/GL_backend.h"
#include "Renderer/RenderDataManager.h"
#include "Core/Game.h" // remove me when u can
#include "World/World.h" // remove me when u can
#include "Util.h"
#include "Hell/ResourceManagement/ResourceManager.h"

// TODO
struct SkinningCommand {
    uint32_t vertexCount;
    uint32_t baseInputVertex;
    uint32_t baseOutputVertex;
    uint32_t baseTransformIndex;
};

namespace OpenGLRenderer {

    void ComputeSkinningPass() {
        ProfilerOpenGLZoneFunction();

        OpenGLShader* shader = GetShaderOLD("ComputeSkinning");
        OpenGLSSBO* skinningTransformsSSBO = GetSSBO("SkinningTransforms");

        if (!shader) return;
        if (!skinningTransformsSSBO) return;

        // Calculate total amount of vertices to skin and allocate space
        uint32_t totalVertexCount = 0;
        Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("AssetGeometry");
        for (const RenderItem& renderItem : RenderDataManager::GetCombinedSkinnedRenderItems()) {
            Mesh* mesh = meshBuffer.GetMeshById(renderItem.meshId);
            if (!mesh) continue;

            totalVertexCount += mesh->vertexCount;
        }

        // Make sure there is enough space allocated on the GPU to store them all
        OpenGLBackEnd::AllocateSkinnedVertexBufferSpace(totalVertexCount);

        // Skin
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, OpenGLBackEnd::GetSkinnedVertexDataVBO());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, meshBuffer.GetVBO());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, skinningTransformsSSBO->GetHandle());
        BindSSBO(3, "VertexWeights");

        shader->Bind();

        const std::vector<glm::mat4>& skinningTransforms = RenderDataManager::GetSkinningTransforms();
        skinningTransformsSSBO->Update(skinningTransforms.size() * sizeof(glm::mat4), &skinningTransforms[0]);

        for (const RenderItem& renderItem : RenderDataManager::GetCombinedSkinnedRenderItems()) {
            uint32_t meshId = renderItem.meshId;
            Mesh* mesh = meshBuffer.GetMeshById(meshId);
            if (!mesh) continue;

            shader->SetInt("vertexCount", mesh->vertexCount);
            shader->SetInt("baseInputVertex", mesh->baseVertex);
            shader->SetInt("baseInputVertexWeight", renderItem.baseVertexWeight);
            shader->SetInt("baseOutputVertex", renderItem.baseSkinnedVertex);
            shader->SetInt("baseTransformIndex", renderItem.baseSkinningTransformIndex);

            //std::cout << mesh->name << " " << renderItem.baseVertexWeight << "\n";

            GLuint workgroupSize = 128;
            GLuint groupCountX = (mesh->vertexCount + workgroupSize - 1) / workgroupSize;
            glDispatchCompute(groupCountX, 1, 1);
        }

        glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
    }
}
