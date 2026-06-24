#pragma once
#include "Types/Renderer/SkinnedModel.h"
#include <string>
#include <vector>
#include <unordered_map>

#include "Hell/ResourceManagement/Types/Animation.h"

struct AnimationLayer {
    Animation* m_animation;
    std::vector<AnimatedTransform> m_globalNodeTransforms;
    std::vector<AnimatedTransform> m_localNodeTransforms;
    std::vector<float> m_boneWeights;
    float m_AnimationWeight = 1.0f;
    float m_currentTime = 0.0f;
    float m_animationSpeed = 1.0f;
    bool m_loop = true;
    bool m_paused = false;
    bool m_isComplete = false;
};

struct Animator {
    void SetSkinnedModel(const std::string& skinnedModelName);
    void PlayAnimation(const std::string& layerName, const std::string& animationName, float speed = 1.0f, bool loop = false);
    void PlayAndLoopAnimation(const std::string& layerName, const std::string& animationName, float speed = 1.0f);
    void UpdateAnimations(float deltaTime);
    void CreateAnimationLayer(const std::string& name);
    void ClearAllAnimations();
    void PauseAllLayers();
    void SetAdditiveTransform(const std::string& nodeName, const glm::mat4& matrix);

    uint32_t GetAnimationFrameNumber(const std::string& animationLayerName);
    bool AnimationIsPastFrameNumber(const std::string& animationLayerName, int frameNumber);
    bool AnimationLayerExists(const std::string& name) const;
    bool AnimationIsCompleteAnyLayer(const std::string& animationName);
    bool AllAnimationsComplete();
    
    AnimationLayer* GetAnimationLayerByName(const std::string& name);

    std::vector<glm::mat4> m_LocalBlendedBoneTransforms;
    std::vector<glm::mat4> m_globalBlendedNodeTransforms;

    std::unordered_map<std::string, glm::mat4> m_additiveNodeTransforms; // Used for shark spine

    std::unordered_map<std::string, AnimationLayer> m_animationLayers;
    //std::vector<AnimationState> m_animationStates;

private:
    void UpdateAnimation(AnimationLayer& animationState, float deltaTime);
    float GetAnimationTimeInTicks(AnimationLayer& animationState);
    const AnimatedNode* FindAnimatedNode(Animation* animation, const char* NodeName);

    SkinnedModel* m_skinnedModel = nullptr;
};