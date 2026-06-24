#include "Util.h"
#include "Hell/Math/OBB.h"

#include <algorithm>
#include <limits>

namespace Util {

    AABBRayResult RayIntersectAABB(glm::vec3 rayOrigin, glm::vec3 rayDir, float maxDistance, const AABB& aabb, const glm::mat4& worldTransform) {
        AABBRayResult result;
        OBB obb(aabb, worldTransform);
        OBBRayResult obbRayResult = obb.Raycast(rayOrigin, rayDir, maxDistance);

        if (obbRayResult.hitFound) {
            result.hitFound = true;
            result.hitPositionWorld = obbRayResult.hitPositionWorld;
            result.hitPositionLocal = obbRayResult.hitPositionLocal;
            result.hitNormalWorld = obbRayResult.hitNormalWorld;
            result.hitNormalLocal = obbRayResult.hitNormalLocal;
        }

        return result;
    }

    CubeRayResult CastCubeRay(const glm::vec3& rayOrigin, const glm::vec3 rayDir, std::vector<Transform>& cubeTransforms, float maxDistance) {
        CubeRayResult rayResult;
        rayResult.distanceToHit = std::numeric_limits<float>::max();
        const AABB localCubeBounds(glm::vec3(-0.5f), glm::vec3(0.5f));

        for (Transform& cubeTransform : cubeTransforms) {
            OBB obb(localCubeBounds, cubeTransform.to_mat4());
            OBBRayResult obbRayResult = obb.Raycast(rayOrigin, rayDir, std::min(maxDistance, rayResult.distanceToHit));

            if (obbRayResult.hitFound) {
                rayResult.distanceToHit = obbRayResult.distanceToHit;
                rayResult.hitFound = true;
                rayResult.hitPosition = obbRayResult.hitPositionWorld;
                rayResult.cubeTransform = cubeTransform;
                rayResult.hitNormal = obbRayResult.hitNormalWorld;
            }
        }

        return rayResult;
    }

    bool RayIntersectsTriangle(const glm::vec3& rayOrigin, const glm::vec3& rayDir, const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, float& t) {
        const float EPSILON = 1e-8f;
        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec3 h = glm::cross(rayDir, edge2);
        float a = glm::dot(edge1, h);
        if (fabs(a) < EPSILON) {
            return false; // Ray is parallel to the triangle.
        }
        float f = 1.0f / a;
        glm::vec3 s = rayOrigin - v0;
        float u = f * glm::dot(s, h);
        if (u < 0.0f || u > 1.0f) {
            return false;
        }
        glm::vec3 q = glm::cross(s, edge1);
        float v = f * glm::dot(rayDir, q);
        if (v < 0.0f || u + v > 1.0f) {
            return false;
        }
        t = f * glm::dot(edge2, q); // Distance along the ray to the intersection.
        return t > EPSILON;
    }

    glm::vec3 GetMouseRayDir(glm::mat4 projection, glm::mat4 view, int windowWidth, int windowHeight, int mouseX, int mouseY) {
        float x = (2.0f * mouseX) / (float)windowWidth - 1.0f;
        float y = 1.0f - (2.0f * mouseY) / (float)windowHeight;
        float z = 1.0f;
        glm::vec3 ray_nds = glm::vec3(x, y, z);
        glm::vec4 ray_clip = glm::vec4(ray_nds.x, ray_nds.y, ray_nds.z, 1.0f);
        glm::vec4 ray_eye = glm::inverse(projection) * ray_clip;
        ray_eye = glm::vec4(ray_eye.x, ray_eye.y, ray_eye.z, 0.0f);
        glm::vec4 inv_ray_wor = (inverse(view) * ray_eye);
        glm::vec3 ray_wor = glm::vec3(inv_ray_wor.x, inv_ray_wor.y, inv_ray_wor.z);
        ray_wor = normalize(ray_wor);
        return ray_wor;
    }

    bool RayIntersectsSphere(const glm::vec3& rayOrigin, const glm::vec3& rayDir, const glm::vec3& spherePosition, float sphereRadius) {
        glm::dvec3 oc = glm::dvec3(rayOrigin) - glm::dvec3(spherePosition);
        double b = glm::dot(oc, glm::dvec3(rayDir));
        double c = glm::dot(oc, oc) - (double)sphereRadius * (double)sphereRadius;
        double discriminant = b * b - c;

        if (discriminant < 0.0) return false;
        return (-b + glm::sqrt(discriminant)) >= 0.0;
    }
}
