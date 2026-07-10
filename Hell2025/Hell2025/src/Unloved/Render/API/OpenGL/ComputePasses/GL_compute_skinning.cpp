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

namespace OpenGL::Renderer {

    void ComputeSkinningPass() {
        ProfilerOpenGLZoneFunction();

        const std::vector<SkinningJob>& skinningJobs = Unloved::RenderDataManager::GetSkinningJobs();
        const std::vector<glm::mat4>& skinningTransforms = Unloved::RenderDataManager::GetSkinningTransforms();
        uint32_t totalVertexCount = Unloved::RenderDataManager::GetRequiredSkinnedVertexCount();

        if (skinningJobs.empty()) return;
        if (skinningTransforms.empty()) return;
        if (totalVertexCount == 0) return;

        // Calculate total amount of vertices to skin and allocate space
        Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("AssetGeometry");
        OpenGLMeshBuffer& glMeshBuffer = OpenGL::ResourceManager::GetMeshBuffer("AssetGeometry");

        // Make sure there is enough space allocated on the GPU to store them all
        OpenGL::BackEnd::AllocateSkinnedVertexBufferSpace(totalVertexCount);

        UpdateSSBO("SkinningJobs", skinningJobs.size() * sizeof(SkinningJob), skinningJobs.data());
        UpdateSSBO("SkinningTransforms", skinningTransforms.size() * sizeof(glm::mat4), skinningTransforms.data());

        // Skin
        BindSSBO(0, BackEnd::GetSkinnedVertexDataVBO());
        BindSSBO(1, glMeshBuffer.GetVBO());
        BindSSBO(2, "SkinningTransforms");
        BindSSBO(3, glMeshBuffer.GetVertexWeightSSBO());
        BindSSBO(4, "SkinningJobs");

        BindShader("ComputeSkinning");
        SetUniformInt("u_totalVertexBufferSize", totalVertexCount);

        int i = 0;

        for (const SkinningJob& skinningJob : skinningJobs) {

            SetUniformInt("u_jobIndex", i);

            SetUniformInt("vertexCount", skinningJob.vertexCount);
            SetUniformInt("baseInputVertex", skinningJob.baseVertex);
            SetUniformInt("baseInputVertexWeight", skinningJob.baseVertexWeight);
            SetUniformInt("baseOutputVertex", skinningJob.baseSkinningVertex);
            SetUniformInt("baseTransformIndex", skinningJob.baseSkinningTransformIndex);

            GLuint workgroupSize = 128;
            GLuint groupCountX = (skinningJob.vertexCount + workgroupSize - 1) / workgroupSize;
            DispatchCompute(groupCountX, 1, 1);

            i++;
        }

        glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
    }









    void ComputeSkinningPassOLD() {
        ProfilerOpenGLZoneFunction();

        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("ComputeSkinning");
        OpenGLSSBO* skinningTransformsSSBO = OpenGL::ResourceManager::GetSSBOPtr("SkinningTransforms");

        if (!shader) return;
        if (!skinningTransformsSSBO) return;

        const std::vector<RenderItem>& skinnedRenderItems = Unloved::RenderDataManager::GetCombinedSkinnedRenderItems();
        const std::vector<glm::mat4>& skinningTransforms = Unloved::RenderDataManager::GetSkinningTransforms();
        uint32_t totalVertexCount = Unloved::RenderDataManager::GetRequiredSkinnedVertexCount();

        if (skinnedRenderItems.empty()) return;
        if (skinningTransforms.empty()) return;
        if (totalVertexCount == 0) return;

        // Calculate total amount of vertices to skin and allocate space
        Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("AssetGeometry");
        OpenGLMeshBuffer& glMeshBuffer = OpenGL::ResourceManager::GetMeshBuffer("AssetGeometry");

        // Make sure there is enough space allocated on the GPU to store them all
        OpenGL::BackEnd::AllocateSkinnedVertexBufferSpace(totalVertexCount);

        skinningTransformsSSBO->Update(skinningTransforms.size() * sizeof(glm::mat4), skinningTransforms.data());

        // Skin
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, OpenGL::BackEnd::GetSkinnedVertexDataVBO());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, glMeshBuffer.GetVBO());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, skinningTransformsSSBO->GetHandle());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, glMeshBuffer.GetVertexWeightSSBO());

        OpenGL::BindShader("ComputeSkinning");

        for (const RenderItem& renderItem : skinnedRenderItems) {
            uint32_t meshId = renderItem.meshId;
            Mesh* mesh = meshBuffer.GetMeshById(meshId);
            if (!mesh) continue;

            OpenGL::SetUniformInt("vertexCount", mesh->vertexCount);
            OpenGL::SetUniformInt("baseInputVertex", mesh->baseVertex);
            OpenGL::SetUniformInt("baseInputVertexWeight", renderItem.baseVertexWeight);
            OpenGL::SetUniformInt("baseOutputVertex", renderItem.baseVertex);
            OpenGL::SetUniformInt("baseTransformIndex", renderItem.baseSkinningTransformIndex);

            //std::cout << mesh->name << " " << renderItem.baseVertexWeight << "\n";

            GLuint workgroupSize = 128;
            GLuint groupCountX = (mesh->vertexCount + workgroupSize - 1) / workgroupSize;
            OpenGL::DispatchCompute(groupCountX, 1, 1);
        }

        glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
    }
}
