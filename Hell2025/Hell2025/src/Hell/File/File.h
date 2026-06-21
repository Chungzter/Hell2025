#pragma once
#include "FileInfo.h"

#include <string>
#include <vector>

struct IESProfile;

namespace Hell::File {

std::vector<FileInfo> IterateDirectory(const std::string& directory, std::vector<std::string> extensions = {});
bool LoadIESProfile(const std::string& filepath, IESProfile& outData);

}
