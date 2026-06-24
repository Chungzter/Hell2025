#include "AssetLoader.h"

#include "Hell/Logging.h"

#include <algorithm>
#include <assimp/config.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <cstdint>
#include <string>
#include <utility>

namespace Hell::AssetLoader {

    namespace {
        constexpr unsigned int ANIMATION_IMPORT_FLAGS = aiProcess_RemoveComponent;
        constexpr unsigned int ANIMATION_REMOVE_COMPONENTS =
            aiComponent_MESHES |
            aiComponent_MATERIALS |
            aiComponent_TEXTURES |
            aiComponent_LIGHTS |
            aiComponent_CAMERAS |
            aiComponent_BONEWEIGHTS;

        void ConfigureAnimationImporter(Assimp::Importer& importer) {
            importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_READ_ALL_GEOMETRY_LAYERS, false);
            importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_READ_MATERIALS, false);
            importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_READ_TEXTURES, false);
            importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_READ_CAMERAS, false);
            importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_READ_LIGHTS, false);
            importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_READ_ANIMATIONS, true);
            importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_READ_WEIGHTS, false);
            importer.SetPropertyBool(AI_CONFIG_IMPORT_NO_SKELETON_MESHES, true);
            importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, ANIMATION_REMOVE_COMPONENTS);
        }

        bool LoadAnimation(const FileInfo& fileInfo, Animation& outAnimation) {
            Assimp::Importer importer;
            ConfigureAnimationImporter(importer);

            const aiScene* scene = importer.ReadFile(fileInfo.path, ANIMATION_IMPORT_FLAGS);

            if (!scene) {
                Logging::Error() << "AssetLoader::LoadAnimation(..) failed to load '" << fileInfo.path << "': " << importer.GetErrorString() << "\n";
                return false;
            }

            if (scene->mNumAnimations == 0 || !scene->mAnimations[0]) {
                Logging::Warning() << "AssetLoader::LoadAnimation(..) found no animations in '" << fileInfo.path << "'\n";
                return false;
            }

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

                const uint32_t positionKeyCount = channel->mNumPositionKeys;
                const uint32_t rotationKeyCount = channel->mNumRotationKeys;
                const uint32_t scalingKeyCount = channel->mNumScalingKeys;

                const aiVectorKey* positionKeys = channel->mPositionKeys;
                const aiQuatKey* rotationKeys = channel->mRotationKeys;
                const aiVectorKey* scalingKeys = channel->mScalingKeys;

                const uint32_t keyCount = std::max(positionKeyCount, std::max(rotationKeyCount, scalingKeyCount));

                if (keyCount == 0) {
                    continue;
                }

                animatedNode.m_nodeKeys.resize(keyCount);

                for (uint32_t keyIndex = 0; keyIndex < keyCount; ++keyIndex) {
                    SQT& sqt = animatedNode.m_nodeKeys[keyIndex];

                    if (positionKeyCount > 0) {
                        const aiVectorKey& key = positionKeys[keyIndex < positionKeyCount ? keyIndex : positionKeyCount - 1];
                        sqt.positon = glm::vec3(key.mValue.x, key.mValue.y, key.mValue.z);
                    }

                    if (rotationKeyCount > 0) {
                        const aiQuatKey& key = rotationKeys[keyIndex < rotationKeyCount ? keyIndex : rotationKeyCount - 1];
                        sqt.rotation = glm::quat(key.mValue.w, key.mValue.x, key.mValue.y, key.mValue.z);
                    }

                    if (scalingKeyCount > 0) {
                        const aiVectorKey& key = scalingKeys[keyIndex < scalingKeyCount ? keyIndex : scalingKeyCount - 1];
                        sqt.scale = glm::vec3(key.mValue.x, key.mValue.y, key.mValue.z);
                    }

                    if (keyIndex < positionKeyCount) {
                        sqt.timeStamp = static_cast<float>(positionKeys[keyIndex].mTime);
                    }
                    else if (keyIndex < rotationKeyCount) {
                        sqt.timeStamp = static_cast<float>(rotationKeys[keyIndex].mTime);
                    }
                    else if (keyIndex < scalingKeyCount) {
                        sqt.timeStamp = static_cast<float>(scalingKeys[keyIndex].mTime);
                    }

                    loadedAnimation.m_finalTimeStamp = std::max(loadedAnimation.m_finalTimeStamp, sqt.timeStamp);
                }

                loadedAnimation.m_animatedNodes.push_back(std::move(animatedNode));
            }

            loadedAnimation.SetLoadState(LoadState::LOADED);
            outAnimation = std::move(loadedAnimation);
            return true;
        }
    }

    AnimationData LoadAnimationData(FileInfo fileInfo) {
        AnimationData result;

        if (!LoadAnimation(fileInfo, result.animation)) {
            result.animation.SetFileInfo(fileInfo);
            result.animation.SetLoadState(LoadState::FAILED);
        }

        return result;
    }
}
