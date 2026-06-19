#include "SkinnedModel.h"
#include "AssetManagement/AssetManager.h"
#include "Util/Util.h"
#include <mutex>
#include "Hell/Logging.h"

void SkinnedModel::BakeToAssetManager() {
    m_vertexCount = m_skinnedModelData.vertexCount;
    m_indexCount = m_skinnedModelData.indexCount;
    m_boneOffsets = m_skinnedModelData.boneOffsets;
    m_nodes = m_skinnedModelData.nodes;
    m_boneMapping = m_skinnedModelData.boneMapping;

    for (SkinnedMeshData& skinnedMeshData : m_skinnedModelData.meshes) {
        int meshIndex = AssetManager::CreateSkinnedMesh2(skinnedMeshData);
        AddMeshIndex(meshIndex);
    }

    // Store bone node indices
    m_boneNodeIndices.assign(m_boneMapping.size(), -1);
    for (int nodeIdx = 0; nodeIdx < GetNodeCount(); ++nodeIdx) {
        const auto& name = m_nodes[nodeIdx].name;
        auto it = m_boneMapping.find(name);
        if (it != m_boneMapping.end()) {
            m_boneNodeIndices[it->second] = nodeIdx;
        }
        m_nodeMapping[name] = nodeIdx;
    }
}

void SkinnedModel::PrintNodeInfo() {
    std::cout << "\n" << m_fileInfo.name.c_str() << " nodes\n";
    for (size_t i = 0; i < m_nodes.size(); ++i) {
        const Node& node = m_nodes[i];
        std::cout << " " << i << ": " << node.name.c_str() << " (" << node.parentIndex << ")\n";
    }
}

void SkinnedModel::PrintBoneInfo() {
    std::cout << "\n" << m_fileInfo.name.c_str() << " bones\n";

    for (const auto& pair : m_boneMapping) {
        std::cout << pair.first << ": " << pair.second << "\n";
    }
}

void SkinnedModel::SetFileInfo(FileInfo fileInfo) {
    m_fileInfo = fileInfo;
}

bool SkinnedModel::BoneExists(const std::string& boneName) {
    return m_boneMapping.find(boneName) != m_boneMapping.end();
}

const FileInfo& SkinnedModel::GetFileInfo() {
    return m_fileInfo;
}

void SkinnedModel::SetVertexCount(uint32_t vertexCount) {
    m_vertexCount = vertexCount;
}

const std::string& SkinnedModel::GetName() {
    return m_fileInfo.name;
}

void SkinnedModel::AddMeshIndex(uint32_t index) {
    m_meshIndices.push_back(index);
}

uint32_t SkinnedModel::GetMeshCount() {
    return m_meshIndices.size();
}

std::vector<uint32_t>& SkinnedModel::GetMeshIndices() {
    return m_meshIndices;
}

uint32_t SkinnedModel::GetVertexCount() { 
    return m_vertexCount; 
}

uint32_t SkinnedModel::GetBoneCount() {
    return m_boneMapping.size();
}

uint32_t SkinnedModel::GetNodeCount() {
    return m_nodes.size();
}

int32_t SkinnedModel::GetBoneIndex(const std::string& boneName) {
    auto it = m_boneMapping.find(boneName);
    return (it != m_boneMapping.end()) ? it->second : -1;
}

int32_t SkinnedModel::GetNodeIndex(const std::string& nodeName) {
    auto it = m_nodeMapping.find(nodeName);
    return (it != m_nodeMapping.end()) ? it->second : -1;
}

const glm::mat4& SkinnedModel::GetBoneOffset(const std::string& boneName) {
    const int boneIndex = GetBoneIndex(boneName);
    if (boneIndex >= 0 && boneIndex < (int)m_boneOffsets.size()) {
        return m_boneOffsets[boneIndex];
    }
    static const glm::mat4 identity(1.0f);
    return identity;
}

const glm::mat4& SkinnedModel::GetInverseBindTransform(const std::string& nodeName) {
    for (int i = 0; i < m_nodes.size(); i++) {
        if (m_nodes[i].name == nodeName) {
            return m_nodes[i].inverseBindTransform;
        }
    }

    Logging::Error() << "SkinnedModel::GetInverseBindTransform(..) failed to find '" << nodeName << "'";

    const static glm::mat4 identity(1.0f);
    return identity;
}

void SkinnedModel::SetLoadingState(LoadingState loadingState) {
    m_loadingState = loadingState;
}

LoadingState SkinnedModel::GetLoadingState() const {
    return m_loadingState;
}