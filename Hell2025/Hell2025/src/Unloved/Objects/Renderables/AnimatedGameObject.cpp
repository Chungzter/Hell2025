#include "AnimatedGameObject.h"
#include "Unloved/Bible/Bible.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Debug/DebugDraw.h"
#include "Hell/Logging.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/Physics/Physics.h"
#include "Legacy/Renderer/RenderDataManager.h"
#include "Util.h"

#include <iostream>
#include <unordered_set>

namespace Unloved {

AnimatedGameObject::AnimatedGameObject(uint64_t id) {
    m_objectId = id;
}

void AnimatedGameObject::UpdateRenderItems() {
    m_animatedMeshNodes.UpdateRenderItems(GetModelMatrix(), m_boneSkinningMatrices);
}

const uint32_t AnimatedGameObject::GetVerteXCount() {
    if (m_skinnedModel) {
        return m_skinnedModel->GetVertexCount();
    }
    else {
        return 0;
    }
}

void AnimatedGameObject::UpdateBoneTransformsFromRagdoll() {
    Ragdoll* ragdoll = Hell::Physics::GetRagdollById(m_ragdollId);
    if (!ragdoll) return;
    if (!m_skinnedModel) return;

    int nodeCount = m_skinnedModel->m_nodes.size();
    m_animator.m_globalBlendedNodeTransforms.resize(nodeCount);

    for (int i = 0; i < m_skinnedModel->m_nodes.size(); i++) {
        std::string NodeName = m_skinnedModel->m_nodes[i].name;
        glm::mat4 nodeTransformation = glm::mat4(1);
        nodeTransformation = m_skinnedModel->m_nodes[i].inverseBindTransform;
        unsigned int parentIndex = m_skinnedModel->m_nodes[i].parentIndex;
        glm::mat4 ParentTransformation = (parentIndex == -1) ? glm::mat4(1) : m_animator.m_globalBlendedNodeTransforms[parentIndex];
        glm::mat4 GlobalTransformation = ParentTransformation * nodeTransformation;

        for (int j = 0; j < ragdoll->m_markerBoneNames.size(); j++) {
            if (ragdoll->m_markerBoneNames[j] == NodeName) {
                PxRigidDynamic* pxRigidDynamic = ragdoll->m_pxRigidDynamics[j];
                GlobalTransformation = Hell::Physics::PxMat44ToGlmMat4(pxRigidDynamic->getGlobalPose());
            }
        }

        m_animator.m_globalBlendedNodeTransforms[i] = GlobalTransformation;
    }
}


void AnimatedGameObject::Update(float deltaTime) {
    if (!m_skinnedModel) return;

    if (m_animationMode == AnimationMode::RAGDOLL_V2) {
        UpdateBoneTransformsFromRagdoll();
    }
    else {
        if (m_animationMode == AnimationMode::BINDPOSE) {
            //m_animationLayerOLD.ClearAllAnimationStates();
            //m_animator.ClearAllAnimations();
        }

        m_animator.UpdateAnimations(deltaTime);
        //m_globalBlendedNodeTransforms = m_animator.m_globalBlendedNodeTransforms;
    }

    m_boneSkinningMatrices.clear();

    for (uint32_t i = 0; i < m_skinnedModel->GetBoneCount(); i++) {
        m_boneSkinningMatrices.push_back(glm::mat4(1));
    }

    // Compute local bone matrices
    int boneCount = m_skinnedModel->GetBoneCount();
    m_boneSkinningMatrices.resize(boneCount);
    for (int b = 0; b < boneCount; ++b) {
        int nodeIdx = m_skinnedModel->m_boneNodeIndices[b];
        m_boneSkinningMatrices[b] = m_animator.m_globalBlendedNodeTransforms[nodeIdx] * m_skinnedModel->m_boneOffsets[b];
    }

    // If it has a ragdoll then sink the ragdoll rigids to the animated pose
    if (m_animationMode == AnimationMode::BINDPOSE || m_animationMode == AnimationMode::ANIMATION) {
        SyncRagdollToAnimation();
    }
}

void AnimatedGameObject::SyncRagdollToAnimation() {
    Ragdoll* ragdoll = Hell::Physics::GetRagdollById(m_ragdollId);
    if (!ragdoll) return;

    for (int i = 0; i < ragdoll->m_markerBoneNames.size(); i++) {
        const std::string& boneName = ragdoll->m_markerBoneNames[i];

        int nodeIndex = GetNodeIndex(boneName);
        if (nodeIndex == -1) {
            continue;
        }

        PxRigidDynamic* pxRigidDynamic = ragdoll->m_pxRigidDynamics[i];
        if (!pxRigidDynamic) {
            Logging::Error() << "pxRigidDynamic for " << boneName << " is nullptr";
            continue;
        }

        glm::mat4 boneWorldMatrix = GetModelMatrix() * GetAnimatedTransformByNodeIndex(nodeIndex);
        PxTransform pxTransform = PxTransform(Hell::Physics::GlmMat4ToPxMat44(boneWorldMatrix));

        pxRigidDynamic->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
        pxRigidDynamic->setGlobalPose(pxTransform);
    }
}

void AnimatedGameObject::CleanUp() {
    Hell::Physics::MarkRagdollForRemoval(m_ragdollId);
}

void AnimatedGameObject::SetMeshWoundMaskTextureIndex(const std::string& meshName, int32_t woundMaskTextureIndex) {
    m_animatedMeshNodes.SetMeshWoundMaskTextureIndex(meshName, woundMaskTextureIndex);
}

void AnimatedGameObject::SetBlendingModeByMeshName(const std::string& meshName, BlendingMode blendingMode) {
    m_animatedMeshNodes.SetBlendingModeByMeshName(meshName, blendingMode);
}

void AnimatedGameObject::SetMeshMaterialByMeshName(const std::string& meshName, const std::string& materialName) {
    m_animatedMeshNodes.SetMeshMaterialByMeshName(meshName, materialName);
}

void AnimatedGameObject::SetMeshFurLength(const std::string& meshName, float furLength) {
    m_animatedMeshNodes.SetMeshFurLength(meshName, furLength);
}

void AnimatedGameObject::SetMeshFurUVScale(const std::string& meshName, float uvScale) {
    m_animatedMeshNodes.SetMeshFurUVScale(meshName, uvScale);
}

void AnimatedGameObject::SetMeshFurShellDistanceAttenuation(const std::string& meshName, float furShellDistanceAttenuation) {
    m_animatedMeshNodes.SetMeshFurShellDistanceAttenuation(meshName, furShellDistanceAttenuation);
}

void AnimatedGameObject::SetMeshMaterialByMeshIndex(int meshIndex, const std::string& materialName) {
    m_animatedMeshNodes.SetMeshMaterialByMeshIndex(meshIndex, materialName);
}

void AnimatedGameObject::SetMeshToRenderAsGlassByMeshIndex(const std::string& meshName) {
    m_animatedMeshNodes.SetMeshToRenderAsGlassByMeshIndex(meshName);
}

void AnimatedGameObject::SetMeshEmissiveColorTextureByMeshName(const std::string& meshName, const std::string& textureName) {
    m_animatedMeshNodes.SetMeshEmissiveColorTextureByMeshName(meshName, textureName);
}

void AnimatedGameObject::SetMeshWoundMaterialByMeshName(const std::string& meshName, const std::string& textureName) {
    m_animatedMeshNodes.SetMeshWoundMaterialByMeshName(meshName, textureName);
}

void AnimatedGameObject::SetAllMeshMaterials(const std::string& materialName) {
    m_animatedMeshNodes.SetAllMeshMaterials(materialName);
}

void AnimatedGameObject::SetAllMeshBlendingModes(BlendingMode blendingMode) {
    m_animatedMeshNodes.SetAllMeshBlendingModes(blendingMode);
}

void AnimatedGameObject::SetExclusiveViewportIndex(int index) {
    m_animatedMeshNodes.SetExclusiveViewportIndex(index);
}

void AnimatedGameObject::SetIgnoredViewportIndex(int index) {
    m_animatedMeshNodes.SetIgnoredViewportIndex(index);
}

void AnimatedGameObject::EnableRendering() {
    m_animatedMeshNodes.EnableRendering();
}

void AnimatedGameObject::DisableRendering() {
    m_animatedMeshNodes.DisableRendering();
}


const glm::mat4& AnimatedGameObject::GetInverseBindTransformByBoneName(const std::string& name) {
    const static glm::mat4 identity = glm::mat4(1.0f);

    if (!m_skinnedModel) return identity;

    // Name exists?
    auto it = m_skinnedModel->m_nodeMapping.find(name);
    if (it == m_skinnedModel->m_nodeMapping.end()) return identity;

    unsigned int index = it->second;

    // Index in range
    if (index >= m_skinnedModel->m_nodes.size()) return identity;

    return m_skinnedModel->m_nodes[index].inverseBindTransform;
}


void AnimatedGameObject::SetAnimationModeToBindPose() {
    m_animationMode = AnimationMode::BINDPOSE;
    m_animator.ClearAllAnimations();
}


void AnimatedGameObject::SetAnimationModeToRagdoll() {
    Ragdoll* ragdoll = Hell::Physics::GetRagdollById(m_ragdollId);
    if (!ragdoll) {
        Logging::Error() << "AnimatedGameObject::SetAnimationModeToRagdoll() failed because m_ragdollId [" << m_ragdollId << "] was not found in the RagdollManager";
        return;
    }

    if (m_animationMode != AnimationMode::RAGDOLL_V2) {
        m_animationMode = AnimationMode::RAGDOLL_V2;
        m_animator.ClearAllAnimations();
        ragdoll->EnableSimulation();
    }
}


void AnimatedGameObject::SetAnimationModeToAnimated() {
    m_animationMode = AnimationMode::ANIMATION;
    m_animator.ClearAllAnimations();
}


void AnimatedGameObject::PlayAnimation(const std::string& layerName, const std::string& animationName, float speed) {
    m_animator.PlayAnimation(layerName, animationName, speed, false);
}


void AnimatedGameObject::PlayAndLoopAnimation(const std::string& layerName, const std::string& animationName, float speed) {
    m_animator.PlayAnimation(layerName, animationName, speed, true);
}


void AnimatedGameObject::PlayAnimation(const std::string& layerName, std::vector<std::string>& animationNames, float speed) {
    int rand = std::rand() % animationNames.size();
    PlayAnimation(layerName, animationNames[rand], speed);
}


void AnimatedGameObject::PlayAndLoopAnimation(const std::string& layerName, std::vector<std::string>& animationNames, float speed) {
    int rand = std::rand() % animationNames.size();
    PlayAndLoopAnimation(layerName, animationNames[rand], speed);
}


const glm::mat4 AnimatedGameObject::GetModelMatrix() {
    if (m_useModelMatrixOverride) {
        return m_modelMatrixOverride;
    }

    if (m_animationMode == AnimationMode::RAGDOLL_V2) {
        return glm::mat4(1);
    }
    else {
        return m_transform.to_mat4();
    }
}


bool AnimatedGameObject::IsAllAnimationsComplete() {
    return m_animator.AllAnimationsComplete();
}


void AnimatedGameObject::SetName(std::string name) {
    m_name = name;
}


void AnimatedGameObject::SetSkinnedModel(const std::string& name, const std::string& presetName) {
    SkinnedModel* ptr = Hell::ResourceManager::GetSkinnedModelByName(name);
    if (ptr) {
        m_skinnedModel = ptr;
        m_animator.SetSkinnedModel(name);
    }
    else {
        std::cout << "Could not SetSkinnedModel(name) with name: \"" << name << "\", it does not exist\n";
    }

    if (presetName != UNDEFINED_STRING) {
        Bible::ConfigureAnimatedMeshNodes(m_objectId, &m_animatedMeshNodes, presetName);
    }
    else {
        m_animatedMeshNodes.SetSkinnedModel(m_objectId, name);
    }
}


const glm::mat4& AnimatedGameObject::GetAnimatedTransformByNodeIndex(int32_t nodeIndex) {
    const static glm::mat4 identity = glm::mat4(1.0f);

    if (!m_skinnedModel || nodeIndex < 0 || nodeIndex >= m_animator.m_globalBlendedNodeTransforms.size()) {
        return identity;
    }

    return m_animator.m_globalBlendedNodeTransforms[nodeIndex];
}


const glm::mat4& AnimatedGameObject::GetAnimatedTransformByBoneName(const std::string& name) {
    const static glm::mat4 identity = glm::mat4(1.0f);

    if (!m_skinnedModel) return identity;

    auto it = m_skinnedModel->m_nodeMapping.find(name);
    if (it == m_skinnedModel->m_nodeMapping.end()) {
        //std::cout << "AnimatedGameObject::GetAnimatedTransformByBoneName() failed to find '" << name << "'\n";
        return identity;
    }

    int index = it->second;
    if (index < 0 || index >= int(m_animator.m_globalBlendedNodeTransforms.size())) {
        //std::cout << "AnimatedGameObject::GetAnimatedTransformByBoneName() '" << name << "' index " << index << " out of range of " << m_animator.m_globalBlendedNodeTransforms.size() << "\n";
        return identity;
    }

    return m_animator.m_globalBlendedNodeTransforms[index];
}


void AnimatedGameObject::SetScale(float scale) {
    m_transform.scale = glm::vec3(scale);
}

void AnimatedGameObject::SetPosition(glm::vec3 position) {
    m_transform.position = position;
}


void AnimatedGameObject::SetRotationX(float rotation) {
    m_transform.rotation.x = rotation;
}


void AnimatedGameObject::SetRotationY(float rotation) {
    m_transform.rotation.y = rotation;
}

void AnimatedGameObject::SetRotationZ(float rotation) {
    m_transform.rotation.z = rotation;
}

void AnimatedGameObject::PrintNodeNames() {
    std::cout << m_skinnedModel->GetName() << "\n";
    for (int i = 0; i < m_skinnedModel->m_nodes.size(); i++) {
        std::cout << "-" << i << " " << m_skinnedModel->m_nodes[i].name << "\n";
    }
}

void AnimatedGameObject::PrintMeshNames() {
    m_animatedMeshNodes.PrintMeshNames();
}


void AnimatedGameObject::EnableModelMatrixOverride() {
    m_useModelMatrixOverride = true;
}


void AnimatedGameObject::SetCameraMatrix(const glm::mat4& matrix) {
    m_modelMatrixOverride = matrix;
}


const uint32_t AnimatedGameObject::GetAnimationFrameNumber(const std::string& animationLayerName) {
    return m_animator.GetAnimationFrameNumber(animationLayerName);
}


bool AnimatedGameObject::AnimationIsPastFrameNumber(const std::string& animationLayerName, int frameNumber) {
    return m_animator.AnimationIsPastFrameNumber(animationLayerName, frameNumber);
}


void AnimatedGameObject::DrawBones(int exclusiveViewportIndex) {
    if (!m_skinnedModel) return;

    // Traverse the tree
    for (int i = 0; i < m_skinnedModel->m_nodes.size(); i++) {
        glm::mat4 nodeTransformation = glm::mat4(1);
        unsigned int parentIndex = m_skinnedModel->m_nodes[i].parentIndex;
        std::string& nodeName = m_skinnedModel->m_nodes[i].name;
        std::string& parentNodeName = m_skinnedModel->m_nodes[parentIndex].name;

        if (parentIndex != -1 && m_skinnedModel->BoneExists(nodeName) && m_skinnedModel->BoneExists(parentNodeName)) {
            const glm::mat4& boneWorldMatrix = m_animator.m_globalBlendedNodeTransforms[i];
            const glm::mat4& parentBoneWorldMatrix = m_animator.m_globalBlendedNodeTransforms[parentIndex];
            glm::vec3 position = GetModelMatrix() * boneWorldMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
            glm::vec3 parentPosition = GetModelMatrix() * parentBoneWorldMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
            DebugDraw::DrawPoint(position, OUTLINE_COLOR, false, exclusiveViewportIndex);
            DebugDraw::DrawLine(position, parentPosition, WHITE, false, exclusiveViewportIndex);
        }
    }

    // // To draw all nodes
    // for (const glm::mat4& boneWorldMatrix : m_animationLayer.m_globalBlendedNodeTransforms) {
    //     glm::vec3 position = GetModelMatrix() * boneWorldMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    //     DebugDraw::DrawPoint(position, color, false, exclusiveViewportIndex);
    // }
}


void AnimatedGameObject::DrawBoneTangentVectors(float size, int exclusiveViewportIndex) {
    size *= 0.5f;

    for (const glm::mat4& boneWorldMatrix : m_animator.m_globalBlendedNodeTransforms) {
        glm::vec3 origin = GetModelMatrix() * boneWorldMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        glm::vec3 right = glm::normalize(glm::vec3(boneWorldMatrix[0]));
        glm::vec3 up = glm::normalize(glm::vec3(boneWorldMatrix[1]));
        glm::vec3 forward = glm::normalize(glm::vec3(boneWorldMatrix[2]));

        DebugDraw::DrawLine(origin, origin + (forward * size), BLUE, false, exclusiveViewportIndex);
        DebugDraw::DrawLine(origin, origin + (up * size), GREEN, false, exclusiveViewportIndex);
        DebugDraw::DrawLine(origin, origin + (right * size), RED, false, exclusiveViewportIndex);
    }
}


bool AnimatedGameObject::AnimationByNameIsComplete(const std::string& name) {
    return m_animator.AnimationIsCompleteAnyLayer(name);
}


const glm::mat4& AnimatedGameObject::GetGlobalBlendedNodeTransfrom(const std::string& nodeName) {
    uint32_t nodeIndex = GetNodeIndex(nodeName);

    if (nodeIndex >= 0 && nodeIndex < (uint32_t)m_animator.m_globalBlendedNodeTransforms.size()) {
        return m_animator.m_globalBlendedNodeTransforms[nodeIndex];
    }
    static const glm::mat4 identity = glm::mat4(1.0f);
    return identity;
}


const glm::mat4 AnimatedGameObject::GetBoneWorldMatrixWithBoneOffset(const std::string& boneName) {
    const glm::mat4& globalBlendedNodeTransform = GetGlobalBlendedNodeTransfrom(boneName);
    const glm::mat4& boneOffset = m_skinnedModel->GetBoneOffset(boneName);

    return GetModelMatrix() * globalBlendedNodeTransform * boneOffset;
}


int32_t AnimatedGameObject::GetBoneIndex(const std::string& boneName) {
    if (!m_skinnedModel) return -1;
    return m_skinnedModel->GetBoneIndex(boneName);
}


int32_t AnimatedGameObject::GetNodeIndex(const std::string& nodeName) {
    if (!m_skinnedModel) return -1;
    return m_skinnedModel->GetNodeIndex(nodeName);
}


const glm::mat4 AnimatedGameObject::GetBoneWorldMatrix(const std::string& boneName) {
    int nodeIndex = GetNodeIndex(boneName);
    if (nodeIndex < 0 || nodeIndex >= int(m_animator.m_globalBlendedNodeTransforms.size())) {
        return glm::mat4(1.0f);
    }
    else {
        return GetModelMatrix() * m_animator.m_globalBlendedNodeTransforms[nodeIndex];
    }
}

const glm::vec3 AnimatedGameObject::GetBoneWorldPosition(const std::string& boneName) {
    return GetBoneWorldMatrix(boneName)[3];
}

void AnimatedGameObject::SetRagdollId(uint64_t RagdollId) {
    m_ragdollId = RagdollId;
}

void AnimatedGameObject::SetAdditiveTransform(const std::string& nodeName, const glm::mat4& matrix) {
    m_animator.SetAdditiveTransform(nodeName, matrix);
}

void AnimatedGameObject::PauseAllAnimationLayers() {
    m_animator.PauseAllLayers();
}
}
