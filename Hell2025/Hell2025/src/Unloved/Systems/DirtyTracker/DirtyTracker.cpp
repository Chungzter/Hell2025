#include "DirtyTracker.h"

#include "Hell/Common/Enum.h"
#include "Hell/Logging.h"
#include "Hell/Math/AABB.h"
#include "Hell/ResourceManagement/ResourceManager.h"

#include "Unloved/Common/Constants.h"
#include "Unloved/Debug/DebugDraw.h"
#include "Unloved/ObjectId.h"
#include "Unloved/Objects/House/Door.h"
#include "Unloved/Objects/Interior/Piano.h"
#include "Unloved/Objects/Lighting/Light.h"
#include "Unloved/Objects/Props/GenericObject.h"
#include "Unloved/Objects/Props/PickUp.h"
#include "Unloved/World/World.h"

namespace Unloved::DirtyTracker {

    void CalculateDirtyDoorAABBs();
    void DebugDrawLightShadowMapDirtyFlags();
    bool IntersectAABB(const RenderItem& renderItemA, const glm::vec3& boundsMinB, const glm::vec3& boundsMaxB);
    bool IntersectAABB(const glm::vec3& boundsMinA, const glm::vec3& boundsMaxA, const RenderItem& renderItemB);
    bool IntersectAABB(const glm::vec3& boundsMinA, const glm::vec3& boundsMaxA, const glm::vec3& boundsMinB, const glm::vec3& boundsMaxB);
    bool IntersectAABB(const glm::vec3& boundsMinA, const glm::vec3& boundsMaxA, const glm::vec4& boundsMinB, const glm::vec4& boundsMaxB);
    bool IntersectAABB(const glm::vec4& boundsMinA, const glm::vec4& boundsMaxA, const glm::vec3& boundsMinB, const glm::vec3& boundsMaxB);
    bool IntersectAABB(const glm::vec4& boundsMinA, const glm::vec4& boundsMaxA, const glm::vec4& boundsMinB, const glm::vec4& boundsMaxB);
    void PrintDirtyLightDebugMessage(const Light& light, uint64_t intersectingObjectId);
    void UpdateLightShadowMapDirtyFlag(Light& light);
    void UpdateLightRaytracingDirtyFlag(Light& light);

    std::vector<uint64_t> g_dirtyDoorIds;
    std::vector<uint64_t> g_dirtyLightIds;
    std::vector<DirtyBounds> g_dirtyBoundsSet;

    const std::vector<uint64_t>& GetDirtyDoorIds()  { return g_dirtyDoorIds; }
    const std::vector<uint64_t>& GetDirtyLightIds() { return g_dirtyLightIds; }

    std::vector<GPUAABB> g_dirtyDoorAABBs;
    bool g_printDebug = true;

    void BeginFrame() {
        g_dirtyDoorIds.clear();
        g_dirtyLightIds.clear();
        g_dirtyBoundsSet.clear();
    }

    void Update() {
        CalculateDirtyDoorAABBs();

        for (Light& light : Unloved::World::GetLights()) {
            if (light.ConsumeForcedDirtyFlag()) {
                light.SetShadowMapDirtyFlag(true);
                g_dirtyLightIds.push_back(light.GetObjectId());
            }
            else {
                light.SetShadowMapDirtyFlag(false);
            }

            UpdateLightRaytracingDirtyFlag(light);
        }

        // Doors
        for (const DirtyBounds& dirtyBounds : g_dirtyBoundsSet) {

            // Skip non doors
            if (Unloved::GetObjectIdType(dirtyBounds.objectId) != ObjectType::DOOR) {
                continue;
            }

            bool found = false;
    
            // Check whether the ID is already in there
            for (uint64_t dirtyId : g_dirtyDoorIds) {
                if (dirtyId == dirtyBounds.objectId) {
                    found = true;
                    break;
                }
            }

            // Add it if it isn't
            if (!found) {
                g_dirtyDoorIds.push_back(dirtyBounds.objectId);
            }
        }

        // Lights
        for (Light& light : World::GetLights()) {
            if (light.IsDirtyForShadowMaps()) {
                continue;
            }

            for (const DirtyBounds& dirtyBounds : g_dirtyBoundsSet) {

                // Skip any object that doesn't cast shadows
                if (!dirtyBounds.castShadows) {
                    continue;
                }
                
                // If intersection is found add the light ID and move onto checking next light
                if (IntersectAABB(light.GetWorldBoundsMin(), light.GetWorldBoundsMax(), dirtyBounds.boundsMin, dirtyBounds.boundsMax)) {
                    light.SetShadowMapDirtyFlag(true);
                    g_dirtyLightIds.push_back(light.GetObjectId());

                    // Temporary debug draw function
                    DebugDraw::DrawAABB(AABB(light.GetWorldBoundsMin(), light.GetWorldBoundsMax()), YELLOW);

                    break;
                }
            }
        }
    }

    void AddDirtyBounds(const DirtyBounds& dirtyBounds) {
        // Bail if invalid AABB was passed in
        if (dirtyBounds.boundsMin.x > dirtyBounds.boundsMax.x ||
            dirtyBounds.boundsMin.y > dirtyBounds.boundsMax.y ||
            dirtyBounds.boundsMin.z > dirtyBounds.boundsMax.z) {
            return;
        }

        g_dirtyBoundsSet.push_back(dirtyBounds);
    }

    const std::vector<GPUAABB>& GetDirtyDoorAABBs() {
        return g_dirtyDoorAABBs;
    }

    template<typename Object> 
    bool MarkLightAsDirtyIfObjectIsDirty(Light& light, Object& object) {
        // Bail early if whole object not dirty
        if (!object.IsDirty()) return false;

        // Otherwise check individual RenderItem AABBs
        for (const RenderItem& renderItem : object.GetRenderItems()) {
            if (IntersectAABB(renderItem, light.GetWorldBoundsMin(), light.GetWorldBoundsMax())) {
                light.SetShadowMapDirtyFlag(true);

                if (g_printDebug) {
                    std::string message;

                    message += "LIGHT\n";
                    message += " ID:        " + std::to_string(light.GetObjectId()) + "\n";
                    message += " Position:  " + Hell::String::FormatVec3(light.GetPosition()) + "\n";
                    message += " Radius:    " + std::to_string(light.GetRadius()) + "\n";
                    message += " ABBB min:  " + Hell::String::FormatVec3(light.GetWorldBoundsMin()) + "\n";
                    message += " ABBB max:  " + Hell::String::FormatVec3(light.GetWorldBoundsMax()) + "\n";
                    message += "\n";
                    message += " Triggered dirty by by " + Hell::Enum::ToString(Unloved::GetObjectIdType(object.GetObjectId())) + "\n";
                    message += " ID:        " + std::to_string(object.GetObjectId()) + "\n";
                    message += " ABBB min:  " + Hell::String::FormatVec3(renderItem.aabbMin) + "\n";
                    message += " ABBB max:  " + Hell::String::FormatVec3(renderItem.aabbMax) + "\n";
            
                    Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("AssetGeometry");
                    if (Mesh* mesh = meshBuffer.GetMeshById(renderItem.meshId)) {
                        message += " Mesh name: " + mesh->name + "\n";
                    }

                    message += "\n";

                    Logging::Debug() << message;
                }
                return true;
            }
        }
        return false;
    }

    void CalculateDirtyDoorAABBs() {
        g_dirtyDoorAABBs.clear();

        for (Door& door : Unloved::World::GetDoors()) {
            if (door.IsDirty()) {
                GPUAABB aabb;
                aabb.boundsMin = glm::vec4(door.GetPhsyicsAABB().GetBoundsMin(), 0.0f);
                aabb.boundsMax = glm::vec4(door.GetPhsyicsAABB().GetBoundsMax(), 0.0f);
                g_dirtyDoorAABBs.push_back(aabb);

                //DebugDraw::DrawAABB(door.GetPhsyicsAABB(), YELLOW);
            }
        }
    }

    void UpdateLightShadowMapDirtyFlag(Light& light) {
        // Was light forced dirty?
        if (light.ConsumeForcedDirtyFlag()) {
            light.SetShadowMapDirtyFlag(true);
            return;
        }

        // Otherwise start by setting the flag false
        light.SetShadowMapDirtyFlag(false);

        for (Door& object :          World::GetDoors())          if (MarkLightAsDirtyIfObjectIsDirty(light, object)) return;
        for (GenericObject& object : World::GetGenericObjects()) if (MarkLightAsDirtyIfObjectIsDirty(light, object)) return;
        for (Piano& object :         World::GetPianos())         if (MarkLightAsDirtyIfObjectIsDirty(light, object)) return;
        for (PickUp& object :        World::GetPickUps())        if (MarkLightAsDirtyIfObjectIsDirty(light, object)) return;
    }

    void UpdateLightRaytracingDirtyFlag(Light& light) {
        light.SetRaytracingDirtyFlag(false);

        // Bail if you are a fireplace light
        if (!light.GetCreateInfo().saveToFile) return; // <------------------ VERY HACKY

        for (const GPUAABB& gpuAabb : g_dirtyDoorAABBs) {
            AABB doorAABB(glm::vec3(gpuAabb.boundsMin), glm::vec3(gpuAabb.boundsMax));
            AABB lightCullingAABB(light.GetWorldBoundsMin(), light.GetWorldBoundsMax());

            if (doorAABB.IntersectsAABB(lightCullingAABB)) {
                light.SetRaytracingDirtyFlag(true);
                break;
            }
        }
    }

    void DebugDrawLightShadowMapDirtyFlags() {
        for (Light& light : Unloved::World::GetLights()) {
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
        Logging::Debug() << "LIGHT " << light.GetObjectId() << " triggered dirty by " << Hell::Enum::ToString(Unloved::GetObjectIdType(intersectingObjectId)) << " " << intersectingObjectId << "\n";
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
