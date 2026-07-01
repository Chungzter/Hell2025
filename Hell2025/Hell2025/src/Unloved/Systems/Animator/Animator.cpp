#include "Animator.h"

#include "Hell/Math/Math.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/ResourceManagement/Types/Animation.h"
#include "Hell/ResourceManagement/Types/SkinnedModel.h"

#include <algorithm>
#include <cmath>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Unloved::Animator {

    namespace {
        constexpr float MIN_ANIMATION_WEIGHT = 0.00001f;

        AnimationLayerState* FindLayer(AnimationState& state, const std::string& name) {
            for (AnimationLayerState& layer : state.layers) {
                if (layer.name == name) {
                    return &layer;
                }
            }
            return nullptr;
        }

        const AnimationLayerState* FindLayer(const AnimationState& state, const std::string& name) {
            for (const AnimationLayerState& layer : state.layers) {
                if (layer.name == name) {
                    return &layer;
                }
            }
            return nullptr;
        }

        AnimationLayerState& GetOrCreateLayer(AnimationState& state, const std::string& name) {
            if (AnimationLayerState* layer = FindLayer(state, name)) {
                return *layer;
            }

            AnimationLayerState& layer = state.layers.emplace_back();
            layer.name = name;
            layer.weight = 1.0f;

            if (state.skinnedModel) {
                const int nodeCount = state.skinnedModel->GetNodeCount();
                layer.nodeWeights.assign(nodeCount, 1.0f);
            }

            return layer;
        }

        float Clamp01(float value) {
            return std::clamp(value, 0.0f, 1.0f);
        }

        float AnimationDurationSeconds(const Animation* animation) {
            if (!animation) {
                return 0.0f;
            }

            const float ticksPerSecond = animation->GetTicksPerSecond();
            if (ticksPerSecond <= 0.0f) {
                return 0.0f;
            }

            return animation->m_duration / ticksPerSecond;
        }

        Hell::QuatTransform BindPoseForNode(const SkinnedModel& skinnedModel, int nodeIndex) {
            return Hell::QuatTransform(skinnedModel.m_nodes[nodeIndex].inverseBindTransform);
        }

        Hell::QuatTransform BlendTransform(const Hell::QuatTransform& from, const Hell::QuatTransform& to, float factor) {
            factor = Clamp01(factor);

            if (factor <= 0.0f) {
                return from;
            }

            if (factor >= 1.0f) {
                return to;
            }

            Hell::QuatTransform result;
            result.translation = from.translation + (to.translation - from.translation) * factor;
            result.scale = from.scale + (to.scale - from.scale) * factor;

            glm::quat targetRotation = to.rotation;
            if (glm::dot(from.rotation, targetRotation) < 0.0f) {
                targetRotation = -targetRotation;
            }

            result.rotation = glm::normalize(glm::slerp(from.rotation, targetRotation, factor));
            return result;
        }

        Hell::QuatTransform ApplyAdditiveTransform(const Hell::QuatTransform& base, const Hell::QuatTransform& bindPose, const Hell::QuatTransform& additivePose, float weight) {
            weight = Clamp01(weight);

            Hell::QuatTransform result = base;
            result.translation += (additivePose.translation - bindPose.translation) * weight;
            result.scale += (additivePose.scale - bindPose.scale) * weight;

            glm::quat rotationDelta = additivePose.rotation * glm::inverse(bindPose.rotation);
            rotationDelta = glm::normalize(rotationDelta);

            if (glm::dot(glm::quat(1.0f, 0.0f, 0.0f, 0.0f), rotationDelta) < 0.0f) {
                rotationDelta = -rotationDelta;
            }

            glm::quat weightedDelta = glm::normalize(glm::slerp(glm::quat(1.0f, 0.0f, 0.0f, 0.0f), rotationDelta, weight));
            result.rotation = glm::normalize(weightedDelta * result.rotation);
            return result;
        }

        const AnimationPlayback* GetPrimaryPlayback(const AnimationLayerState& layer) {
            const AnimationPlayback* bestPlayback = nullptr;

            for (const AnimationPlayback& playback : layer.playbacks) {
                if (!playback.animation) {
                    continue;
                }

                if (!bestPlayback) {
                    bestPlayback = &playback;
                    continue;
                }

                if (bestPlayback->fadingOut && !playback.fadingOut) {
                    bestPlayback = &playback;
                    continue;
                }

                if (bestPlayback->fadingOut == playback.fadingOut && playback.weight > bestPlayback->weight) {
                    bestPlayback = &playback;
                }
            }

            return bestPlayback;
        }

        const AnimatedNode* FindAnimatedNode(Animation* animation, const std::string& nodeName) {
            if (!animation) {
                return nullptr;
            }

            for (const AnimatedNode& animatedNode : animation->m_animatedNodes) {
                if (animatedNode.m_nodeName == nodeName) {
                    return &animatedNode;
                }
            }
            return nullptr;
        }

        int FindAnimatedNodeIndex(float animationTime, const AnimatedNode* animatedNode) {
            if (!animatedNode || animatedNode->m_nodeKeys.empty()) {
                return -1;
            }

            if (animationTime < animatedNode->m_nodeKeys[0].timeStamp) {
                return -1;
            }

            for (int i = 1; i < animatedNode->m_nodeKeys.size(); i++) {
                if (animationTime < animatedNode->m_nodeKeys[i].timeStamp) {
                    return i - 1;
                }
            }

            return static_cast<int>(animatedNode->m_nodeKeys.size()) - 1;
        }

        glm::vec3 SamplePosition(float animationTime, const AnimatedNode* animatedNode) {
            if (!animatedNode || animatedNode->m_nodeKeys.empty()) {
                return glm::vec3(0.0f);
            }

            const int index = FindAnimatedNodeIndex(animationTime, animatedNode);
            const int nextIndex = index + 1;

            if (index == -1 || animatedNode->m_nodeKeys.size() == 1) {
                return animatedNode->m_nodeKeys[0].positon;
            }

            if (nextIndex == animatedNode->m_nodeKeys.size()) {
                return animatedNode->m_nodeKeys[index].positon;
            }

            const float deltaTime = animatedNode->m_nodeKeys[nextIndex].timeStamp - animatedNode->m_nodeKeys[index].timeStamp;
            const float factor = (animationTime - animatedNode->m_nodeKeys[index].timeStamp) / deltaTime;
            const glm::vec3 start = animatedNode->m_nodeKeys[index].positon;
            const glm::vec3 end = animatedNode->m_nodeKeys[nextIndex].positon;
            return start + factor * (end - start);
        }

        glm::vec3 SampleScale(float animationTime, const AnimatedNode* animatedNode) {
            if (!animatedNode || animatedNode->m_nodeKeys.empty()) {
                return glm::vec3(1.0f);
            }

            const int index = FindAnimatedNodeIndex(animationTime, animatedNode);
            const int nextIndex = index + 1;

            if (index == -1 || animatedNode->m_nodeKeys.size() == 1) {
                return animatedNode->m_nodeKeys[0].scale;
            }

            if (nextIndex == animatedNode->m_nodeKeys.size()) {
                return animatedNode->m_nodeKeys[index].scale;
            }

            const float deltaTime = animatedNode->m_nodeKeys[nextIndex].timeStamp - animatedNode->m_nodeKeys[index].timeStamp;
            const float factor = (animationTime - animatedNode->m_nodeKeys[index].timeStamp) / deltaTime;
            const glm::vec3 start = animatedNode->m_nodeKeys[index].scale;
            const glm::vec3 end = animatedNode->m_nodeKeys[nextIndex].scale;
            return start + factor * (end - start);
        }

        glm::quat SampleRotation(float animationTime, const AnimatedNode* animatedNode) {
            if (!animatedNode || animatedNode->m_nodeKeys.empty()) {
                return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
            }

            const int index = FindAnimatedNodeIndex(animationTime, animatedNode);
            const int nextIndex = index + 1;

            if (index == -1 || animatedNode->m_nodeKeys.size() == 1) {
                return animatedNode->m_nodeKeys[0].rotation;
            }

            if (nextIndex == animatedNode->m_nodeKeys.size()) {
                return animatedNode->m_nodeKeys[index].rotation;
            }

            const float deltaTime = animatedNode->m_nodeKeys[nextIndex].timeStamp - animatedNode->m_nodeKeys[index].timeStamp;
            const float factor = (animationTime - animatedNode->m_nodeKeys[index].timeStamp) / deltaTime;
            const glm::quat& start = animatedNode->m_nodeKeys[index].rotation;
            glm::quat end = animatedNode->m_nodeKeys[nextIndex].rotation;

            if (glm::dot(start, end) < 0.0f) {
                end = -end;
            }

            return glm::normalize(glm::slerp(start, end, factor));
        }

        float GetAnimationTimeInTicks(const AnimationPlayback& playback) {
            if (!playback.animation) {
                return 0.0f;
            }

            if (playback.animation->m_duration <= 0.0f) {
                return 0.0f;
            }

            const float ticksPerSecond = playback.animation->m_ticksPerSecond != 0.0f ? playback.animation->m_ticksPerSecond : 25.0f;
            float timeInTicks = playback.time * ticksPerSecond;

            if (timeInTicks == playback.animation->m_duration) {
                return timeInTicks;
            }

            timeInTicks = std::fmod(timeInTicks, playback.animation->m_duration);
            return std::min(timeInTicks, playback.animation->m_duration);
        }

        void UpdateFade(AnimationPlayback& playback, float deltaTime) {
            if (playback.fadeDuration <= 0.0f) {
                playback.weight = playback.targetWeight;
                return;
            }

            playback.fadeTime += deltaTime;

            const float factor = Clamp01(playback.fadeTime / playback.fadeDuration);
            playback.weight = playback.fadeStartWeight + (playback.targetWeight - playback.fadeStartWeight) * factor;

            if (factor >= 1.0f) {
                playback.fadeTime = 0.0f;
                playback.fadeDuration = 0.0f;
                playback.fadeStartWeight = playback.weight;

                if (playback.fadingOut) {
                    playback.weight = 0.0f;
                    playback.complete = true;
                    playback.paused = true;
                }
            }
        }

        void AdvancePlayback(AnimationPlayback& playback, float deltaTime) {
            if (!playback.animation) {
                return;
            }

            if (!playback.paused) {
                playback.time += deltaTime * playback.speed;
            }

            const float duration = AnimationDurationSeconds(playback.animation);
            if (duration > 0.0f && playback.time >= duration) {
                if (!playback.loop) {
                    playback.time = duration;
                    playback.paused = true;
                    playback.complete = true;
                }
                else {
                    playback.time = 0.0f;
                }
            }

            UpdateFade(playback, deltaTime);
        }

        void SamplePlaybackPose(const SkinnedModel& skinnedModel, AnimationPlayback& playback) {
            if (!playback.animation) {
                return;
            }

            const int nodeCount = static_cast<int>(skinnedModel.m_nodes.size());
            playback.sampledPose.resize(nodeCount);

            const float timeInTicks = GetAnimationTimeInTicks(playback);

            for (int i = 0; i < skinnedModel.m_nodes.size(); i++) {
                glm::mat4 nodeTransformation = glm::mat4(1.0f);
                const std::string& nodeName = skinnedModel.m_nodes[i].name;
                const AnimatedNode* animatedNode = FindAnimatedNode(playback.animation, nodeName);

                if (animatedNode) {
                    const glm::vec3 translation = SamplePosition(timeInTicks, animatedNode);
                    const glm::quat rotation = SampleRotation(timeInTicks, animatedNode);
                    const glm::vec3 scale = SampleScale(timeInTicks, animatedNode);
                    nodeTransformation = glm::translate(glm::mat4(1.0f), translation) * glm::mat4(rotation);
                    nodeTransformation = glm::scale(nodeTransformation, scale);
                }

                playback.sampledPose[i] = Hell::QuatTransform(nodeTransformation);
            }
        }

        void BlendPlaybackPosesIntoLayer(const SkinnedModel& skinnedModel, AnimationLayerState& layer) {
            const int nodeCount = static_cast<int>(skinnedModel.m_nodes.size());
            layer.blendedPose.assign(nodeCount, Hell::QuatTransform());
            std::vector<float> blendedWeight(nodeCount, 0.0f);

            for (const AnimationPlayback& playback : layer.playbacks) {
                if (!playback.animation || playback.weight <= MIN_ANIMATION_WEIGHT || playback.sampledPose.size() != nodeCount) {
                    continue;
                }

                for (int i = 0; i < nodeCount; i++) {
                    const float newWeightSum = blendedWeight[i] + playback.weight;
                    const float blendFactor = playback.weight / newWeightSum;

                    if (blendedWeight[i] <= MIN_ANIMATION_WEIGHT) {
                        layer.blendedPose[i] = playback.sampledPose[i];
                    }
                    else {
                        layer.blendedPose[i] = BlendTransform(layer.blendedPose[i], playback.sampledPose[i], blendFactor);
                    }

                    blendedWeight[i] = newWeightSum;
                }
            }

            for (int i = 0; i < nodeCount; i++) {
                if (blendedWeight[i] <= MIN_ANIMATION_WEIGHT) {
                    layer.blendedPose[i] = BindPoseForNode(skinnedModel, i);
                }
            }

            layer.localNodeTransforms = layer.blendedPose;
            layer.globalNodeTransforms.resize(nodeCount);

            for (int i = 0; i < nodeCount; i++) {
                const glm::mat4 localMatrix = layer.blendedPose[i].to_mat4();
                const int parent = skinnedModel.m_nodes[i].parentIndex;
                const glm::mat4 globalMatrix = parent >= 0 ? layer.globalNodeTransforms[parent].to_mat4() * localMatrix : localMatrix;
                layer.globalNodeTransforms[i] = Hell::QuatTransform(globalMatrix);
            }
        }

        void UpdateLayer(AnimationState& state, AnimationLayerState& layer, float deltaTime) {
            if (!state.skinnedModel || layer.playbacks.empty()) {
                return;
            }

            for (AnimationPlayback& playback : layer.playbacks) {
                AdvancePlayback(playback, deltaTime);
            }

            layer.playbacks.erase(std::remove_if(layer.playbacks.begin(), layer.playbacks.end(), [](const AnimationPlayback& playback) {
                return playback.fadingOut && playback.weight <= MIN_ANIMATION_WEIGHT;
            }), layer.playbacks.end());

            for (AnimationPlayback& playback : layer.playbacks) {
                SamplePlaybackPose(*state.skinnedModel, playback);
            }

            BlendPlaybackPosesIntoLayer(*state.skinnedModel, layer);
        }

        void FadeOutPlayback(AnimationPlayback& playback, float fadeDuration) {
            playback.fadingOut = true;
            playback.targetWeight = 0.0f;
            playback.fadeStartWeight = playback.weight;
            playback.fadeTime = 0.0f;
            playback.fadeDuration = fadeDuration;

            if (fadeDuration <= 0.0f) {
                playback.weight = 0.0f;
                playback.complete = true;
                playback.paused = true;
            }
        }
    }

    void Clear(AnimationState& state) {
        state.layers.clear();
        state.localNodePose.clear();
        state.globalNodeTransforms.clear();
        state.boneSkinningMatrices.clear();
        state.additiveNodeTransforms.clear();
    }

    void ClearAllAnimations(AnimationState& state) {
        state.layers.clear();
    }

    void SetSkinnedModel(AnimationState& state, SkinnedModel* skinnedModel) {
        Clear(state);
        state.skinnedModel = skinnedModel;
    }

    void PlayAnimation(AnimationState& state, const std::string& layerName, const std::string& animationName, float speed, bool loop) {
        AnimationPlayOptions options;
        options.speed = speed;
        options.loop = loop;
        PlayAnimation(state, layerName, animationName, options);
    }

    void PlayAnimation(AnimationState& state, const std::string& layerName, const std::string& animationName, const AnimationPlayOptions& options) {
        Animation* animation = Hell::ResourceManager::GetAnimationPtr(animationName);
        if (!animation || !state.skinnedModel) {
            return;
        }

        AnimationLayerState& layer = GetOrCreateLayer(state, layerName);

        if (!layer.playbacks.empty() && options.loop && layer.playbacks.front().animation == animation && !layer.playbacks.front().fadingOut) {
            return;
        }

        if (!options.restartIfAlreadyPlaying) {
            for (const AnimationPlayback& playback : layer.playbacks) {
                if (playback.animation == animation && !playback.fadingOut) {
                    return;
                }
            }
        }

        const int nodeCount = state.skinnedModel->GetNodeCount();
        if (layer.nodeWeights.size() != nodeCount) {
            layer.nodeWeights.assign(nodeCount, 1.0f);
        }

        layer.blendedPose.resize(nodeCount);
        layer.localNodeTransforms.resize(nodeCount);
        layer.globalNodeTransforms.resize(nodeCount);

        const float fadeInDuration = std::max(0.0f, options.fadeInDuration);
        const float fadeOutDurationOption = std::max(0.0f, options.fadeOutDuration);
        const bool shouldCrossFade = fadeInDuration > 0.0f || fadeOutDurationOption > 0.0f;
        if (!shouldCrossFade) {
            layer.playbacks.clear();
        }
        else {
            const float fadeOutDuration = fadeOutDurationOption > 0.0f ? fadeOutDurationOption : fadeInDuration;
            for (AnimationPlayback& playback : layer.playbacks) {
                FadeOutPlayback(playback, fadeOutDuration);
            }
        }

        AnimationPlayback& playback = layer.playbacks.emplace_back();
        playback.animation = animation;
        playback.speed = options.speed;
        playback.weight = fadeInDuration > 0.0f ? 0.0f : 1.0f;
        playback.targetWeight = 1.0f;
        playback.fadeStartWeight = playback.weight;
        playback.fadeTime = 0.0f;
        playback.fadeDuration = fadeInDuration;
        playback.time = 0.0f;
        playback.loop = options.loop;
        playback.paused = false;
        playback.complete = false;
        playback.fadingOut = false;
        playback.sampledPose.resize(nodeCount);
    }

    void PlayAndLoopAnimation(AnimationState& state, const std::string& layerName, const std::string& animationName, float speed) {
        PlayAnimation(state, layerName, animationName, speed, true);
    }

    void CrossFade(AnimationState& state, const std::string& layerName, const std::string& animationName, float fadeDuration, float speed, bool loop) {
        const float clampedFadeDuration = std::max(0.0f, fadeDuration);
        AnimationPlayOptions options;
        options.speed = speed;
        options.fadeInDuration = clampedFadeDuration;
        options.fadeOutDuration = clampedFadeDuration;
        options.loop = loop;
        PlayAnimation(state, layerName, animationName, options);
    }

    void FadeOutLayer(AnimationState& state, const std::string& layerName, float fadeDuration) {
        AnimationLayerState* layer = FindLayer(state, layerName);
        if (!layer) {
            return;
        }

        fadeDuration = std::max(0.0f, fadeDuration);
        for (AnimationPlayback& playback : layer->playbacks) {
            FadeOutPlayback(playback, fadeDuration);
        }
    }

    void SetLayerWeight(AnimationState& state, const std::string& layerName, float weight) {
        AnimationLayerState& layer = GetOrCreateLayer(state, layerName);
        layer.weight = Clamp01(weight);
    }

    void SetLayerBlendMode(AnimationState& state, const std::string& layerName, AnimationBlendMode blendMode) {
        AnimationLayerState& layer = GetOrCreateLayer(state, layerName);
        layer.blendMode = blendMode;
    }

    void SetLayerNodeWeights(AnimationState& state, const std::string& layerName, const std::vector<float>& nodeWeights) {
        AnimationLayerState& layer = GetOrCreateLayer(state, layerName);
        layer.nodeWeights = nodeWeights;
        for (float& nodeWeight : layer.nodeWeights) {
            nodeWeight = Clamp01(nodeWeight);
        }
    }

    void Update(AnimationState& state, float deltaTime) {
        if (!state.skinnedModel) {
            return;
        }

        for (AnimationLayerState& layer : state.layers) {
            UpdateLayer(state, layer, deltaTime);
        }

        const int nodeCount = state.skinnedModel->GetNodeCount();
        std::vector<Hell::QuatTransform> finalLocals(nodeCount);
        std::vector<Hell::QuatTransform> bindPose(nodeCount);

        for (int i = 0; i < nodeCount; i++) {
            bindPose[i] = BindPoseForNode(*state.skinnedModel, i);
            finalLocals[i] = bindPose[i];
        }

        for (AnimationLayerState& layer : state.layers) {
            if (layer.weight <= MIN_ANIMATION_WEIGHT || layer.playbacks.empty() || layer.blendedPose.size() != nodeCount) {
                continue;
            }

            for (int i = 0; i < nodeCount; i++) {
                const float nodeWeight = i < layer.nodeWeights.size() ? layer.nodeWeights[i] : 1.0f;
                const float weight = Clamp01(layer.weight * nodeWeight);

                if (weight <= MIN_ANIMATION_WEIGHT) {
                    continue;
                }

                if (layer.blendMode == AnimationBlendMode::Additive) {
                    finalLocals[i] = ApplyAdditiveTransform(finalLocals[i], bindPose[i], layer.blendedPose[i], weight);
                }
                else {
                    finalLocals[i] = BlendTransform(finalLocals[i], layer.blendedPose[i], weight);
                }
            }
        }

        state.localNodePose = finalLocals;
        state.globalNodeTransforms.resize(nodeCount);

        for (int i = 0; i < nodeCount; i++) {
            glm::mat4 localMatrix = state.localNodePose[i].to_mat4();
            const std::string& nodeName = state.skinnedModel->m_nodes[i].name;
            auto additive = state.additiveNodeTransforms.find(nodeName);

            if (additive != state.additiveNodeTransforms.end()) {
                localMatrix = additive->second * localMatrix;
            }

            const int parent = state.skinnedModel->m_nodes[i].parentIndex;
            state.globalNodeTransforms[i] = parent >= 0 ? state.globalNodeTransforms[parent] * localMatrix : localMatrix;
        }

        for (glm::mat4& matrix : state.globalNodeTransforms) {
            Hell::Math::Sanitize(matrix);
        }
    }

    void UpdateBoneSkinningMatrices(AnimationState& state) {
        if (!state.skinnedModel) {
            return;
        }

        const int boneCount = state.skinnedModel->GetBoneCount();
        state.boneSkinningMatrices.assign(boneCount, glm::mat4(1.0f));

        for (int boneIndex = 0; boneIndex < boneCount; boneIndex++) {
            const int nodeIndex = state.skinnedModel->m_boneNodeIndices[boneIndex];
            if (nodeIndex >= 0 && nodeIndex < state.globalNodeTransforms.size()) {
                state.boneSkinningMatrices[boneIndex] = state.globalNodeTransforms[nodeIndex] * state.skinnedModel->m_boneOffsets[boneIndex];
            }
        }
    }

    void PauseAllLayers(AnimationState& state) {
        for (AnimationLayerState& layer : state.layers) {
            for (AnimationPlayback& playback : layer.playbacks) {
                playback.paused = true;
            }
        }
    }

    void SetAdditiveTransform(AnimationState& state, const std::string& nodeName, const glm::mat4& matrix) {
        state.additiveNodeTransforms[nodeName] = matrix;
    }

    uint32_t GetAnimationFrameNumber(const AnimationState& state, const std::string& animationLayerName) {
        const AnimationLayerState* layer = FindLayer(state, animationLayerName);
        if (!layer) {
            return 0;
        }

        const AnimationPlayback* playback = GetPrimaryPlayback(*layer);
        if (!playback || !playback->animation) {
            return 0;
        }

        return static_cast<uint32_t>(playback->time * playback->animation->m_ticksPerSecond);
    }

    bool AnimationIsPastFrameNumber(const AnimationState& state, const std::string& animationLayerName, int frameNumber) {
        return GetAnimationFrameNumber(state, animationLayerName) > static_cast<uint32_t>(frameNumber);
    }

    bool AnimationIsCompleteAnyLayer(const AnimationState& state, const std::string& animationName) {
        for (const AnimationLayerState& layer : state.layers) {
            for (const AnimationPlayback& playback : layer.playbacks) {
                if (playback.animation && playback.animation->GetName() == animationName && playback.complete) {
                    return true;
                }
            }
        }

        return false;
    }

    bool AllAnimationsComplete(const AnimationState& state) {
        for (const AnimationLayerState& layer : state.layers) {
            for (const AnimationPlayback& playback : layer.playbacks) {
                if (!playback.complete) {
                    return false;
                }
            }
        }

        return true;
    }
}
