#include "Physics.h"
#include "Hell/Physics/Types/HeightField.h"
#include <vector>

#include "Core/GameOLD.h"
#include "Editor/Editor.h"
#include "Util/Util.h"

#include "Renderer/Renderer.h"

namespace Hell::Physics {

void ActivateAllHeightFields() {
        for (HeightField& heightField : GetHeightFields()) {
            heightField.ActivatePhsyics();
        }
    }
}
