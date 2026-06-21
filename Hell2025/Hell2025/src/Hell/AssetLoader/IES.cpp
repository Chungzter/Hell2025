#include "AssetLoader.h"

#include "Hell/Logging.h"
#include "Hell/ResourceManagement/Types/IESProfile.h"

#include <tinyies/tiny_ies.hpp>

#include <utility>

namespace Hell::AssetLoader {

    bool LoadIES(const std::string& path, IESProfile& outProfile) {
        tiny_ies<float>::light ies;
        std::string error;
        std::string warning;

        if (!tiny_ies<float>::load_ies(path, error, warning, ies)) {
            Logging::Error() << "AssetLoader::LoadIES() failed to load '" << path << "': " << error << "\n";
            return false;
        }

        if (!warning.empty()) {
            Logging::Warning() << "AssetLoader::LoadIES() warning for '" << path << "': " << warning << "\n";
        }

        IESProfile loadedProfile(outProfile.m_name);
        loadedProfile.m_fileInfo = outProfile.m_fileInfo;
        loadedProfile.m_textureIndex = outProfile.m_textureIndex;
        loadedProfile.m_verticalAngles = std::move(ies.vertical_angles);
        loadedProfile.m_horizontalAngles = std::move(ies.horizontal_angles);
        loadedProfile.m_candelaValues = std::move(ies.candela);
        loadedProfile.m_horizontalAngleCount = ies.number_horizontal_angles;
        loadedProfile.m_verticalAngleCount = ies.number_vertical_angles;
        loadedProfile.m_maxIntensity = ies.max_candela;
        loadedProfile.RecalculateDerivedValues();

        outProfile = std::move(loadedProfile);
        return true;
    }
}
