#pragma once
#include <Game/Types.h>
#include <Game/CreateInfo.h>

namespace Unloved {

struct House {
    void SetFilename(const std::string& filename);
    void SetCreateInfoCollection(CreateInfoCollection& createInfoCollection);

    CreateInfoCollection& GetCreateInfoCollection()     { return m_createInfoCollection; }
    const std::string& GetFilename() const              { return m_filename; }

private:
    CreateInfoCollection m_createInfoCollection;
    std::string m_filename = UNDEFINED_STRING;
};
}
