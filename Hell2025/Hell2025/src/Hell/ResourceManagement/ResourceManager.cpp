#include "ResourceManager.h"

#include "Hell/Logging.h"
#include "Hell/MemoryTracker/MemoryTracker.h"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

namespace Hell::ResourceManager {

    namespace {
        std::unordered_map<std::string, Animation> g_animations;
        std::unordered_map<std::string, GenericMesh> g_genericMeshes;
        std::unordered_map<std::string, IESProfile> g_iesProfiles;
        std::unordered_map<std::string, MeshBuffer> g_meshBuffers;
        std::unordered_map<std::string, SpriteSheetTexture> g_spriteSheetTextures;
        std::unordered_map<std::string, Texture> g_textures;

        std::vector<Material> g_materials;
        std::unordered_map<std::string, int32_t> g_materialIndices;
        std::vector<std::string> g_materialNamesByIndex;
        std::vector<std::string> g_textureNamesByBindlessIndex;

        Material CreateDefaultMaterial() {
            Material material;
            material.m_basecolor = GetTextureBindlessIndexByName("CheckerBoard_ALB");
            material.m_normal = GetTextureBindlessIndexByName("DefaultNRM");
            material.m_rma = GetTextureBindlessIndexByName("DefaultRMA");
            material.m_emissive = GetTextureBindlessIndexByName("Black");
            material.m_opacity = GetTextureBindlessIndexByName("White");
            material.m_hairMaps = GetTextureBindlessIndexByName("Black");
            return material;
        }
    }

    void CleanUp() {
        for (auto& object : g_genericMeshes) { object.second.CleanUp(); } g_genericMeshes.clear();
        for (auto& object : g_meshBuffers)   { object.second.CleanUp(); } g_meshBuffers.clear();
        for (auto& object : g_textures)      { object.second.CleanUp(); } g_textures.clear();

        g_animations.clear();
        g_iesProfiles.clear();
        g_materials.clear();
        g_materialIndices.clear();
        g_materialNamesByIndex.clear();
        g_spriteSheetTextures.clear();
        g_textureNamesByBindlessIndex.clear();
    }

    void AppendMemoryReport(MemoryTracker::MemoryReport& report) {
        if (!g_genericMeshes.empty()) {
            MemoryTracker::MemoryReportCategory& category = report.categories.emplace_back();
            category.name = "Generic Meshes";
            category.entries.reserve(g_genericMeshes.size());

            for (const auto& [name, genericMesh] : g_genericMeshes) {
                MemoryTracker::MemoryReportEntry& entry = category.entries.emplace_back();
                entry.name = name;
                entry.cpuBytes = genericMesh.GetCPUAllocatedByteCount();
                entry.gpuBytes = genericMesh.GetGPUAllocatedByteCount();
            }

            std::sort(category.entries.begin(), category.entries.end(), [](const auto& a, const auto& b) {
                return a.name < b.name;
            });
        }

        if (!g_meshBuffers.empty()) {
            MemoryTracker::MemoryReportCategory& category = report.categories.emplace_back();
            category.name = "Mesh Buffers";
            category.entries.reserve(g_meshBuffers.size());

            for (const auto& [name, meshBuffer] : g_meshBuffers) {
                MemoryTracker::MemoryReportEntry& entry = category.entries.emplace_back();
                entry.name = name;
                entry.cpuBytes = meshBuffer.GetCPUAllocatedByteCount();
                entry.gpuBytes = meshBuffer.GetGPUAllocatedByteCount();
            }

            std::sort(category.entries.begin(), category.entries.end(), [](const auto& a, const auto& b) {
                return a.name < b.name;
            });
        }

        if (!g_iesProfiles.empty()) {
            MemoryTracker::MemoryReportCategory& category = report.categories.emplace_back();
            category.name = "IES Profiles";
            category.entries.reserve(g_iesProfiles.size());

            for (const auto& [name, iesProfile] : g_iesProfiles) {
                MemoryTracker::MemoryReportEntry& entry = category.entries.emplace_back();
                entry.name = name;
                entry.cpuBytes = iesProfile.GetCPUAllocatedByteCount();
            }

            std::sort(category.entries.begin(), category.entries.end(), [](const auto& a, const auto& b) {
                return a.name < b.name;
            });
        }

        if (!g_textures.empty()) {
            MemoryTracker::MemoryReportCategory& category = report.categories.emplace_back();
            category.name = "Textures";
            category.entries.reserve(g_textures.size());

            for (const auto& [name, texture] : g_textures) {
                MemoryTracker::MemoryReportEntry& entry = category.entries.emplace_back();
                entry.name = name;
                entry.cpuBytes = texture.GetCPUAllocatedByteCount();
                entry.gpuBytes = texture.GetGPUAllocatedByteCount();
            }

            std::sort(category.entries.begin(), category.entries.end(), [](const auto& a, const auto& b) {
                return a.name < b.name;
            });
        }
    }

    // Animation

    Animation& CreateAnimation(const std::string& name) {
        auto it = g_animations.find(name);

        if (it != g_animations.end()) {
            Logging::Fatal() << "ResourceManager::CreateAnimation(..) failed: '" << name << "' already exists\n";
            return it->second;
        }

        auto result = g_animations.emplace(name, Animation());
        return result.first->second;
    }

    std::unordered_map<std::string, Animation>& GetAnimations() {
        return g_animations;
    }

    Animation& GetAnimation(const std::string& name) {
        auto it = g_animations.find(name);

        if (it == g_animations.end()) {
            Logging::Error() << "ResourceManager::GetAnimation(..) failed: '" << name << "' does not exist\n";

            static Animation invalid;
            return invalid;
        }

        return it->second;
    }

    Animation* GetAnimationPtr(const std::string& name) {
        auto it = g_animations.find(name);

        if (it == g_animations.end()) {
            Logging::Error() << "ResourceManager::GetAnimationPtr(..) failed: '" << name << "' does not exist\n";
            return nullptr;
        }

        return &it->second;
    }

    // Generic Mesh

    GenericMesh& CreateGenericMesh(const std::string& name) {
        auto it = g_genericMeshes.find(name);

        if (it != g_genericMeshes.end()) {
            Logging::Fatal() << "ResourceManager::CreateGenericMesh(..) failed: '" << name << "' already exists\n";
            return it->second;
        }

        auto result = g_genericMeshes.emplace(name, GenericMesh(name));
        return result.first->second;
    }

    GenericMesh& GetGenericMesh(const std::string& name) {
        auto it = g_genericMeshes.find(name);

        if (it == g_genericMeshes.end()) {
            Logging::Error() << "ResourceManager::GetGenericMesh(..) failed: '" << name << "' does not exist\n";

            static GenericMesh invalid;
            return invalid;
        }

        return it->second;
    }

    GenericMesh* GetGenericMeshPtr(const std::string& name) {
        auto it = g_genericMeshes.find(name);

        if (it == g_genericMeshes.end()) {
            Logging::Error() << "ResourceManager::GetGenericMeshPtr(..) failed: '" << name << "' does not exist\n";
            return nullptr;
        }

        return &it->second;
    }

    // IES Profile

    IESProfile& CreateIESProfile(const std::string& name) {
        auto it = g_iesProfiles.find(name);

        if (it != g_iesProfiles.end()) {
            Logging::Fatal() << "ResourceManager::CreateIESProfile(..) failed: '" << name << "' already exists\n";
            return it->second;
        }

        auto result = g_iesProfiles.emplace(name, IESProfile(name));
        return result.first->second;
    }

    IESProfile& GetIESProfile(const std::string& name) {
        auto it = g_iesProfiles.find(name);

        if (it == g_iesProfiles.end()) {
            Logging::Error() << "ResourceManager::GetIESProfile(..) failed: '" << name << "' does not exist\n";

            static IESProfile invalid;
            return invalid;
        }

        return it->second;
    }

    IESProfile* GetIESProfilePtr(const std::string& name) {
        auto it = g_iesProfiles.find(name);

        if (it == g_iesProfiles.end()) {
            Logging::Error() << "ResourceManager::GetIESProfilePtr(..) failed: '" << name << "' does not exist\n";
            return nullptr;
        }

        return &it->second;
    }

    // Material

    Material& CreateMaterial(const std::string& name) {
        auto it = g_materialIndices.find(name);

        if (it != g_materialIndices.end()) {
            Logging::Fatal() << "ResourceManager::CreateMaterial(..) failed: '" << name << "' already exists\n";
            return g_materials[it->second];
        }

        const int32_t index = static_cast<int32_t>(g_materials.size());
        g_materials.push_back(CreateDefaultMaterial());
        g_materialIndices.emplace(name, index);
        g_materialNamesByIndex.push_back(name);
        return g_materials.back();
    }

    std::vector<Material>& GetMaterials() {
        return g_materials;
    }

    std::vector<std::string> GetMaterialNames() {
        return g_materialNamesByIndex;
    }

    Material* GetDefaultMaterial() {
        auto it = g_materialIndices.find("CheckerBoard");
        if (it == g_materialIndices.end()) {
            Logging::Error() << "ResourceManager::GetDefaultMaterial() failed: 'CheckerBoard' does not exist\n";
            return nullptr;
        }

        return &g_materials[it->second];
    }

    Material* GetMaterialByIndex(int32_t index) {
        if (index >= 0 && static_cast<size_t>(index) < g_materials.size()) {
            return &g_materials[index];
        }

        Logging::Error() << "ResourceManager::GetMaterialByIndex(..) failed: index '" << index << "' is out of range\n";
        return GetDefaultMaterial();
    }

    Material* GetMaterialByName(const std::string& name) {
        auto it = g_materialIndices.find(name);
        if (it != g_materialIndices.end()) {
            return GetMaterialByIndex(it->second);
        }

        Logging::Error() << "ResourceManager::GetMaterialByName(..) failed: '" << name << "' does not exist\n";
        return GetDefaultMaterial();
    }

    int32_t GetMaterialIndexByName(const std::string& name) {
        auto it = g_materialIndices.find(name);
        return it != g_materialIndices.end() ? it->second : -1;
    }

    std::string GetMaterialNameByIndex(int32_t index) {
        if (index >= 0 && static_cast<size_t>(index) < g_materialNamesByIndex.size()) {
            return g_materialNamesByIndex[index];
        }

        Logging::Error() << "ResourceManager::GetMaterialNameByIndex(..) failed: index '" << index << "' is out of range\n";
        return UNDEFINED_STRING;
    }

    // Mesh Buffer

    MeshBuffer& CreateMeshBuffer(const std::string& name) {
        auto it = g_meshBuffers.find(name);

        if (it != g_meshBuffers.end()) {
            Logging::Fatal() << "ResourceManager::CreateMeshBuffer(..) failed: '" << name << "' already exists\n";
            return it->second;
        }

        auto result = g_meshBuffers.emplace(name, MeshBuffer(name));
        return result.first->second;
    }

    MeshBuffer& GetMeshBuffer(const std::string& name) {
        auto it = g_meshBuffers.find(name);

        if (it == g_meshBuffers.end()) {
            Logging::Error() << "ResourceManager::GetMeshBuffer(..) failed: '" << name << "' does not exist\n";

            static MeshBuffer invalid;
            return invalid;
        }

        return it->second;
    }

    MeshBuffer* GetMeshBufferPtr(const std::string& name) {
        auto it = g_meshBuffers.find(name);

        if (it == g_meshBuffers.end()) {
            Logging::Error() << "ResourceManager::GetMeshBufferPtr(..) failed: '" << name << "' does not exist\n";
            return nullptr;
        }

        return &it->second;
    }

    // Sprite Sheet Texture

    SpriteSheetTexture& CreateSpriteSheetTexture(const std::string& name) {
        auto it = g_spriteSheetTextures.find(name);

        if (it != g_spriteSheetTextures.end()) {
            Logging::Fatal() << "ResourceManager::CreateSpriteSheetTexture(..) failed: '" << name << "' already exists\n";
            return it->second;
        }

        auto result = g_spriteSheetTextures.try_emplace(name);
        return result.first->second;
    }

    SpriteSheetTexture& GetSpriteSheetTexture(const std::string& name) {
        auto it = g_spriteSheetTextures.find(name);

        if (it == g_spriteSheetTextures.end()) {
            Logging::Error() << "ResourceManager::GetSpriteSheetTexture(..) failed: '" << name << "' does not exist\n";

            static SpriteSheetTexture invalid;
            return invalid;
        }

        return it->second;
    }

    SpriteSheetTexture* GetSpriteSheetTexturePtr(const std::string& name) {
        auto it = g_spriteSheetTextures.find(name);
        return it != g_spriteSheetTextures.end() ? &it->second : nullptr;
    }

    // Texture

    Texture& CreateTexture(const std::string& name) {
        auto it = g_textures.find(name);

        if (it != g_textures.end()) {
            Logging::Fatal() << "ResourceManager::CreateTexture(..) failed: '" << name << "' already exists\n";
            return it->second;
        }

        auto result = g_textures.try_emplace(name);
        Texture& texture = result.first->second;
        const int32_t bindlessIndex = static_cast<int32_t>(g_textureNamesByBindlessIndex.size());

        texture.SetBindlessIndex(bindlessIndex);

        g_textureNamesByBindlessIndex.push_back(name);

        return texture;
    }

    std::unordered_map<std::string, Texture>& GetTextures() {
        return g_textures;
    }

    Texture* GetTextureByName(const std::string& name) {
        auto it = g_textures.find(name);

        if (it == g_textures.end()) {
            Logging::Error() << "ResourceManager::GetTextureByName() failed because texture '" << name << "' does not exist\n";
            return nullptr;
        }

        return &it->second;
    }

    Texture* GetTextureByBindlessIndex(int32_t bindlessIndex) {
        if (bindlessIndex < 0 || static_cast<size_t>(bindlessIndex) >= g_textureNamesByBindlessIndex.size()) {
            Logging::Error() << "ResourceManager::GetTextureByBindlessIndex() failed because bindless index '" << bindlessIndex << "' does not exist\n";
            return nullptr;
        }

        return GetTextureByName(g_textureNamesByBindlessIndex[bindlessIndex]);
    }

    int32_t GetTextureBindlessIndexByName(const std::string& name, bool ignoreWarning) {
        auto it = g_textures.find(name);
        if (it != g_textures.end()) {
            return it->second.GetBindlessIndex();
        }

        if (!ignoreWarning) {
            Logging::Fatal() << "ResourceManager::GetTextureBindlessIndexByName() failed because texture '" << name << "' does not exist\n";
        }

        return -1;
    }
}
