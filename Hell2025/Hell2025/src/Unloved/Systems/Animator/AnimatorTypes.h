#pragma once

#include "Hell/Transform.h"

#include <glm/mat4x4.hpp>

#include <string>
#include <unordered_map>
#include <vector>

struct Animation;
struct SkinnedModel;

namespace Unloved {

enum class AnimationBlendMode {
    Override,
    Additive
};

struct AnimationPlayOptions {
    float speed = 1.0f;
    float fadeInDuration = 0.0f;
    float fadeOutDuration = 0.0f;
    bool loop = false;
    bool restartIfAlreadyPlaying = true;
};

struct AnimationPlayback {
    Animation* animation = nullptr;
    float time = 0.0f;
    float speed = 1.0f;
    float weight = 1.0f;
    float targetWeight = 1.0f;
    float fadeStartWeight = 1.0f;
    float fadeTime = 0.0f;
    float fadeDuration = 0.0f;
    bool loop = false;
    bool paused = false;
    bool complete = false;
    bool fadingOut = false;
    std::vector<Hell::QuatTransform> sampledPose;
};

struct AnimationLayerState {
    std::string name;
    AnimationBlendMode blendMode = AnimationBlendMode::Override;
    float weight = 1.0f;
    std::vector<float> nodeWeights;
    std::vector<AnimationPlayback> playbacks;
    std::vector<Hell::QuatTransform> blendedPose;
    std::vector<Hell::QuatTransform> localNodeTransforms;
    std::vector<Hell::QuatTransform> globalNodeTransforms;
};

struct AnimationState {
    SkinnedModel* skinnedModel = nullptr;
    std::vector<AnimationLayerState> layers;
    std::vector<Hell::QuatTransform> localNodePose;
    std::vector<glm::mat4> globalNodeTransforms;
    std::vector<glm::mat4> boneSkinningMatrices;
    std::unordered_map<std::string, glm::mat4> additiveNodeTransforms;
};

}
