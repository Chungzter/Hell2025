#pragma once

#include "Unloved/Common/CreateInfo.h"
#include "Unloved/Common/Enums.h"
#include "Unloved/Common/Types.h"
#include "Unloved/ObjectId.h"

#include "Hell/Math/AABB.h"
#include "Hell/Render/TextureTypes.h"
#include "Hell/Render/VertexAttributes.h"

#include "Unloved/Bible/Info/ItemInfo.h"
#include "Unloved/Debug/DebugTypes.h"
#include "Unloved/Render/RendererEnums.h"
#include "Unloved/Render/RendererTypes.h"

#include <nlohmann/json.hpp>

#include <vector>

namespace Util {
    // Rendering
    void UpdateRenderItemAABB(RenderItem& renderItem);
    void UpdateRenderItemAABBFastA(RenderItem& renderItem);
    void UpdateRenderItemAABBFastB(RenderItem& renderItem);
    AABB ComputeWorldAABB(glm::vec3& localAabbMin, glm::vec3& localAabbMax, glm::mat4& modelMatrix);
    glm::mat4 GetLightSpaceMatrix(const glm::mat4& viewMatrix, glm::vec3 lightDir, const float viewportWidth, const float viewportHeight, const float fov, const float nearPlane, const float farPlane);
    std::vector<glm::vec4> GetFrustumCornersWorldSpace(const glm::mat4& projectionMatrix, const glm::mat4& viewMatrix);
    std::vector<glm::mat4> GetLightProjectionViews(const glm::mat4& viewMatrix, glm::vec3 lightDir, std::vector<float>& shadowCascadeLevels, const float viewportWidth, const float viewportHeight, const float fov);

}
