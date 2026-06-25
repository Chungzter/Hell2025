#pragma once
#include "Hell/Math/AABB.h"

#pragma warning(push, 0)
#include "PxPhysicsAPI.h"
#include "geometry/PxGeometryHelpers.h"
#include "Hell/Physics/Types/RigidDynamic.h"
#pragma warning(pop)

#include <glm/vec3.hpp>
#include <span>
#include <vector>

struct CollisionReport {
    PxActor* rigidA = NULL;
    PxActor* rigidB = NULL;
};

struct CharacterCollisionReport {
    PxController* characterController;
    PxShape* hitShape;
    PxRigidActor* hitActor;
    glm::vec3 hitNormal;
    glm::vec3 worldPosition;
};

class CCTHitCallback : public PxUserControllerHitReport {
public:
    void onShapeHit(const PxControllerShapeHit& hit);
    void onControllerHit(const PxControllersHit& /*hit*/);
    void onObstacleHit(const PxControllerObstacleHit& /*hit*/);
};

class ContactReportCallback : public PxSimulationEventCallback {
public:
    void onConstraintBreak(PxConstraintInfo* /*constraints*/, PxU32 /*count*/) {}
    void onWake(PxActor** /*actors*/, PxU32 /*count*/) {}
    void onSleep(PxActor** /*actors*/, PxU32 /*count*/) {}
    void onTrigger(PxTriggerPair* /*pairs*/, PxU32 /*count*/) {}
    void onAdvance(const PxRigidBody* const*, const PxTransform*, const PxU32) {}

    void onContact(const PxContactPairHeader& pairHeader, const PxContactPair* /*pairs*/, PxU32 /*nbPairs*/) {
        if (!pairHeader.actors[0] || !pairHeader.actors[1]) {
            return;
        }
        CollisionReport report;
        report.rigidA = pairHeader.actors[0];
        report.rigidB = pairHeader.actors[1];
        //Hell::Physics::AddCollisionReport(report);
    }
};
