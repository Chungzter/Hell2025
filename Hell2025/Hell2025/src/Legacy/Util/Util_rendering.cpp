#include "Util.h"
#include "Unloved/Config/Config.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include <array>

#include <iostream> // TODO clean up logging

namespace Util {

    void UpdateRenderItemAABBFastA(RenderItem& renderItem) {
        Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(renderItem.meshId);
        if (!mesh) return;

        renderItem.aabbMin = glm::vec4(std::numeric_limits<float>::max());
        renderItem.aabbMax = glm::vec4(-std::numeric_limits<float>::max());

        const std::array<glm::vec4, 8> corners = {
            glm::vec4(renderItem.modelMatrix * glm::vec4(mesh->aabbMin.x, mesh->aabbMax.y, mesh->aabbMax.z, 1.0f)),
            glm::vec4(renderItem.modelMatrix * glm::vec4(mesh->aabbMax.x, mesh->aabbMax.y, mesh->aabbMax.z, 1.0f)),
            glm::vec4(renderItem.modelMatrix * glm::vec4(mesh->aabbMin.x, mesh->aabbMin.y, mesh->aabbMax.z, 1.0f)),
            glm::vec4(renderItem.modelMatrix * glm::vec4(mesh->aabbMax.x, mesh->aabbMin.y, mesh->aabbMax.z, 1.0f)),
            glm::vec4(renderItem.modelMatrix * glm::vec4(mesh->aabbMin.x, mesh->aabbMax.y, mesh->aabbMin.z, 1.0f)),
            glm::vec4(renderItem.modelMatrix * glm::vec4(mesh->aabbMax.x, mesh->aabbMax.y, mesh->aabbMin.z, 1.0f)),
            glm::vec4(renderItem.modelMatrix * glm::vec4(mesh->aabbMin.x, mesh->aabbMin.y, mesh->aabbMin.z, 1.0f)),
            glm::vec4(renderItem.modelMatrix * glm::vec4(mesh->aabbMax.x, mesh->aabbMin.y, mesh->aabbMin.z, 1.0f))
        };

        for (const auto& corner : corners) {
            renderItem.aabbMin = glm::min(renderItem.aabbMin, corner);
            renderItem.aabbMax = glm::max(renderItem.aabbMax, corner);
        }
    }

    void UpdateRenderItemAABBFastB(RenderItem& renderItem) {
        Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(renderItem.meshId);
        if (!mesh) return;

        glm::vec3& localMin = mesh->aabbMin;
        glm::vec3& localMax = mesh->aabbMax;
        glm::vec3 localCenter = (localMin + localMax) * 0.5f;
        glm::vec3 localHalfExtents = (localMax - localMin) * 0.5f;
        glm::vec3 worldCenter = glm::vec3(renderItem.modelMatrix * glm::vec4(localCenter, 1.0f));

        const glm::mat3 R = glm::mat3(renderItem.modelMatrix);

        glm::vec3 abs_row0 = glm::abs(glm::vec3(R[0][0], R[1][0], R[2][0]));
        glm::vec3 abs_row1 = glm::abs(glm::vec3(R[0][1], R[1][1], R[2][1]));
        glm::vec3 abs_row2 = glm::abs(glm::vec3(R[0][2], R[1][2], R[2][2]));

        glm::vec3 worldHalfExtents = glm::vec3(
            glm::dot(abs_row0, localHalfExtents),
            glm::dot(abs_row1, localHalfExtents),
            glm::dot(abs_row2, localHalfExtents)
        );

        glm::vec3 worldMin = worldCenter - worldHalfExtents;
        glm::vec3 worldMax = worldCenter + worldHalfExtents;

        renderItem.aabbMin = glm::vec4(worldMin, 1.0f);
        renderItem.aabbMax = glm::vec4(worldMax, 1.0f);
    }

    void UpdateRenderItemAABB(RenderItem& renderItem) {
        Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(renderItem.meshId);
        if (!mesh) return;

        glm::vec3 aabbMin = glm::vec3(std::numeric_limits<float>::max());
        glm::vec3 aabbMax = glm::vec3(-std::numeric_limits<float>::max());

        const std::array<glm::vec3, 8> corners = {
            glm::vec3(renderItem.modelMatrix * glm::vec4(mesh->aabbMin.x, mesh->aabbMax.y, mesh->aabbMax.z, 1.0f)),
            glm::vec3(renderItem.modelMatrix * glm::vec4(mesh->aabbMax.x, mesh->aabbMax.y, mesh->aabbMax.z, 1.0f)),
            glm::vec3(renderItem.modelMatrix * glm::vec4(mesh->aabbMin.x, mesh->aabbMin.y, mesh->aabbMax.z, 1.0f)),
            glm::vec3(renderItem.modelMatrix * glm::vec4(mesh->aabbMax.x, mesh->aabbMin.y, mesh->aabbMax.z, 1.0f)),
            glm::vec3(renderItem.modelMatrix * glm::vec4(mesh->aabbMin.x, mesh->aabbMax.y, mesh->aabbMin.z, 1.0f)),
            glm::vec3(renderItem.modelMatrix * glm::vec4(mesh->aabbMax.x, mesh->aabbMax.y, mesh->aabbMin.z, 1.0f)),
            glm::vec3(renderItem.modelMatrix * glm::vec4(mesh->aabbMin.x, mesh->aabbMin.y, mesh->aabbMin.z, 1.0f)),
            glm::vec3(renderItem.modelMatrix * glm::vec4(mesh->aabbMax.x, mesh->aabbMin.y, mesh->aabbMin.z, 1.0f))
        };

        for (const auto& corner : corners) {
            aabbMin = glm::min(aabbMin, corner);
            aabbMax = glm::max(aabbMax, corner);
        }

        renderItem.aabbMin = glm::vec4(aabbMin, 0.0f);
        renderItem.aabbMax = glm::vec4(aabbMax, 0.0f);
    }

    AABB ComputeWorldAABB(glm::vec3& localAabbMin, glm::vec3& localAabbMax, glm::mat4& modelMatrix) {
        glm::vec3 worldAabbMin = glm::vec3(std::numeric_limits<float>::max());
        glm::vec3 worldAabbMax = glm::vec3(-std::numeric_limits<float>::max());

        std::vector<glm::vec3> corners = {
            modelMatrix* glm::vec4(localAabbMin.x, localAabbMax.y, localAabbMax.z, 1.0f),
            modelMatrix* glm::vec4(localAabbMax.x, localAabbMax.y, localAabbMax.z, 1.0f),
            modelMatrix* glm::vec4(localAabbMin.x, localAabbMin.y, localAabbMax.z, 1.0f),
            modelMatrix* glm::vec4(localAabbMax.x, localAabbMin.y, localAabbMax.z, 1.0f),
            modelMatrix* glm::vec4(localAabbMin.x, localAabbMax.y, localAabbMin.z, 1.0f),
            modelMatrix* glm::vec4(localAabbMax.x, localAabbMax.y, localAabbMin.z, 1.0f),
            modelMatrix* glm::vec4(localAabbMin.x, localAabbMin.y, localAabbMin.z, 1.0f),
            modelMatrix* glm::vec4(localAabbMax.x, localAabbMin.y, localAabbMin.z, 1.0f)
        };

        for (glm::vec3& corner : corners) {
            worldAabbMin = glm::min(worldAabbMin, corner);
            worldAabbMax = glm::max(worldAabbMax, corner);
        }

        return AABB(worldAabbMin, worldAabbMax);
    }

    std::vector<glm::vec4> GetFrustumCornersWorldSpace(const glm::mat4& projectionMatrix, const glm::mat4& viewMatrix) {
        const auto inv = glm::inverse(projectionMatrix * viewMatrix);

        std::vector<glm::vec4> frustumCorners;
        for (unsigned int x = 0; x < 2; ++x) {
            for (unsigned int y = 0; y < 2; ++y) {
                for (unsigned int z = 0; z < 2; ++z) {
                    const glm::vec4 pt = inv * glm::vec4(2.0f * x - 1.0f, 2.0f * y - 1.0f, 2.0f * z - 1.0f, 1.0f);
                    frustumCorners.push_back(pt / pt.w);
                }
            }
        }
        return frustumCorners;
    }

    glm::mat4 GetLightSpaceMatrix(const glm::mat4& viewMatrix, glm::vec3 lightDir, const float viewportWidth, const float viewportHeight, const float fov, const float nearPlane, const float farPlane) {
        const auto proj = glm::perspective(fov, viewportWidth / viewportHeight, nearPlane, farPlane);

        const auto corners = GetFrustumCornersWorldSpace(proj, viewMatrix);

        glm::vec3 center = glm::vec3(0, 0, 0);
        for (const glm::vec3& v : corners) {
            center += glm::vec3(v);

        }
        center /= corners.size();

        const glm::mat4 lightView = glm::lookAt(center + lightDir, center, glm::vec3(0.0f, 1.0f, 0.0f));

        float minX = std::numeric_limits<float>::max();
        float maxX = std::numeric_limits<float>::lowest();
        float minY = std::numeric_limits<float>::max();
        float maxY = std::numeric_limits<float>::lowest();
        float minZ = std::numeric_limits<float>::max();
        float maxZ = std::numeric_limits<float>::lowest();
        for (const glm::vec4& v : corners) {
            const glm::vec4 trf = lightView * v;
            minX = std::min(minX, trf.x);
            maxX = std::max(maxX, trf.x);
            minY = std::min(minY, trf.y);
            maxY = std::max(maxY, trf.y);
            minZ = std::min(minZ, trf.z);
            maxZ = std::max(maxZ, trf.z);
        }

        constexpr float zMult = 10.0f; // Tune this parameter according to the scene
        if (minZ < 0) {
            minZ *= zMult;
        }
        else {
            minZ /= zMult;
        }
        if (maxZ < 0) {
            maxZ /= zMult;
        }
        else {
            maxZ *= zMult;
        }

        const glm::mat4 lightProjection = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);
        return lightProjection * lightView;
    }

    #include <iostream>
    #include <cfloat>
    #include <glm/glm.hpp>
    #include <glm/gtc/matrix_transform.hpp>

    #include <iostream>
    #include <cfloat>
    #include <glm/glm.hpp>
    #include <glm/gtc/matrix_transform.hpp>


    // Builds a stable cascade matrix by snapping ortho bounds to texel grid, with debug info
    glm::mat4 GetLightSpaceMatrix5(const glm::mat4& viewMatrix,
                                  glm::vec3 lightDir,
                                  const float viewportWidth,
                                  const float viewportHeight,
                                  const float fov,
                                  const float nearPlane,
                                  const float farPlane)
    {
        const int shadowMapResolution = 2048;

        // 1) get world-space frustum corners
        const auto proj = glm::perspective(fov, viewportWidth / viewportHeight, nearPlane, farPlane);
        const auto corners = GetFrustumCornersWorldSpace(proj, viewMatrix);

        // 2) compute world-space center
        glm::vec3 center(0.0f);
        for (auto& v : corners) center += glm::vec3(v);
        center /= float(corners.size());

        // 3) initial light view
        glm::mat4 lightView = glm::lookAt(center + lightDir, center, glm::vec3(0, 1, 0));

        // 4) compute ortho bounds in light space
        float minX = FLT_MAX, maxX = -FLT_MAX;
        float minY = FLT_MAX, maxY = -FLT_MAX;
        float minZ = FLT_MAX, maxZ = -FLT_MAX;
        for (auto& v : corners) {
            glm::vec4 tr = lightView * v;
            minX = std::min(minX, tr.x); maxX = std::max(maxX, tr.x);
            minY = std::min(minY, tr.y); maxY = std::max(maxY, tr.y);
            minZ = std::min(minZ, tr.z); maxZ = std::max(maxZ, tr.z);
        }

        // 5) compute texel sizes for X/Y
        float width = maxX - minX;
        float height = maxY - minY;
        float texelSizeX = width / float(shadowMapResolution);
        float texelSizeY = height / float(shadowMapResolution);

        // 6) snap min/max edges to grid
        float snapMinX = std::floor(minX / texelSizeX) * texelSizeX;
        float snapMaxX = snapMinX + std::ceil(width / texelSizeX) * texelSizeX;
        float snapMinY = std::floor(minY / texelSizeY) * texelSizeY;
        float snapMaxY = snapMinY + std::ceil(height / texelSizeY) * texelSizeY;

        // 7) optionally expand Z bounds
        constexpr float zMult = 10.0f;
        float zNear = (minZ < 0 ? minZ * zMult : minZ / zMult);
        float zFar = (maxZ < 0 ? maxZ / zMult : maxZ * zMult);

        // 8) rebuild ortho with snapped bounds
        glm::mat4 lightProj = glm::ortho(snapMinX, snapMaxX, snapMinY, snapMaxY, zNear, zFar);
        glm::mat4 lightSpaceMatrix = lightProj * lightView;

        // --- DEBUG: check sub-texel drift ---
        static glm::mat4 prevMatrix = glm::mat4(1.0f);
        glm::vec2 delta = glm::vec2(
            lightSpaceMatrix[3].x - prevMatrix[3].x,
            lightSpaceMatrix[3].y - prevMatrix[3].y
        );
        glm::vec2 deltaTex = glm::vec2(delta.x / texelSizeX, delta.y / texelSizeY);
        glm::vec2 frac = glm::vec2(
            deltaTex.x - std::round(deltaTex.x),
            deltaTex.y - std::round(deltaTex.y)
        );
        std::cout << "delta/world: (" << delta.x << ", " << delta.y << ") "
            << "delta/texels: (" << deltaTex.x << ", " << deltaTex.y << ") "
            << "frac: (" << frac.x << ", " << frac.y << ")" << std::endl;
        prevMatrix = lightSpaceMatrix;

        return lightSpaceMatrix;
    }


    std::vector<glm::mat4> GetLightProjectionViews(const glm::mat4& viewMatrix, glm::vec3 lightDir, std::vector<float>& shadowCascadeLevels, const float viewportWidth, const float viewportHeight, const float fov) {
        std::vector<glm::mat4> ret;

        for (size_t i = 0; i < shadowCascadeLevels.size() + 1; ++i) {
            if (i == 0) {
                ret.push_back(GetLightSpaceMatrix(viewMatrix, lightDir, viewportWidth, viewportHeight, fov, Config::GetNearPlane(), shadowCascadeLevels[i]));
            }
            else if (i < shadowCascadeLevels.size()) {
                ret.push_back(GetLightSpaceMatrix(viewMatrix, lightDir, viewportWidth, viewportHeight, fov, shadowCascadeLevels[i - 1], shadowCascadeLevels[i]));
            }
            else {
                ret.push_back(GetLightSpaceMatrix(viewMatrix, lightDir, viewportWidth, viewportHeight, fov, shadowCascadeLevels[i - 1], Config::GetFarPlane()));
            }
        }
        return ret;
    }
}
