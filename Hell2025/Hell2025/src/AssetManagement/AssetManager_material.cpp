#include "AssetManager.h"

#include "Hell/Logging.h"

namespace AssetManager {

    bool FileInfoIsAlbedoTexture(const FileInfo& fileInfo) {
        if (fileInfo.name.size() >= 4 && fileInfo.name.substr(fileInfo.name.size() - 4) == "_ALB") {
            return true;
        }
        return false;
    }

    std::string GetMaterialNameFromFileInfo(const FileInfo& fileInfo) {
        const std::string suffix = "_ALB";
        if (fileInfo.name.size() > suffix.size() && fileInfo.name.substr(fileInfo.name.size() - suffix.size()) == suffix) {
            return fileInfo.name.substr(0, fileInfo.name.size() - suffix.size());
        }
        return "";
    }

    void CreateGoldenVariant(const std::string& srcName, const std::string& dstName) {
        Material* material = AssetManager::GetMaterialByName(srcName);
        if (material) {
            std::vector<Material>& materials = GetMaterials();
            Material& goldenVariant = materials.emplace_back(Material());
            goldenVariant.m_name = dstName;
            goldenVariant.m_basecolor = GetTextureBindlessIndexByName("Gold_ALB", true);
            goldenVariant.m_normal = material->m_normal;
            goldenVariant.m_rma = GetTextureBindlessIndexByName("Gold_RMA", true);
        }
    }

    enum struct MaterialType {
        ALB, // RGB: Base color
        NRM, // RGB: Normal map
        RMA, // R:   Roughness G: metallic B: ao
        EMI, // RGB: Emissive
        OPA, // RGB: Opacity
        HAR, // RG:  flow map  G: strand   B: root factor
        UNDEFINED
    };

    MaterialType GetMaterialType(const std::string& textureName) {
        if (textureName.size() < 3) return MaterialType::UNDEFINED;

        std::string_view suffix(textureName.data() + textureName.size() - 3, 3);

        if (suffix == "ALB") return MaterialType::ALB;
        if (suffix == "NRM") return MaterialType::NRM;
        if (suffix == "RMA") return MaterialType::RMA;
        if (suffix == "EMI") return MaterialType::EMI;
        if (suffix == "OPA") return MaterialType::OPA;
        if (suffix == "HAR") return MaterialType::HAR;

        return MaterialType::UNDEFINED;
    }

    std::string GetMaterialName(const std::string& textureName) {
        if (textureName.size() < 4) return "UNDEFINED";
        return textureName.substr(0, textureName.size() - 4);
    }

    void SetFallbackIfMissing(int& textureIndex, const std::string& textureName) {
        if (textureIndex == -1) {
            textureIndex = GetTextureBindlessIndexByName(textureName);
        }
    }

    void BuildMaterials() {
        std::vector<Material>& materials = GetMaterials();
        std::vector<Texture>& textures = GetTextures();

        // Start fresh
        materials.clear();

        // For any texture with an ALB suffix, create a material, and store indices for accompanying material textures
        for (Texture& texture : textures) {
            MaterialType materialType= GetMaterialType(texture.GetFileName());

            // If we found an Albedo texture then create a material
            if (materialType == MaterialType::ALB) {

                std::string materialName = GetMaterialName(texture.GetFileName());

                Material& material = materials.emplace_back();
                material.m_name = materialName;
                material.m_basecolor = GetTextureBindlessIndexByName(materialName + "_ALB");
                material.m_normal = GetTextureBindlessIndexByName(materialName + "_NRM");
                material.m_rma = GetTextureBindlessIndexByName(materialName + "_RMA");
                material.m_emissive = GetTextureBindlessIndexByName(materialName + "_EMI");
                material.m_opacity = GetTextureBindlessIndexByName(materialName + "_OPA");
                material.m_hairMaps = GetTextureBindlessIndexByName(materialName + "_HAR");

                SetFallbackIfMissing(material.m_normal, "DefaultNRM");
                SetFallbackIfMissing(material.m_rma, "DefaultRMA");
                SetFallbackIfMissing(material.m_emissive, "Black");
                SetFallbackIfMissing(material.m_opacity, "White");
                SetFallbackIfMissing(material.m_hairMaps, "Black");
            }
        }
    }

    Material* GetDefaultMaterial() {
        int index = GetMaterialIndexByName(DEFAULT_MATERIAL_NAME);
        return GetMaterialByIndex(index);
    }

    Material* GetMaterialByName(const std::string& name) {
        int index = GetMaterialIndexByName(name);
        return GetMaterialByIndex(index);
    }

    Material* GetMaterialByIndex(int index) {
        std::vector<Material>& materials = GetMaterials();
        if (index >= 0 && index < materials.size()) {
            Material* material = &materials[index];
            Texture* baseColor = AssetManager::GetTextureByBindlessIndex(material->m_basecolor);
            Texture* normal = AssetManager::GetTextureByBindlessIndex(material->m_normal);
            Texture* rma = AssetManager::GetTextureByBindlessIndex(material->m_rma);
            if (baseColor && baseColor->BakeComplete() &&
                normal && normal->BakeComplete() &&
                rma && rma->BakeComplete()) {
                return &materials[index];
            }
            else {
                return GetDefaultMaterial();
            }
        }
        else {
            //std::cout << "AssetManager::GetMaterialByIndex(int index) failed because index '" << index << "' is out of range. Size is " << g_materials.size() << "!\n";
            return GetDefaultMaterial();
        }
    }

    std::string GetMaterialNameByIndex(int index) {
        std::vector<Material>& materials = GetMaterials();
        if (index >= 0 && index < materials.size()) {
            return materials[index].m_name;
        }

        Logging::Error() << "AssetManager::GetMaterialNameByIndex(..) failed because index " << index << " was out of range of size << " << materials.size() << "\n";
        return "UNDEFINED_MATERIAL_NAME";
    }

    int GetMaterialIndexByName(const std::string& name) {
        std::unordered_map<std::string, int>& indexMap = GetMaterialIndexMap();

        auto it = indexMap.find(name);
        if (it != indexMap.end()) {
            return it->second;
        }
        else {
            //std::cout << "AssetManager::GetMaterialIndexByName(const std::string& name) failed because '" << name << "' does not exist\n";
            return -1;
        }
    }
}