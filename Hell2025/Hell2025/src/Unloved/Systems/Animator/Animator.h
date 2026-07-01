#pragma once

#include "Unloved/Systems/Animator/AnimatorTypes.h"

#include <cstdint>
#include <string>
#include <vector>

namespace Unloved::Animator {
    void Clear(AnimationState& state);
    void ClearAllAnimations(AnimationState& state);
    void SetSkinnedModel(AnimationState& state, SkinnedModel* skinnedModel);
    void PlayAnimation(AnimationState& state, const std::string& layerName, const std::string& animationName, float speed = 1.0f, bool loop = false);
    void PlayAnimation(AnimationState& state, const std::string& layerName, const std::string& animationName, const AnimationPlayOptions& options);
    void PlayAndLoopAnimation(AnimationState& state, const std::string& layerName, const std::string& animationName, float speed = 1.0f);
    void CrossFade(AnimationState& state, const std::string& layerName, const std::string& animationName, float fadeDuration, float speed = 1.0f, bool loop = false);
    void FadeOutLayer(AnimationState& state, const std::string& layerName, float fadeDuration);
    void SetLayerWeight(AnimationState& state, const std::string& layerName, float weight);
    void SetLayerBlendMode(AnimationState& state, const std::string& layerName, AnimationBlendMode blendMode);
    void SetLayerNodeWeights(AnimationState& state, const std::string& layerName, const std::vector<float>& nodeWeights);
    void Update(AnimationState& state, float deltaTime);
    void UpdateBoneSkinningMatrices(AnimationState& state);
    void PauseAllLayers(AnimationState& state);
    void SetAdditiveTransform(AnimationState& state, const std::string& nodeName, const glm::mat4& matrix);

    uint32_t GetAnimationFrameNumber(const AnimationState& state, const std::string& animationLayerName);
    bool AnimationIsPastFrameNumber(const AnimationState& state, const std::string& animationLayerName, int frameNumber);
    bool AnimationIsCompleteAnyLayer(const AnimationState& state, const std::string& animationName);
    bool AllAnimationsComplete(const AnimationState& state);
}
