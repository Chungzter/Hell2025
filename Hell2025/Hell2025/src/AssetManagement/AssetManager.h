#pragma once

#include <Game/Types.h>
#include "Hell/Render/VertexAttributes.h"

#include "File/File.h"

#include "Types/Animation/Animation.h"
#include "Hell/ResourceManagement/Types/IESProfile.h"
#include "Hell/ResourceManagement/Types/Mesh.h"
#include "Types/Renderer/Model.h"
#include "Types/Renderer/SkinnedMesh.hpp"
#include "Types/Renderer/SkinnedModel.h"
#include "Types/Renderer/SpriteSheetTexture.h"
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

    // Animations
    std::vector<Animation>& GetAnimations();
    Animation* GetAnimationByName(const std::string& name);
    Animation* GetAnimationByIndex(int index, bool printError = true);
    int GetAnimationIndexByName(const std::string& name);

    // Materials
    std::vector<Material>& GetMaterials();
    std::vector<std::string> GetMaterialNames();
    Material* GetDefaultMaterial();
    Material* GetMaterialByIndex(int index);
    Material* GetMaterialByName(const std::string& name);
    int GetMaterialIndexByName(const std::string& name);
    std::string GetMaterialNameByIndex(int index);

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
    Texture& CreateNewTexture(const std::string& name);
    Texture* GetTextureByName(const std::string& name);
    Texture* GetTextureByBindlessIndex(int32_t bindlessIndex);
    int32_t GetTextureBindlessIndexByName(const std::string& name, bool ignoreWarning = true);
    void ReserveTextureStorage(size_t textureCount);

    // Spritesheet Textures
    std::vector<SpriteSheetTexture>& GetSpriteSheetTextures();
    SpriteSheetTexture* GetSpriteSheetTextureByName(const std::string& textureName);
    void BuildSpriteSheetTextures();

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
    void BuildMaterials();

    // Loading 
    void LoadPendingTexturesAsync();
    void LoadPendingModelsAsync();
    void LoadPendingSkinnedModelsAsync();
    void LoadPendingAnimationsAsync();
    void LoadAnimation(Animation* animation);
    void LoadModel(Model* model);
    void LoadSkinnedModel(SkinnedModel* skinnedModel);
    void LoadTexture(Texture* texture);

    // Baking
    void BakeModels();
    void BakeSkinnedModels();

    // Import/Export
    void ExportMissingModels();
    void ExportMissingSkinnedModels();
    void ExportMissingModelBvhs();

    // BVH
    void CopyInAllLoadedModelBvhData();

    // Vertex data
    std::vector<Vertex>& GetVertices();
    std::vector<uint32_t>& GetIndies();

    //std::vector<Vertex>& GetWeightedVertices();
    //std::vector<uint32_t>& GetWeightedIndies();

    // Index maps
    std::unordered_map<std::string, int>& GetTextureIndexMap();
    std::unordered_map<std::string, int>& GetMaterialIndexMap();
    std::unordered_map<std::string, int>& GetModelIndexMap();
}
