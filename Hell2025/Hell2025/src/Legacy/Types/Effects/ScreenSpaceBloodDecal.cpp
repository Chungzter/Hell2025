#include "ScreenSpaceBloodDecal.h"
#include "Util.h"

void ScreenSpaceBloodDecal::Init(ScreenSpaceBloodDecalCreateInfo createInfo) {
    Transform transform;
    transform.position = createInfo.position;
    transform.rotation.y = Util::RandomFloat(0.0f, HELL_PI * 2);
    transform.scale = glm::vec3(2.0f);

    m_modelMatrix = transform.to_mat4();
    m_inverseModelMatrix = glm::inverse(m_modelMatrix);

    m_type = Util::RandomInt(0, 3);
}

