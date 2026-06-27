#pragma once

#include "Unloved/Systems/Blood/BloodScreenSpaceDecal.h"
#include "Unloved/Systems/Blood/BloodVAT.h"

#include <vector>

namespace Unloved::BloodSystem {
    void AddBloodVAT(const glm::vec3& position, const glm::vec3& direction);
    void AddBloodScreenSpaceDecal(BloodScreenSpaceDecalCreateInfo createInfo);

    std::vector<BloodScreenSpaceDecal>& GetBloodScreenSpaceDecals();
    std::vector<BloodVAT>& GetBloodVAT();

    void Update(float deltaTime);
    void CleanUp();
}
