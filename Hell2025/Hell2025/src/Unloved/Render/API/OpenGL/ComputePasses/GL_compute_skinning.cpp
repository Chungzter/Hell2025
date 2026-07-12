#include "Hell/Render/API/OpenGL/GL_back_end.h"
#include "Hell/ResourceManagement/ResourceManager.h"

#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Unloved/Render/RenderDataManager.h"

#include "Hell/Input.h"
#include "Unloved/Debug/Debug.h"

namespace OpenGL::Renderer {

    void ComputeSkinningPass() {
        ProfilerOpenGLZoneFunction();

        const std::vector<SkinningDispatchGroup>& skinningDispatchGroups = Unloved::RenderDataManager::GetSkinningDispatchGroups();
        const std::vector<SkinningJob>& skinningJobs = Unloved::RenderDataManager::GetSkinningJobs();
        const std::vector<glm::mat4>& skinningTransforms = Unloved::RenderDataManager::GetSkinningTransforms();

        uint32_t totalVertexCount = Unloved::RenderDataManager::GetRequiredSkinnedVertexCount();

        if (skinningDispatchGroups.empty()) return;
        if (skinningJobs.empty()) return;
        if (skinningTransforms.empty()) return;

        // Calculate total amount of vertices to skin and allocate space
        Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("AssetGeometry");
        OpenGLMeshBuffer& glMeshBuffer = OpenGL::ResourceManager::GetMeshBuffer("AssetGeometry");

        // Make sure there is enough space allocated on the GPU to store them all
        OpenGL::BackEnd::AllocateSkinnedVertexBufferSpace(totalVertexCount);

        UpdateSSBO("SkinningDispatchGroups", skinningDispatchGroups.size() * sizeof(SkinningDispatchGroup), skinningDispatchGroups.data());
        UpdateSSBO("SkinningJobs", skinningJobs.size() * sizeof(SkinningJob), skinningJobs.data());
        UpdateSSBO("SkinningTransforms", skinningTransforms.size() * sizeof(glm::mat4), skinningTransforms.data());

        BindSSBO(0, BackEnd::GetSkinnedVertexDataVBO());
        BindSSBO(1, glMeshBuffer.GetVBO());
        BindSSBO(2, "SkinningTransforms");
        BindSSBO(3, glMeshBuffer.GetVertexWeightSSBO());
        BindSSBO(4, "SkinningJobs");
        BindSSBO(5, "SkinningDispatchGroups");

        BindShader("ComputeSkinning");
        DispatchCompute(skinningDispatchGroups.size(), 1, 1);

        glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
    }
}
