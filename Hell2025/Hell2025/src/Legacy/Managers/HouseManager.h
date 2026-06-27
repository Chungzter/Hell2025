#pragma once
//#include <Game/CreateInfo.h>
#include "Unloved/Objects/House/House.h"

namespace HouseManager {
    using namespace Unloved;

    void Init();
    void LoadHouse(const std::string& filename);
    void SaveHouse(const std::string& filename);
    void UpdateCreateInfoCollectionFromWorld(const std::string& houseName);

    House* GetHouseByName(const std::string& filename);
}