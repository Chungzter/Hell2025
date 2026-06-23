#include "AssetLoader.h"

#include "Hell/Logging.h"
#include "Hell/ResourceManagement/ResourceManager.h"

#include <algorithm>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <cstdint>
#include <string>
#include <utility>

namespace Hell::AssetLoader {

    namespace {
        glm::vec3 GetPosition(const aiNodeAnim& channel, uint32_t keyIndex) {
            if (channel.mNumPositionKeys == 0) {
                return glm::vec3(0.0f);
            }

            const aiVectorKey& key = channel.mPositionKeys[std::min(keyIndex, channel.mNumPositionKeys - 1)];
            return glm::vec3(key.mValue.x, key.mValue.y, key.mValue.z);
        }

        glm::quat GetRotation(const aiNodeAnim& channel, uint32_t keyIndex) {
            if (channel.mNumRotationKeys == 0) {
                return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
            }

            const aiQuatKey& key = channel.mRotationKeys[std::min(keyIndex, channel.mNumRotationKeys - 1)];
            return glm::quat(key.mValue.w, key.mValue.x, key.mValue.y, key.mValue.z);
        }

        glm::vec3 GetScale(const aiNodeAnim& channel, uint32_t keyIndex) {
            if (channel.mNumScalingKeys == 0) {
                return glm::vec3(1.0f);
            }

            const aiVectorKey& key = channel.mScalingKeys[std::min(keyIndex, channel.mNumScalingKeys - 1)];
            return glm::vec3(key.mValue.x, key.mValue.y, key.mValue.z);
        }

        float GetTimeStamp(const aiNodeAnim& channel, uint32_t keyIndex) {
            if (keyIndex < channel.mNumPositionKeys) {
                return static_cast<float>(channel.mPositionKeys[keyIndex].mTime);
            }
            if (keyIndex < channel.mNumRotationKeys) {
                return static_cast<float>(channel.mRotationKeys[keyIndex].mTime);
            }
            if (keyIndex < channel.mNumScalingKeys) {
                return static_cast<float>(channel.mScalingKeys[keyIndex].mTime);
            }
            return 0.0f;
        }

        bool LoadAnimation(const std::string& path, Animation& outAnimation) {
            Assimp::Importer importer;
            const aiScene* scene = importer.ReadFile(
                path,
                aiProcess_Triangulate |
                aiProcess_GenSmoothNormals |
                aiProcess_FlipUVs
            );

            if (!scene) {
                Logging::Error() << "AssetLoader::LoadAnimation(..) failed to load '" << path << "': " << importer.GetErrorString() << "\n";
                return false;
            }

            if (scene->mNumAnimations == 0 || !scene->mAnimations[0]) {
                Logging::Warning() << "AssetLoader::LoadAnimation(..) found no animations in '" << path << "'\n";
                return false;
            }

            const FileInfo fileInfo = outAnimation.GetFileInfo();
            const aiAnimation& sourceAnimation = *scene->mAnimations[0];

            Animation loadedAnimation;
            loadedAnimation.SetFileInfo(fileInfo);
            loadedAnimation.m_duration = static_cast<float>(sourceAnimation.mDuration);
            loadedAnimation.m_ticksPerSecond = static_cast<float>(sourceAnimation.mTicksPerSecond);
            loadedAnimation.m_animatedNodes.reserve(sourceAnimation.mNumChannels);

            for (uint32_t nodeIndex = 0; nodeIndex < sourceAnimation.mNumChannels; ++nodeIndex) {
                const aiNodeAnim* channel = sourceAnimation.mChannels[nodeIndex];
                if (!channel) {
                    continue;
                }

                const std::string nodeName = channel->mNodeName.C_Str();
                AnimatedNode animatedNode(nodeName);

                const uint32_t keyCount = std::max({
                    channel->mNumPositionKeys,
                    channel->mNumRotationKeys,
                    channel->mNumScalingKeys
                });

                if (keyCount == 0) {
                    continue;
                }

                const unsigned int animatedNodeIndex = static_cast<unsigned int>(loadedAnimation.m_animatedNodes.size());
                loadedAnimation.m_NodeMapping.emplace(nodeName, animatedNodeIndex);
                animatedNode.m_nodeKeys.reserve(keyCount);

                for (uint32_t keyIndex = 0; keyIndex < keyCount; ++keyIndex) {
                    SQT& sqt = animatedNode.m_nodeKeys.emplace_back();
                    sqt.positon = GetPosition(*channel, keyIndex);
                    sqt.rotation = GetRotation(*channel, keyIndex);
                    sqt.scale = GetScale(*channel, keyIndex);
                    sqt.timeStamp = GetTimeStamp(*channel, keyIndex);

                    loadedAnimation.m_finalTimeStamp = std::max(loadedAnimation.m_finalTimeStamp, sqt.timeStamp);
                }

                loadedAnimation.m_animatedNodes.push_back(std::move(animatedNode));
            }

            loadedAnimation.SetLoadingState(LoadingState::Value::LOADING_COMPLETE);
            outAnimation = std::move(loadedAnimation);
            return true;
        }
    }

    void LoadAnimations() {
        for (auto& animationEntry : ResourceManager::GetAnimations()) {
            Animation& animation = animationEntry.second;

            if (animation.GetLoadingState() != LoadingState::Value::AWAITING_LOADING_FROM_DISK) {
                continue;
            }

            animation.SetLoadingState(LoadingState::Value::LOADING_FROM_DISK);

            if (!LoadAnimation(animation.GetFileInfo().path, animation)) {
                animation.SetLoadingState(LoadingState::Value::AWAITING_LOADING_FROM_DISK);
            }
        }
    }
}
