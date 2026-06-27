#pragma once
#include "Legacy/Types/Animation/Animator.h"
#include "Hell/ResourceManagement/Types/SkinnedModel.h"
#include "Unloved/Objects/Renderables/AnimatedMeshNodes.h"
#include <unordered_map>

namespace Unloved {

struct AnimatedGameObject {
    enum class AnimationMode { BINDPOSE, ANIMATION, RAGDOLL_V2 };

public:
    AnimatedGameObject() = default;
    AnimatedGameObject(uint64_t id);
    AnimatedGameObject(const AnimatedGameObject&) = delete;
    AnimatedGameObject& operator=(const AnimatedGameObject&) = delete;
    AnimatedGameObject(AnimatedGameObject&&) noexcept = default;
    AnimatedGameObject& operator=(AnimatedGameObject&&) noexcept = default;
    ~AnimatedGameObject() = default;

    void CleanUp();
    void SetMeshWoundMaskTextureIndex(const std::string& meshName, int32_t woundMaskTextureIndex);
    void UpdateRenderItems();
    void Update(float deltaTime);
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
    void SetAnimationModeToAnimated();
    void SetAnimationModeToBindPose();
    void SetAnimationModeToRagdoll();


    void SetBlendingModeByMeshName(const std::string& meshName, BlendingMode blendingMode);
    void SetMeshMaterialByMeshName(const std::string& meshName, const std::string& materialName);
    void SetMeshMaterialByMeshIndex(int meshIndex, const std::string& materialName);
    void SetMeshToRenderAsGlassByMeshIndex(const std::string& materialName);
    void SetMeshFurLength(const std::string& meshName, float furLength);
    void SetMeshFurShellDistanceAttenuation(const std::string& meshName, float furShellDistanceAttenuation);
    void SetMeshFurUVScale(const std::string& meshName, float uvScale);
    void SetMeshEmissiveColorTextureByMeshName(const std::string& meshName, const std::string& textureName);
    void SetMeshWoundMaterialByMeshName(const std::string& meshName, const std::string& textureName);
    void SetAllMeshMaterials(const std::string& materialName);
    void SetAllMeshBlendingModes(BlendingMode blendingMode);

    void EnableModelMatrixOverride();
    void SetCameraMatrix(const glm::mat4& matrix);
    void DrawBones(int exclusiveViewportIndex = -1);
    void DrawBoneTangentVectors(float size = 0.1f, int exclusiveViewportIndex = -1);
    void SetExclusiveViewportIndex(int index);
    void SetIgnoredViewportIndex(int index);
    //void EnableDrawingForAllMesh();
    //void EnableDrawingForMeshByMeshName(const std::string& meshName);
    //void DisableDrawingForMeshByMeshName(const std::string& meshName);
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

    Animator& GetAnimator() { return m_animator; }

    // Sketchy, only used by shark currently
    const glm::vec3& GetPosition() const                                              { return m_transform.position;  }

    SkinnedModel* GetSkinnedModel()                                                   { return m_skinnedModel; }
    AnimatedMeshNodes& GetAnimatedMeshNodes()                                         { return m_animatedMeshNodes; }

    bool RenderingEnabled()                                                           { return m_animatedMeshNodes.RenderingEnabled(); }
    const uint64_t& GetObjectId() const                                               { return m_objectId; }
    const uint64_t& GetRagdollId() const                                            { return m_ragdollId; }
    const uint32_t GetBaseTransfromIndex() const                                      { return baseTransformIndex; }
    const uint32_t& GetIgnoredViewportIndex() const                                   { return m_animatedMeshNodes.GetIgnoredViewportIndex(); };
    const uint32_t& GetExclusiveViewportIndex() const                                 { return m_animatedMeshNodes.GetExclusiveViewportIndex(); };
    const glm::vec3 GetScale() const                                                  { return m_transform.scale; }
    const std::vector<RenderItem>& GetDeformingRenderItems() const                    { return m_animatedMeshNodes.m_deformingRenderItems; }
    const std::vector<RenderItem>& GetNonDeformingRenderItems() const                 { return m_animatedMeshNodes.m_nonDeformingRenderItems; }
    const std::vector<RenderItem>& GetNonDeformingRenderItemsDepthPeeledTransparent() { return m_animatedMeshNodes.m_nonDeformingRenderItemsDepthPeeledTransparent; }
    const std::vector<glm::mat4>& GetGlobalBlendedNodeTransforms()                    { return m_animator.m_globalBlendedNodeTransforms; }
    const std::vector<glm::mat4>& GetBoneSkinningMatrices()                           { return m_boneSkinningMatrices; }
    const std::string& GetName() const                                                { return m_name; }
    const glm::mat4 GetModelMatrixOverride() const                                    { return m_modelMatrixOverride; }


private:
    void UpdateBoneTransformsFromRagdoll();
    void SyncRagdollToAnimation();

    AnimationMode m_animationMode = AnimationMode::BINDPOSE;
    Animator m_animator;
    SkinnedModel* m_skinnedModel = nullptr;
    Hell::Transform m_transform;
    glm::mat4 m_modelMatrixOverride = glm::mat4(1);
    std::string m_name = "";

    AnimatedMeshNodes m_animatedMeshNodes;

    std::vector<glm::mat4> m_boneSkinningMatrices;
    uint64_t m_objectId = 0;
    uint64_t m_ragdollId = 0;
    uint32_t baseTransformIndex = -1;
    bool m_useModelMatrixOverride = false;
};
}
