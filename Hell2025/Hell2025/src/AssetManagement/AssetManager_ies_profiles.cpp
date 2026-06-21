#include "AssetManager.h"
#include "Hell/ResourceManagement/ResourceManager.h"

namespace AssetManager{

    IESProfile* GetIESProfileByName(const std::string& name) {
        return Hell::ResourceManager::GetIESProfilePtr(name);
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
