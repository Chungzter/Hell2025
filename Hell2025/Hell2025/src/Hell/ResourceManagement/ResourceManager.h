#pragma once

#include "Hell/ResourceManagement/Types/Animation.h"
#include "Hell/ResourceManagement/Types/GenericMesh.h"
#include "Hell/ResourceManagement/Types/IESProfile.h"
#include "Hell/ResourceManagement/Types/Material.h"
#include "Hell/ResourceManagement/Types/MeshBuffer.h"
#include "Hell/ResourceManagement/Types/MidiFile.h"
#include "Hell/ResourceManagement/Types/SoundFont.h"
#include "Hell/ResourceManagement/Types/Texture.h"
#include "Types/Renderer/Model.h"
#include "Types/Renderer/SkinnedModel.h"
#include "Types/Renderer/SpriteSheetTexture.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace Hell::MemoryTracker {
    struct MemoryReport;
}

namespace Hell::ResourceManager {

    void Init();
    void CleanUp();
    void AppendMemoryReport(MemoryTracker::MemoryReport& report);

    Animation& CreateAnimation(const std::string& name);
    std::unordered_map<std::string, Animation>& GetAnimations();
    Animation& GetAnimation(const std::string& name);
    Animation* GetAnimationPtr(const std::string& name);

    GenericMesh& CreateGenericMesh(const std::string& name);
    GenericMesh& GetGenericMesh(const std::string& name);
    GenericMesh* GetGenericMeshPtr(const std::string& name);

    IESProfile& CreateIESProfile(const std::string& name);
    IESProfile& CreateIESProfile(IESProfile&& iesProfile);
    IESProfile& GetIESProfile(const std::string& name);
    IESProfile* GetIESProfilePtr(const std::string& name);

    Material& CreateMaterial(const std::string& name);
    std::vector<Material>& GetMaterials();
    std::vector<std::string> GetMaterialNames();
    Material* GetDefaultMaterial();
    Material* GetMaterialByIndex(int32_t index);
    Material* GetMaterialByName(const std::string& name);
    int32_t GetMaterialIndexByName(const std::string& name);
    std::string GetMaterialNameByIndex(int32_t index);

    MeshBuffer& CreateMeshBuffer(const std::string& name);
    MeshBuffer& GetMeshBuffer(const std::string& name);
    MeshBuffer* GetMeshBufferPtr(const std::string& name);

    MidiFile& CreateMidiFile(const std::string& name);
    MidiFile& CreateMidiFile(MidiFile&& midiFile);
    MidiFile& GetMidiFile(const std::string& name);
    MidiFile* GetMidiFilePtr(const std::string& name);

    Model& CreateModel(const std::string& name);
    std::unordered_map<uint32_t, Model>& GetModels();
    Model* GetModelById(uint32_t modelId);
    Model* GetModelByName(const std::string& name);
    uint32_t GetModelIdByName(const std::string& name);
    void SetModelName(uint32_t modelId, const std::string& name);

    SkinnedModel& CreateSkinnedModel(const std::string& name);
    std::unordered_map<uint32_t, SkinnedModel>& GetSkinnedModels();
    SkinnedModel* GetSkinnedModelById(uint32_t skinnedModelId);
    SkinnedModel* GetSkinnedModelByName(const std::string& name);
    uint32_t GetSkinnedModelIdByName(const std::string& name);

    SoundFont& CreateSoundFont(const std::string& name);
    SoundFont& CreateSoundFont(SoundFont&& soundFont);
    SoundFont& GetSoundFont(const std::string& name);
    SoundFont* GetSoundFontPtr(const std::string& name);

    SpriteSheetTexture& CreateSpriteSheetTexture(const std::string& name);
    SpriteSheetTexture& GetSpriteSheetTexture(const std::string& name);
    SpriteSheetTexture* GetSpriteSheetTexturePtr(const std::string& name);

    Texture& CreateTexture(const std::string& name);
    std::unordered_map<std::string, Texture>& GetTextures();
    Texture* GetTextureByName(const std::string& name);
    Texture* GetTextureByBindlessIndex(int32_t bindlessIndex);
    int32_t GetTextureBindlessIndexByName(const std::string& name, bool ignoreWarning = true);
    void ReserveTextureStorage(size_t textureCount);
}
