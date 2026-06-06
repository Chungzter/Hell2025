#include "AssetManager.h"
#include "Backend/BackEnd.h"
#include "Util.h"

#include <Hell/Logging.h>

namespace AssetManager{

    void LoadIESProfiles() {
        std::vector<FileInfo> files = Util::IterateDirectory("res/ies_profiles", { "ies" });

        std::vector<IESProfile>& profiles = GetIESProfiles();
        profiles.clear();
        profiles.reserve(files.size()); 
        
        // Prevent reallocations during the loop
        size_t newCapacity = GetTextureCount() + files.size();
        ReserveTextureStorage(newCapacity);

        for (FileInfo& fileInfo : files) {

            // Create texture
            Texture& texture = CreateNewTexture();
            texture.SetMinFilter(TextureFilter::LINEAR);
            texture.SetMagFilter(TextureFilter::LINEAR);

            int32_t textureIndex = GetTextureCount() - 1;

            // Create IES Profile
            IESProfile& profile = profiles.emplace_back();
            profile.Load(fileInfo, textureIndex);

            // OpenGL upload
            if (BackEnd::GetAPI() == API::OPENGL) {
                OpenGLTexture& glTexture = texture.GetGLTexture();

                int width = profile.GetVerticalAngleCount();
                int height = profile.GetHorizontalAngleCount();
                const float* data = profile.GetCandelaValues().data();

                glTexture.Create(width, height, GL_R32F, 1);
                glTexture.UploadData(data);

                glTextureParameteri(glTexture.GetHandle(), GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // TODO: Make wrapper functions for me
                glTextureParameteri(glTexture.GetHandle(), GL_TEXTURE_WRAP_S, GL_REPEAT);        // TODO: Make wrapper functions for me

                glTexture.MakeBindlessTextureResident();
            }
            // Vulkan upload
            else if (BackEnd::GetAPI() == API::VULKAN) {
                Logging::ToDo() << "LoadIESProfiles() has no Vulkan upload path\n";
            }
        }
    }

    IESProfile* GetIESProfileByName(const std::string& name) {
        std::vector<IESProfile>& profiles = GetIESProfiles();

        for (IESProfile& profile : profiles) {
            if (profile.GetName() == name) {
                return &profile;
            }
        }

        return nullptr;
    }

    IESProfile* GetIESProfileByIESProfileType(IESProfileType type) {
        switch (type) {
            case IESProfileType::LAMP_0: return GetIESProfileByName("Lamp0");
            case IESProfileType::LAMP_1: return GetIESProfileByName("Lamp1");
            case IESProfileType::LAMP_2: return GetIESProfileByName("Lamp2");
            case IESProfileType::LAMP_3: return GetIESProfileByName("Lamp3");
            case IESProfileType::LAMP_4: return GetIESProfileByName("Lamp4");
            case IESProfileType::LAMP_5: return GetIESProfileByName("Lamp5");
            case IESProfileType::LAMP_6: return GetIESProfileByName("Lamp6");
            case IESProfileType::LAMP_7: return GetIESProfileByName("Lamp7");
            case IESProfileType::LAMP_8: return GetIESProfileByName("Lamp8");
            case IESProfileType::LAMP_9: return GetIESProfileByName("Lamp9");
            case IESProfileType::LAMP_10: return GetIESProfileByName("Lamp10");
            case IESProfileType::LAMP_11: return GetIESProfileByName("Lamp11");
            default:                     return nullptr;
        }
    }
}