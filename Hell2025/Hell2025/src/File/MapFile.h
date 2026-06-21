#pragma once

#include "MapFileFormat.h"

#include <string>

namespace MapFile {

    void CopySignature(char* signatureBuffer, const std::string& signatureName);
}
