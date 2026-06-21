#include "AssetManager.h"
#include "Bvh/Cpu/CpuBvh.h"
#include "Hell/AssetFormats/AssetFormats.h"
#include "Hell/Logging.h"
#include <future>

#include <iostream> // TODO: clean up logging

namespace AssetManager {
    static std::vector<std::future<void>> g_modelFutures;

    void PrintModelMeshNames(Model* model) {
        if (!model) {
            Logging::Error() << "AssetManager::PrintModelMeshNames(..) failed coz model param was nullptr\n";
            return;
        }

        std::cout << model->GetName() << "\n";
        for (const uint32_t& meshIndex : model->GetMeshIndices()) {
            Mesh* mesh = AssetManager::GetMeshByIndex(meshIndex);
            if (mesh) {
                std::cout << " - " << mesh->GetName() << "\n";
            }
            else {
                std::cout << " - INVALID MESH SOMEHOW\n";
            }
        }
    }

    void LoadPendingModelsAsync() {
        for (Model& model : GetModels()) {
            if (model.GetLoadingState() == LoadingState::Value::AWAITING_LOADING_FROM_DISK) {
                model.SetLoadingState(LoadingState::Value::LOADING_FROM_DISK);
                AddItemToLoadLog(model.GetFileInfo().path);
                g_modelFutures.emplace_back(std::async(std::launch::async, LoadModel, &model));
                break;
            }
        }

        // Pump any completed futures
        for (size_t i = 0; i < g_modelFutures.size();) {
            if (g_modelFutures[i].wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                try { 
                    g_modelFutures[i].get();
                }
                catch (...) {
                    std::cout << "Some async model loading error occured\n";
                }
                g_modelFutures.erase(g_modelFutures.begin() + i);
            }
            else {
                ++i;
            }
        }
    }

    void BakeModels() {
        // Prellocate the vertex/index count
        size_t vertexCount = 0;
        size_t indexCount = 0;

        for (const Model& model : GetModels()) {
            for (const MeshData& mesh : model.m_modelData.meshes) {
                vertexCount += mesh.vertexCount;
                indexCount += mesh.indexCount;
            }
        }
        GetVertices().reserve(GetVertices().size() + vertexCount);
        GetIndies().reserve(GetIndies().size() + indexCount);

        // Copy the vertices/indices into the asset manager
        for (Model& model : GetModels()) {
            model.SetName(model.m_modelData.name);
            model.SetAABB(model.m_modelData.aabbMin, model.m_modelData.aabbMax);
            for (MeshData& meshData : model.m_modelData.meshes) {
                int meshIndex = CreateMesh(meshData.name, meshData.vertices, meshData.indices, meshData.aabbMin, meshData.aabbMax, meshData.parentIndex, meshData.localTransform, meshData.inverseBindTransform);
                model.AddMeshIndex(meshIndex);
            }
        }
    }

    void LoadModel(Model* model) {
        const FileInfo& fileInfo = model->GetFileInfo();
        std::string modelPath = "res/models/" + fileInfo.name + "." + fileInfo.ext;
        std::string bvhPath = "res/models/bvh/" + fileInfo.name + ".bvh";
        Hell::AssetFormats::LoadModel(modelPath, model->m_modelData);
        Hell::AssetFormats::LoadModelBvh(bvhPath, model->m_modelBvhData);
        model->SetLoadingState(LoadingState::Value::LOADING_COMPLETE);
    }

    void CopyInAllLoadedModelBvhData() {
        std::vector<Model>& models = GetModels();

        for (Model& model : models) {
            // Skip primitives
            if (model.GetName() == "Primitives") continue;

            // Quick error check that bvh count matches mesh count
            if (model.m_modelBvhData.bvhs.size() != model.GetMeshCount()) {
                std::cout << "CopyInAllLoadedModelBvhData() error: bvh count does not equal mesh count for " << model.GetName() << "\n";
                continue;
            }

            // Iterate each preloaded MeshBvh and extract the data
            for (int i = 0; i < model.m_modelBvhData.bvhs.size(); i++) {
                MeshBvh& sourceMeshBvh = model.m_modelBvhData.bvhs[i];
                uint32_t meshIndex = model.GetMeshIndices()[i];

                Mesh* mesh = AssetManager::GetMeshByIndex(meshIndex);
                if (!mesh) {
                    std::cout << "CopyInAllLoadedModelBvhData() error: mesh with index " << meshIndex << " was invalid for " << model.GetName() << "\n";
                    continue;
                }

                // Swap data out of source MeshBvh and into the unordered map within BVH namespace, returning a new id
                mesh->meshBvhId = Bvh::Cpu::CreateMeshBvhFromMeshBvh(sourceMeshBvh);
            }

            // Clean up
            model.m_modelBvhData.bvhs.clear();
        }

        Bvh::Cpu::FlatternMeshBvhNodes();
    }

    Model* CreateModel(const std::string& name) {
        std::vector<Model>& models = GetModels();
        models.emplace_back();
        Model* model = &models[models.size() - 1];
        model->SetName(name);
        return model;
    }

    Model* GetModelByName(const std::string& name) {
        int index = GetModelIndexByName(name);
        return GetModelByIndex(index);
    }

    Model* GetModelByIndex(int index) {
        if (index >= 0 && index < GetModels().size()) {
            return &GetModels()[index];
        }
        return nullptr;
    }

    int GetModelIndexByName(const std::string& name) {
        std::unordered_map<std::string, int>& modelIndexMap = GetModelIndexMap();
        auto it = modelIndexMap.find(name);
        if (it != modelIndexMap.end()) {
            return it->second;
        }
        Logging::Error() << "AssetManager::GetModelIndexByName() failed because name '" << name << "' was not found in g_models!";
        return -1;
    }

    void BuildPrimitives() {
        // TODO: Rewrite this to store these values in model.m_modelData so that you can bake after creating these!

        Model* model = CreateModel("Primitives");

        /* Quad */ {
            std::vector<Vertex> vertices = {
                // Position               Normal               UV            Tangent
                {{-0.5f, -0.5f, 0.0f},    {0.0f, 0.0f, 1.0f},  {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}}, // Bottom-left
                {{ 0.5f, -0.5f, 0.0f},    {0.0f, 0.0f, 1.0f},  {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}}, // Bottom-right
                {{ 0.5f,  0.5f, 0.0f},    {0.0f, 0.0f, 1.0f},  {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}}, // Top-right
                {{-0.5f,  0.5f, 0.0f},    {0.0f, 0.0f, 1.0f},  {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}}  // Top-left
            };

            std::vector<uint32_t> indices = { 0, 1, 2, 2, 3, 0 };

            int meshIndex = CreateMesh("Quad", vertices, indices);
            model->AddMeshIndex(meshIndex);
        }

        /* Cube */ {
            std::vector<Vertex> cubeVertices = {
                // FRONT FACE
                {{-0.5f, -0.5f,  0.5f}, { 0, 0, 1}, {0, 0}, {1, 0, 0}},
                {{ 0.5f, -0.5f,  0.5f}, { 0, 0, 1}, {1, 0}, {1, 0, 0}},
                {{ 0.5f,  0.5f,  0.5f}, { 0, 0, 1}, {1, 1}, {1, 0, 0}},
                {{-0.5f,  0.5f,  0.5f}, { 0, 0, 1}, {0, 1}, {1, 0, 0}},

                // BACK FACE
                {{ 0.5f, -0.5f, -0.5f}, { 0, 0, -1}, {0, 0}, {-1, 0, 0}},
                {{-0.5f, -0.5f, -0.5f}, { 0, 0, -1}, {1, 0}, {-1, 0, 0}},
                {{-0.5f,  0.5f, -0.5f}, { 0, 0, -1}, {1, 1}, {-1, 0, 0}},
                {{ 0.5f,  0.5f, -0.5f}, { 0, 0, -1}, {0, 1}, {-1, 0, 0}},

                // LEFT FACE
                {{-0.5f, -0.5f, -0.5f}, {-1, 0, 0}, {0, 0}, {0, 0, 1}},
                {{-0.5f, -0.5f,  0.5f}, {-1, 0, 0}, {1, 0}, {0, 0, 1}},
                {{-0.5f,  0.5f,  0.5f}, {-1, 0, 0}, {1, 1}, {0, 0, 1}},
                {{-0.5f,  0.5f, -0.5f}, {-1, 0, 0}, {0, 1}, {0, 0, 1}},

                // RIGHT FACE
                {{ 0.5f, -0.5f,  0.5f}, { 1, 0, 0}, {0, 0}, {0, 0, -1}},
                {{ 0.5f, -0.5f, -0.5f}, { 1, 0, 0}, {1, 0}, {0, 0, -1}},
                {{ 0.5f,  0.5f, -0.5f}, { 1, 0, 0}, {1, 1}, {0, 0, -1}},
                {{ 0.5f,  0.5f,  0.5f}, { 1, 0, 0}, {0, 1}, {0, 0, -1}},

                // TOP FACE
                {{-0.5f,  0.5f,  0.5f}, { 0, 1, 0}, {0, 0}, {1, 0, 0}},
                {{ 0.5f,  0.5f,  0.5f}, { 0, 1, 0}, {1, 0}, {1, 0, 0}},
                {{ 0.5f,  0.5f, -0.5f}, { 0, 1, 0}, {1, 1}, {1, 0, 0}},
                {{-0.5f,  0.5f, -0.5f}, { 0, 1, 0}, {0, 1}, {1, 0, 0}},

                // BOTTOM FACE
                {{-0.5f, -0.5f, -0.5f}, { 0,-1, 0}, {0, 0}, {1, 0, 0}},
                {{ 0.5f, -0.5f, -0.5f}, { 0,-1, 0}, {1, 0}, {1, 0, 0}},
                {{ 0.5f, -0.5f,  0.5f}, { 0,-1, 0}, {1, 1}, {1, 0, 0}},
                {{-0.5f, -0.5f,  0.5f}, { 0,-1, 0}, {0, 1}, {1, 0, 0}} 
            };

            std::vector<uint32_t> cubeIndices = {
                0, 1, 2,  2, 3, 0,      // Front face
                4, 5, 6,  6, 7, 4,      // Back face
                8, 9, 10, 10, 11, 8,    // Left face
                12, 13, 14, 14, 15, 12, // Right face
                16, 17, 18, 18, 19, 16, // Top face
                20, 21, 22, 22, 23, 20  // Bottom face
            };

            int meshIndexCube = CreateMesh("Cube", cubeVertices, cubeIndices);
            model->AddMeshIndex(meshIndexCube);
        }
        model->SetLoadingState(LoadingState::Value::LOADING_COMPLETE);
    }
}
