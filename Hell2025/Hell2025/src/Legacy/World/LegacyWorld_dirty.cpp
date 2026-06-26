#include "LegacyWorld.h"

#include "Legacy/Debug/DebugDraw.h"
#include "Unloved/ObjectId.h"

#include "Hell/Logging.h"

namespace LegacyWorld {
    void DebugDrawLightShadowMapDirtyFlags();
    bool IntersectAABB(const RenderItem& renderItemA, const glm::vec3& boundsMinB, const glm::vec3& boundsMaxB);
    bool IntersectAABB(const glm::vec3& boundsMinA, const glm::vec3& boundsMaxA, const RenderItem& renderItemB);
    bool IntersectAABB(const glm::vec3& boundsMinA, const glm::vec3& boundsMaxA, const glm::vec3& boundsMinB, const glm::vec3& boundsMaxB);
    bool IntersectAABB(const glm::vec3& boundsMinA, const glm::vec3& boundsMaxA, const glm::vec4& boundsMinB, const glm::vec4& boundsMaxB);
    bool IntersectAABB(const glm::vec4& boundsMinA, const glm::vec4& boundsMaxA, const glm::vec3& boundsMinB, const glm::vec3& boundsMaxB);
    bool IntersectAABB(const glm::vec4& boundsMinA, const glm::vec4& boundsMaxA, const glm::vec4& boundsMinB, const glm::vec4& boundsMaxB);
    void PrintDirtyLightDebugMessage(const Light& light, uint64_t intersectingObjectId);
    void UpdateLightShadowMapDirtyFlag(Light& light);
    void UpdateLightsRaytracingDirtyFlag(Light& light);

    bool g_printDebug = true;

    void UpdateDirtyFlags() {

        for (Light& light : GetLights()) {
            UpdateLightShadowMapDirtyFlag(light);
            UpdateLightsRaytracingDirtyFlag(light);
        }

        //DebugDrawLightShadowMapDirtyFlags();
    }

    void UpdateLightShadowMapDirtyFlag(Light& light) {
        // Was light forced dirty?
        if (light.IsForcedDirty()) {
            light.SetShadowMapDirtyFlag(true);
            return;
        }

        // Otherwise start by setting the flag false
        light.SetShadowMapDirtyFlag(false);

        // Check doors
        for (Door& object : LegacyWorld::GetDoors()) {
            if (object.IsDirty()) {
                for (const RenderItem& renderItem : object.GetRenderItems()) {
                    if (IntersectAABB(renderItem, light.GetWorldBoundsMin(), light.GetWorldBoundsMax())) {
                        light.SetShadowMapDirtyFlag(true);
                        if (g_printDebug) PrintDirtyLightDebugMessage(light, object.GetObjectId());
                        return;
                    }
                }
            }
        }

        // Check generic objects
        for (GenericObject& object : LegacyWorld::GetGenericObjects()) {
            if (object.IsDirty()) {
                for (const RenderItem& renderItem : object.GetRenderItems()) {
                    if (IntersectAABB(renderItem, light.GetWorldBoundsMin(), light.GetWorldBoundsMax())) {
                        light.SetShadowMapDirtyFlag(true);
                        if (g_printDebug) PrintDirtyLightDebugMessage(light, object.GetObjectId());
                        return;
                    }
                }
            }
        }
        // Check pianos
        for (Piano& object : LegacyWorld::GetPianos()) {
            if (object.IsDirty()) {
                for (const RenderItem& renderItem : object.GetRenderItems()) {
                    if (IntersectAABB(renderItem, light.GetWorldBoundsMin(), light.GetWorldBoundsMax())) {
                        light.SetShadowMapDirtyFlag(true);
                        if (g_printDebug) PrintDirtyLightDebugMessage(light, object.GetObjectId());
                        return;
                    }
                }
            }
        }

        // Check pickups
        //for (PickUp& object : LegacyWorld::GetPickUps()) {
        //    if (object.IsDirty()) {
        //        for (const RenderItem& renderItem : object.GetRenderItems()) {
        //            if (IntersectAABB(renderItem, light.GetWorldBoundsMin(), light.GetWorldBoundsMax())) {
        //                light.SetShadowMapDirtyFlag(true);
        //                if (g_printDebug) PrintDirtyLightDebugMessage(light, object.GetObjectId());
        //                return;
        //            }
        //        }
        //    }
        //}
    }

    void UpdateLightsRaytracingDirtyFlag(Light& light) {

    }

    void DebugDrawLightShadowMapDirtyFlags() {
        for (Light& light : GetLights()) {
            AABB aabb(light.GetWorldBoundsMin(), light.GetWorldBoundsMax());

            if (light.IsDirtyForShadowMaps()) {
                DebugDraw::DrawAABB(aabb, RED);
            }
            else {
                DebugDraw::DrawAABB(aabb, GREEN);
            }
        }
    }

    void PrintDirtyLightDebugMessage(const Light& light, uint64_t intersectingObjectId) {
        Logging::Debug() << "LIGHT " << light.GetObjectId() << " triggered dirty by " << Util::EnumToString(Unloved::GetObjectIdType(intersectingObjectId)) << " " << intersectingObjectId << "\n";
    }

    bool IntersectAABB(const RenderItem& renderItemA, const glm::vec3& boundsMinB, const glm::vec3& boundsMaxB) {
        return IntersectAABB(renderItemA.aabbMin, renderItemA.aabbMax, boundsMinB, boundsMaxB);
    }

    bool IntersectAABB(const glm::vec3& boundsMinA, const glm::vec3& boundsMaxA, const RenderItem& renderItemB) {
        return IntersectAABB(boundsMinA, boundsMaxA, renderItemB.aabbMin, renderItemB.aabbMax);
    }

    bool IntersectAABB(const glm::vec3& boundsMinA, const glm::vec3& boundsMaxA, const glm::vec3& boundsMinB, const glm::vec3& boundsMaxB) {
        return (boundsMinA.x <= boundsMaxB.x && boundsMaxA.x >= boundsMinB.x) && (boundsMinA.y <= boundsMaxB.y && boundsMaxA.y >= boundsMinB.y) && (boundsMinA.z <= boundsMaxB.z && boundsMaxA.z >= boundsMinB.z);
    }

    bool IntersectAABB(const glm::vec3& boundsMinA, const glm::vec3& boundsMaxA, const glm::vec4& boundsMinB, const glm::vec4& boundsMaxB) {
        return (boundsMinA.x <= boundsMaxB.x && boundsMaxA.x >= boundsMinB.x) && (boundsMinA.y <= boundsMaxB.y && boundsMaxA.y >= boundsMinB.y) && (boundsMinA.z <= boundsMaxB.z && boundsMaxA.z >= boundsMinB.z);
    }    
    
    bool IntersectAABB(const glm::vec4& boundsMinA, const glm::vec4& boundsMaxA, const glm::vec3& boundsMinB, const glm::vec3& boundsMaxB) {
        return (boundsMinA.x <= boundsMaxB.x && boundsMaxA.x >= boundsMinB.x) && (boundsMinA.y <= boundsMaxB.y && boundsMaxA.y >= boundsMinB.y) && (boundsMinA.z <= boundsMaxB.z && boundsMaxA.z >= boundsMinB.z);
    }    
    
    bool IntersectAABB(const glm::vec4& boundsMinA, const glm::vec4& boundsMaxA, const glm::vec4& boundsMinB, const glm::vec4& boundsMaxB) {
        return (boundsMinA.x <= boundsMaxB.x && boundsMaxA.x >= boundsMinB.x) && (boundsMinA.y <= boundsMaxB.y && boundsMaxA.y >= boundsMinB.y) && (boundsMinA.z <= boundsMaxB.z && boundsMaxA.z >= boundsMinB.z);
    }

}
