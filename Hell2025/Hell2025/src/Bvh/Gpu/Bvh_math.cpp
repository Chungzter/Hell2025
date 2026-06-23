#include "BvhOLD.h"

namespace Bvh::Gpu {
    uint32_t FloatToBits(float x);
    float BitsToFloat(uint32_t u);
    float AddUlpMagnitude(float x, unsigned ulps);
    float IntersectAxisMin(const RayData& rayData, int axis, float p);
    float IntersectAxisMax(const RayData& rayData, int axis, float p);
    float RobustMin(float x, float y);
    float RobustMax(float x, float y);
    bool IntersectNode(const RayData& rayData, const glm::vec3& aabbBoundsMin, const glm::vec3& aabbBoundsMax, float& t);

    RayData ComputeRayData(const glm::vec3& rayOrigin, const glm::vec3& rayDir, float minDistance, float maxDistance) {
        RayData rayData;
        rayData.origin[0] = rayOrigin.x;
        rayData.origin[1] = rayOrigin.y;
        rayData.origin[2] = rayOrigin.z;
        rayData.dir[0] = rayDir.x;
        rayData.dir[1] = rayDir.y;
        rayData.dir[2] = rayDir.z;
        rayData.minDistance = minDistance;
        rayData.maxDistance = maxDistance;

        // Compute octant
        rayData.octant[0] = signbit(rayDir.x) ? 1 : 0;
        rayData.octant[1] = signbit(rayDir.y) ? 1 : 0;
        rayData.octant[2] = signbit(rayDir.z) ? 1 : 0;

        // Compute inverse direction
        rayData.invDir[0] = 1.0f / rayDir.x;
        rayData.invDir[1] = 1.0f / rayDir.y;
        rayData.invDir[2] = 1.0f / rayDir.z;

        // Compute padding inverse direction
        rayData.paddedInvDir[0] = AddUlpMagnitude(rayData.invDir[0], 2);
        rayData.paddedInvDir[1] = AddUlpMagnitude(rayData.invDir[1], 2);
        rayData.paddedInvDir[2] = AddUlpMagnitude(rayData.invDir[2], 2);

        return rayData;
    }

    uint32_t FloatToBits(float x) {
        uint32_t u;
        memcpy(&u, &x, sizeof(float));
        return u;
    }

    float BitsToFloat(uint32_t u) {
        float x;
        memcpy(&x, &u, sizeof(float));
        return x;
    }

    // ULP, or "Unit in the Last Place", is the smallest difference between two consecutive representable float values at the magnitude of x
    float AddUlpMagnitude(float x, unsigned ulps) {
        return isfinite(x) ? BitsToFloat(FloatToBits(x) + ulps) : x;
    }

    float IntersectAxisMin(const RayData& rayData, int axis, float p) {
        return (p - rayData.origin[axis]) * rayData.invDir[axis];
    }

    float IntersectAxisMax(const RayData& rayData, int axis, float p) {
        return (p - rayData.origin[axis]) * rayData.paddedInvDir[axis];
    }

    float RobustMin(float x, float y) {
        return x < y ? x : y;
    }

    float RobustMax(float x, float y) {
        return x > y ? x : y;
    }

    bool IntersectNode(const RayData& rayData, const glm::vec3& aabbBoundsMin, const glm::vec3& aabbBoundsMax, float& t) {
        float bounds[6] = {
            aabbBoundsMin.x,
            aabbBoundsMax.x,
            aabbBoundsMin.y,
            aabbBoundsMax.y,
            aabbBoundsMin.z,
            aabbBoundsMax.z
        };

        float tmin_x = IntersectAxisMin(rayData, 0, bounds[0 + rayData.octant[0]]);
        float tmin_y = IntersectAxisMin(rayData, 1, bounds[2 + rayData.octant[1]]);
        float tmin_z = IntersectAxisMin(rayData, 2, bounds[4 + rayData.octant[2]]);
        float tmax_x = IntersectAxisMax(rayData, 0, bounds[0 + 1 - rayData.octant[0]]);
        float tmax_y = IntersectAxisMax(rayData, 1, bounds[2 + 1 - rayData.octant[1]]);
        float tmax_z = IntersectAxisMax(rayData, 2, bounds[4 + 1 - rayData.octant[2]]);

        float tmin = RobustMax(RobustMax(tmin_x, tmin_y), RobustMax(tmin_z, rayData.minDistance));
        float tmax = RobustMin(RobustMin(tmax_x, tmax_y), RobustMin(tmax_z, rayData.maxDistance));

        t = tmin;
        return tmin <= tmax;
    }
}
