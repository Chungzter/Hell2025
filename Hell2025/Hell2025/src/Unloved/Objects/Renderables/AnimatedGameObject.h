#pragma once
#include "Hell/Math/AABB.h"
#include "Hell/ResourceManagement/Types/SkinnedModel.h"

#include "Unloved/Objects/Renderables/AnimatedMeshNodes.h"
#include "Unloved/Objects/Renderables/BoneSegment.h"
#include "Unloved/Systems/Animator/Animator.h"

#include <unordered_map>

namespace Unloved {

struct AnimatedGameObject {
    enum class AnimationMode { BINDPOSE, ANIMATION, RAGDOLL_V2 };

    AnimatedGameObject() = default;
    AnimatedGameObject(uint64_t id);
    AnimatedGameObject(const AnimatedGameObject&) = delete;
    AnimatedGameObject& operator=(const AnimatedGameObject&) = delete;
    AnimatedGameObject(AnimatedGameObject&&) noexcept = default;
    AnimatedGameObject& operator=(AnimatedGameObject&&) noexcept = default;
    ~AnimatedGameObject() = default;

    void CleanUp();
    void UpdateRenderItems();
    void Update(float deltaTime);
    void EvaluateAnimation(float deltaTime);
    void FinalizeAnimation();
    void SetName(std::string name);
    void SetSkinnedModel(const std::string& skinnedModelName, const std::string& presetName = UNDEFINED_STRING);
    void SetScale(float scale);
    void SetPosition(glm::vec3 position);
    void SetRotationX(float rotation);
    void SetRotationY(float rotation);
    void SetRotationZ(float rotation);
    void PlayAnimation(const std::string& layerName, const std::string& animationName, float speed);
    void PlayAnimation(const std::string& layerName, std::vector<std::string>& animationNames, float speed);
    void PlayAndLoopAnimation(const std::string& layerName, const std::string& animationName, float speed);
    void PlayAndLoopAnimation(const std::string& layerName, std::vector<std::string>& animationNames, float speed);
    void CrossFadeAnimation(const std::string& layerName, const std::string& animationName, float fadeDuration, float speed = 1.0f, bool loop = false);
    void FadeOutAnimationLayer(const std::string& layerName, float fadeDuration);
    void SetAnimationLayerWeight(const std::string& layerName, float weight);
    void SetAnimationLayerBlendMode(const std::string& layerName, AnimationBlendMode blendMode);
    void SetAnimationLayerNodeWeights(const std::string& layerName, const std::vector<float>& nodeWeights);
    void SetAnimationModeToAnimated();
    void SetAnimationModeToBindPose();
    void SetAnimationModeToRagdoll();
    void SetBlendingModeByMeshName(const std::string& meshName, BlendingMode blendingMode);
    void SetMeshMaterialByMeshName(const std::string& meshName, const std::string& materialName);
    void SetMeshMaterialByMeshIndex(int meshIndex, const std::string& materialName);
    void SetMeshWoundMaterialByMeshName(const std::string& meshName, const std::string& textureName);
    void SetMeshWoundMaskArrayIndex(const std::string& meshName, int32_t woundMaskArrayIndex);
    void SetAllMeshMaterials(const std::string& materialName);
    void SetAllMeshBlendingModes(BlendingMode blendingMode);
    void SetExcludeFromVulkanTLAS(bool exclude);

    void EnableModelMatrixOverride();
    void SetCameraMatrix(const glm::mat4& matrix);
    void DrawBones(int exclusiveViewportIndex = -1);
    void DrawBoneTangentVectors(float size = 0.1f, int exclusiveViewportIndex = -1);
    void SetExclusiveViewportIndex(int index);
    void SetIgnoredViewportIndex(int index);
    void PrintNodeNames();
    void PrintMeshNames();
    void SetAdditiveTransform(const std::string& nodeName, const glm::mat4& matrix);
    void PauseAllAnimationLayers();
    void SetRagdollId(uint64_t RagdollId);

    bool AnimationIsPastFrameNumber(const std::string& animationLayerName, int frameNumber);
    bool AnimationByNameIsComplete(const std::string& name);
    bool IsAllAnimationsComplete();
    void EnableRendering();
    void DisableRendering();

    void EnableShadows() { m_castsShadows = true; }
    void DisableShadows() { m_castsShadows = false; }

    const glm::mat4 GetModelMatrix();
    const glm::mat4 GetBoneWorldMatrixWithBoneOffset(const std::string& boneName);
    const glm::mat4& GetGlobalBlendedNodeTransfrom(const std::string& nodeName);        // This is busted almost certainly.

    const glm::mat4& GetInverseBindTransformByBoneName(const std::string& name);        // potentially sketchy or incorrectly named
    const glm::mat4& GetAnimatedTransformByBoneName(const std::string& name);           // potentially sketchy or incorrectly named
    const glm::mat4& GetAnimatedTransformByNodeIndex(int32_t nodeIndex);                // potentially sketchy or incorrectly named
    const glm::mat4 GetBoneWorldMatrix(const std::string& boneName);                    // potentially sketchy or incorrectly named
    const glm::vec3 GetBoneWorldPosition(const std::string& boneName);                  // potentially sketchy or incorrectly named

    const uint32_t GetAnimationFrameNumber(const std::string& animationLayerName);
    const uint32_t GetVerteXCount();

    int32_t GetBoneIndex(const std::string& boneName);
    int32_t GetNodeIndex(const std::string& nodeName);

    bool CastsShadows() const                                                         { return m_castsShadows; }
    AnimationState& GetAnimationState()                                               { return m_animationState; }

    // Sketchy, only used by shark currently
    const glm::vec3& GetPosition() const                                              { return m_transform.position;  }

    SkinnedModel* GetSkinnedModel()                                                   { return m_skinnedModel; }
    AnimatedMeshNodes& GetAnimatedMeshNodes()                                         { return m_animatedMeshNodes; }

    bool RenderingEnabled()                                                           { return m_animatedMeshNodes.RenderingEnabled(); }
    const uint64_t& GetObjectId() const                                               { return m_objectId; }
    const uint64_t& GetRagdollId() const                                              { return m_ragdollId; }
    const uint32_t GetBaseTransfromIndex() const                                      { return baseTransformIndex; }
    const uint32_t& GetIgnoredViewportIndex() const                                   { return m_animatedMeshNodes.GetIgnoredViewportIndex(); };
    const uint32_t& GetExclusiveViewportIndex() const                                 { return m_animatedMeshNodes.GetExclusiveViewportIndex(); };
    const glm::vec3 GetScale() const                                                  { return m_transform.scale; }
    const std::vector<RenderItem>& GetDeformingRenderItems() const                    { return m_animatedMeshNodes.m_deformingRenderItems; }
    const std::vector<RenderItem>& GetNonDeformingRenderItems() const                 { return m_animatedMeshNodes.m_nonDeformingRenderItems; }
    const std::vector<RenderItem>& GetNonDeformingRenderItemsDepthPeeledTransparent() { return m_animatedMeshNodes.m_nonDeformingRenderItemsDepthPeeledTransparent; }
    const std::vector<glm::mat4>& GetGlobalBlendedNodeTransforms()                    { return m_animationState.globalNodeTransforms; }
    const std::vector<glm::mat4>& GetBoneSkinningMatrices()                           { return m_animationState.boneSkinningMatrices; }
    const std::vector<glm::mat4>& GetPreviousRenderBoneSkinningMatrices() const        { return m_previousRenderBoneSkinningMatrices; }
    const glm::mat4& GetPreviousRenderModelMatrix() const                              { return m_previousRenderModelMatrix; }
    bool HasRenderPoseHistory() const                                                  { return m_hasRenderPoseHistory; }
    void CommitRenderPoseHistory();
    const std::string& GetName() const                                                { return m_name; }
    const std::string& GetEditorName() const                                          { return m_name; }
    const glm::mat4 GetModelMatrixOverride() const                                    { return m_modelMatrixOverride; }
    const AABB& GetSkinnedAABB() const                                                { return m_skinnedAABB; }

private:
    void UpdateBoneTransformsFromRagdoll();
    void SyncRagdollToAnimation();
    void ComputeBoneSegments();
    void CalculateSkinnedAABB();
    void UpdateDirtyBounds();

    AnimationMode m_animationMode = AnimationMode::BINDPOSE;
    AnimationState m_animationState;
    SkinnedModel* m_skinnedModel = nullptr;
    Hell::Transform m_transform;
    glm::mat4 m_modelMatrixOverride = glm::mat4(1);
    std::string m_name = "";

    AnimatedMeshNodes m_animatedMeshNodes;

    uint64_t m_objectId = 0;
    uint64_t m_ragdollId = 0;
    uint32_t baseTransformIndex = -1;
    bool m_useModelMatrixOverride = false;
    bool m_castsShadows = true;

    std::vector<BoneSegment> m_boneSegments;
    std::vector<glm::mat4> m_previousRenderBoneSkinningMatrices;
    glm::mat4 m_previousRenderModelMatrix = glm::mat4(1.0f);
    bool m_hasRenderPoseHistory = false;
    AABB m_skinnedAABB;
    AABB m_skinnedAABBLastFrame;
    float m_skinnedAABBThreshold = 0.1f;
    float m_skinnedAABBChangeThreshold = 0.01f;
    bool m_hasSkinnedAABBLastFrame = false;
};
}
