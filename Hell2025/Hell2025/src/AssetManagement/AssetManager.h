#pragma once

#include <Game/Types.h>

#include "Types/Renderer/Model.h"
#include "Types/Renderer/SkinnedMesh.hpp"
#include "Types/Renderer/SkinnedModel.h"

#include "Hell/Render/VertexAttributes.h"
#include "Hell/ResourceManagement/Types/IESProfile.h"
#include "Hell/ResourceManagement/Types/Mesh.h"
#include "Hell/ResourceManagement/Types/Texture.h"

#include <cstdint>
#include <span>
#include <string>
#include <vector>
#include <unordered_map>

namespace AssetManager {
    void Init(); 
    void UpdateLoading();
    bool LoadingComplete();
    void AddItemToLoadLog(std::string text);
    std::vector<std::string>& GetLoadLog();

    // Mesh
    std::vector<Mesh>& GetMeshes();
    int CreateMesh(const std::string& name, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, glm::vec3 aabbMin, glm::vec3 aabbMax, int parentIndex, glm::mat4 localTransform, glm::mat4 inverseBindTransform);
    int CreateMesh(const std::string& name, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);
    int GetMeshIndexByName(const std::string& name);
    int GetMeshIndexByName(const std::string& name);
    int GetQuadZFacingMeshIndex();
    Mesh* GetMeshByName(const std::string& name);
    Mesh* GetMeshByIndex(int index);
    Mesh* GetCubeMesh();
    Mesh* GetQuadZFacingMesh();
    Mesh* GetMeshByModelNameMeshName(const std::string& modelName, const std::string& meshName);
    Mesh* GetMeshByModelNameMeshIndex(const std::string& modelName, uint32_t meshIndex);
    int GetMeshIndexByModelNameMeshName(const std::string& modelName, const std::string& meshName);
    std::vector<Vertex> GetMeshVertices(Mesh* mesh);
    std::span<Vertex> GetMeshVerticesSpan(Mesh* mesh);
    std::span<uint32_t> GetMeshIndicesSpan(Mesh* mesh);
    void CreateMeshBvhs();
    const std::string& GetMeshNameByMeshIndex(int index);
    uint32_t GetBaseVertexByMeshIndex(int meshIndex);
    uint32_t GetBaseIndexByMeshIndex(int meshIndex);

    // IES Profiles
    IESProfile* GetIESProfileByName(const std::string& name);
    IESProfile* GetIESProfileByIESProfileType(IESProfileType type);

    // Models
    std::vector<Model>& GetModels();
    Model* CreateModel(const std::string& name);
    Model* GetModelByName(const std::string& name);
    Model* GetModelByIndex(int index);
    int GetModelIndexByName(const std::string& name);
    void PrintModelMeshNames(Model* model);

    // Skinned Mesh
    std::vector<SkinnedMesh>& GetSkinnedMeshes();
    SkinnedMesh* GetSkinnedMeshByIndex(int index);
    int CreateSkinnedMesh2(const SkinnedMeshData& skinnedMeshData);
    int GetSkinnedMeshIndexByName(const std::string& name);

    // Textures
    std::unordered_map<std::string, Texture>& GetTextures();

    // Skinned Model
    std::vector<SkinnedModel>& GetSkinnedModels();
    SkinnedModel* GetSkinnedModelByName(const std::string& name);
    SkinnedModel* GetSkinnedModelByIndex(int index);
    int GetSkinnedModelIndexByName(const std::string& name);

    // Vertex Data
    std::vector<Vertex>& GetVertices();
    std::vector<VertexWeight>& GetVertexWeights();
    std::span<Vertex> GetVerticesSpan(uint32_t baseVertex, uint32_t vertexCount);
    std::vector<uint32_t>& GetIndices();
    std::span<uint32_t> GetIndicesSpan(uint32_t baseIndex, uint32_t indexCount);

    // Building
    void BuildPrimitives();
    void BuildIndexMaps();

    // Loading 
    void LoadPendingModelsAsync();
    void LoadPendingSkinnedModelsAsync();
    void LoadModel(Model* model);
    void LoadSkinnedModel(SkinnedModel* skinnedModel);

    // Baking
    void BakeModels();
    void BakeSkinnedModels();

    // BVH
    void CopyInAllLoadedModelBvhData();

    // Vertex data
    std::vector<Vertex>& GetVertices();
    std::vector<uint32_t>& GetIndies();

    // Index maps
    std::unordered_map<std::string, int>& GetTextureIndexMap();
    std::unordered_map<std::string, int>& GetModelIndexMap();
}
