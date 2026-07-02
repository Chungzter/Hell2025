#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Hell/Render/API/OpenGL/GL_back_end.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Session/Session.h" // remove me when u can
#include "World/LegacyWorld.h" // remove me when u can
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

        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("ComputeSkinning");
        OpenGLSSBO* skinningTransformsSSBO = OpenGL::ResourceManager::GetSSBOPtr("SkinningTransforms");

        if (!shader) return;
        if (!skinningTransformsSSBO) return;

        // Calculate total amount of vertices to skin and allocate space
        uint32_t totalVertexCount = 0;
        Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("AssetGeometry");
        for (const RenderItem& renderItem : Unloved::RenderDataManager::GetCombinedSkinnedRenderItems()) {
            Mesh* mesh = meshBuffer.GetMeshById(renderItem.meshId);
            if (!mesh) continue;

            totalVertexCount += mesh->vertexCount;
        }

        // Make sure there is enough space allocated on the GPU to store them all
        OpenGL::BackEnd::AllocateSkinnedVertexBufferSpace(totalVertexCount);

        // Skin
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, OpenGL::BackEnd::GetSkinnedVertexDataVBO());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, meshBuffer.GetVBO());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, skinningTransformsSSBO->GetHandle());
        OpenGL::BindSSBO(3, "VertexWeights");

        OpenGL::BindShader("ComputeSkinning");

        const std::vector<glm::mat4>& skinningTransforms = Unloved::RenderDataManager::GetSkinningTransforms();
        skinningTransformsSSBO->Update(skinningTransforms.size() * sizeof(glm::mat4), &skinningTransforms[0]);

        for (const RenderItem& renderItem : Unloved::RenderDataManager::GetCombinedSkinnedRenderItems()) {
            uint32_t meshId = renderItem.meshId;
            Mesh* mesh = meshBuffer.GetMeshById(meshId);
            if (!mesh) continue;

            OpenGL::SetUniformInt("vertexCount", mesh->vertexCount);
            OpenGL::SetUniformInt("baseInputVertex", mesh->baseVertex);
            OpenGL::SetUniformInt("baseInputVertexWeight", renderItem.baseVertexWeight);
            OpenGL::SetUniformInt("baseOutputVertex", renderItem.baseSkinnedVertex);
            OpenGL::SetUniformInt("baseTransformIndex", renderItem.baseSkinningTransformIndex);

            //std::cout << mesh->name << " " << renderItem.baseVertexWeight << "\n";

            GLuint workgroupSize = 128;
            GLuint groupCountX = (mesh->vertexCount + workgroupSize - 1) / workgroupSize;
            OpenGL::DispatchCompute(groupCountX, 1, 1);
        }

        glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
    }
}
