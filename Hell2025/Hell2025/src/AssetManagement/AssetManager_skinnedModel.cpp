#include "AssetManager.h"
#include "Hell/AssetFormats/AssetFormats.h"

#include <future>

#include <iostream> // TODO: cleanup logging

namespace AssetManager {
    static std::vector<std::future<void>> g_skinnedModelFutures;

    void LoadPendingSkinnedModelsAsync() {
        for (SkinnedModel& skinnedModel : GetSkinnedModels()) {
            if (skinnedModel.GetLoadingState() == LoadingState::Value::AWAITING_LOADING_FROM_DISK) {
                skinnedModel.SetLoadingState(LoadingState::Value::LOADING_FROM_DISK);
                AddItemToLoadLog(skinnedModel.GetFileInfo().path);
                g_skinnedModelFutures.emplace_back(std::async(std::launch::async, LoadSkinnedModel, &skinnedModel));
                break;
            }
        }

        // Pump any completed futures
        for (size_t i = 0; i < g_skinnedModelFutures.size();) {
            if (g_skinnedModelFutures[i].wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                try {
                    g_skinnedModelFutures[i].get();
                }
                catch (...) {
                    std::cout << "Some async skinned model loading error occured\n";
                }
                g_skinnedModelFutures.erase(g_skinnedModelFutures.begin() + i);
            }
            else {
                ++i;
            }
        }
    }
    
    void AssetManager::LoadSkinnedModel(SkinnedModel* skinnedModel) {
        const FileInfo& fileInfo = skinnedModel->GetFileInfo();
        std::string assetPath = "res/skinned_models/" + fileInfo.name + ".skinnedmodel";
        Hell::AssetFormats::LoadSkinnedModel(assetPath, skinnedModel->m_skinnedModelData);
        skinnedModel->SetLoadingState(LoadingState::Value::LOADING_COMPLETE);
    }

    void BakeSkinnedModels() {
        // Preallocate the vertex/index count
        size_t vertexCount = 0;
        size_t indexCount = 0;

        for (const SkinnedModel& skinnedModel : GetSkinnedModels()) {
            for (const SkinnedMeshData& mesh : skinnedModel.m_skinnedModelData.meshes) {
                vertexCount += mesh.vertexCount;
                indexCount += mesh.indexCount;
            }
        }

        //GetWeightedVertices().reserve(GetWeightedVertices().size() + vertexCount);
        //GetWeightedIndies().reserve(GetWeightedIndies().size() + indexCount);
        GetVertices().reserve(GetVertices().size() + vertexCount);
        GetIndies().reserve(GetIndies().size() + indexCount);

        // Copy vertices/indices to asset manager
        for (SkinnedModel& skinnedModel : GetSkinnedModels()) {
            skinnedModel.BakeToAssetManager();
        }
    }

    SkinnedModel* AssetManager::GetSkinnedModelByName(const std::string& name) {
        std::vector<SkinnedModel>& skinnedModels = GetSkinnedModels();
        for (auto& skinnedModel : skinnedModels) {
            if (name == skinnedModel.GetName()) {
                return &skinnedModel;
            }
        }
        std::cout << "AssetManager::GetSkinnedModelByName(const std::string& name) failed because '" << name << "' does not exist!\n";
        return nullptr;
    }

    SkinnedModel* AssetManager::GetSkinnedModelByIndex(int index) {
        std::vector<SkinnedModel>& skinnedModels = GetSkinnedModels();
        if (index >= 0 && index < skinnedModels.size()) {
            return &skinnedModels[index];
        }
        else {
            std::cout << "AssetManager::GetSkinnedModelByIndex(int index) failed because index '" << index << "' is out of range. Size is " << skinnedModels.size() << "!\n";
            return nullptr;
        }
    }

    int AssetManager::GetSkinnedModelIndexByName(const std::string& name) {
        std::vector<SkinnedModel>& skinnedModels = GetSkinnedModels();
        for (int i = 0; i < skinnedModels.size(); i++) {
            if (name == skinnedModels[i].GetName()) {
                return i;
            }
        }
        std::cout << "AssetManager::GetSkinnedModelIndexByName(const std::string& name) failed because '" << name << "' does not exist!\n";
        return -1;
    }
}
