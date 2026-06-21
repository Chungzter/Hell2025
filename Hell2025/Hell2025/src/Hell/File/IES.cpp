#include "File.h"

#include "Hell/Logging.h"
#include "Hell/ResourceManagement/Types/IESProfile.h"

#include <tinyies/tiny_ies.hpp>
#include <utility>

namespace Hell::File {

bool LoadIESProfile(const std::string& filepath, IESProfile& outData) {
    tiny_ies<float>::light ies;
    std::string error;
    std::string warning;

    if (!tiny_ies<float>::load_ies(filepath, error, warning, ies)) {
        Logging::Error() << "File::LoadIES() failed to load '" << filepath << "': " << error << "\n";
        return false;
    }

    if (!warning.empty()) {
        Logging::Warning() << "File::LoadIES() warning for '" << filepath << "': " << warning << "\n";
    }

    IESProfile loadedProfile(outData.m_name);
    loadedProfile.m_fileInfo = outData.m_fileInfo;
    loadedProfile.m_textureIndex = outData.m_textureIndex;
    loadedProfile.m_verticalAngles = std::move(ies.vertical_angles);
    loadedProfile.m_horizontalAngles = std::move(ies.horizontal_angles);
    loadedProfile.m_candelaValues = std::move(ies.candela);
    loadedProfile.m_horizontalAngleCount = ies.number_horizontal_angles;
    loadedProfile.m_verticalAngleCount = ies.number_vertical_angles;
    loadedProfile.m_maxIntensity = ies.max_candela;
    loadedProfile.RecalculateDerivedValues();

    outData = std::move(loadedProfile);
    return true;
}

}
