#include "RagdollData.h"

#include "Hell/Common/String.h"
#include "Hell/Logging.h"
#include "Hell/Physics/Physics.h"

#include <iostream>

RagdollData::RagdollData(const std::string& name) {
    m_name = name;
}

size_t RagdollData::GetCPUAllocatedByteCount() const {
    size_t bytes = m_groups.capacity() * sizeof(RagdollGroup);
    bytes += m_markers.capacity() * sizeof(RagdollMarker);
    bytes += m_joints.capacity() * sizeof(RagdollJoint);

    for (const RagdollGroup& group : m_groups) {
        bytes += group.markerJsonIds.capacity() * sizeof(RdString);
        bytes += group.markerNames.capacity() * sizeof(RdString);
    }

    for (const RagdollMarker& marker : m_markers) {
        bytes += marker.convexMeshVertices.capacity() * sizeof(RdPoint);
        bytes += marker.convexMeshIndices.capacity() * sizeof(RdUint);
    }

    return bytes;
}

void RagdollData::PrintMarkerInfo() {
    for (RagdollMarker& marker : m_markers) {
        std::cout << "Name:                          " << marker.name << "\n";
        std::cout << "Bone path:                     " << marker.bonePath << "\n";
        std::cout << "Bone name:                     " << marker.boneName << "\n";

        std::cout << "RdGeometryDescriptionComponent\n";
        std::cout << " - Shape type:                 " << RdGeometryTypeToString(marker.geometryDescriptionComponent.type) << "\n";
        std::cout << " - Extents:                    " << marker.geometryDescriptionComponent.extents << "\n";
        std::cout << " - Shape rotation:             " << marker.geometryDescriptionComponent.rotation << "\n";
        std::cout << " - Shape offset:               " << marker.geometryDescriptionComponent.offset << "\n";
        std::cout << " - Shape radius:               " << marker.geometryDescriptionComponent.radius << "\n";
        std::cout << " - Shape length:               " << marker.geometryDescriptionComponent.length << "\n";
        std::cout << " - Convex decomposition:       " << RdConvexDecompositionTypeToString(marker.geometryDescriptionComponent.convexDecomposition) << "\n";

        std::cout << "Mass:                          " << marker.mass << "\n";
        std::cout << "Contact stiffness:             " << marker.contactStiffness << "\n";
        std::cout << "Contact damping ratio:         " << marker.contactDampingRatio << "\n";
        std::cout << "Drive slerp:                   " << Hell::String::FormatBool(marker.driveSlerp) << "\n";
        std::cout << "Drive spring type:             " << marker.driveSpringType << "\n";
        std::cout << "Linear stiffness:              " << marker.linearStiffness << "\n";
        std::cout << "Linear dampingRatio:           " << marker.linearDampingRatio << "\n";
        std::cout << "Angular stiffness:             " << marker.angularStiffness << "\n";
        std::cout << "Angular dampingRatio:          " << marker.angularDampingRatio << "\n";
        std::cout << "Input type:                    " << marker.inputType << "\n";
        std::cout << "Collision group:               " << marker.collisionGroup << "\n";
        std::cout << "Use root stiffness:            " << Hell::String::FormatBool(marker.useRootStiffness) << "\n";
        std::cout << "Use linear angular stiffness:  " << Hell::String::FormatBool(marker.useLinearAngularStiffness) << "\n";
        std::cout << "Limit range (twist,s1,s2):     " << marker.limitRange << "\n";
        std::cout << "Mesh vertex count:             " << static_cast<unsigned>(marker.convexMeshVertices.size()) << "\n";
        std::cout << "Mesh index count:              " << static_cast<unsigned>(marker.convexMeshIndices.size()) << "\n";
        std::cout << "linearDamping:                 " << marker.linearDamping << "\n";
        std::cout << "angularDamping:                " << marker.angularDamping << "\n";
        std::cout << "Is kinematic:                  " << Hell::String::FormatBool(marker.isKinematic) << "\n";
        std::cout << "EnableCCD                      " << Hell::String::FormatBool(marker.enableCCD) << "\n";
        std::cout << "Input matrix:\n" << marker.inputMatrix << "\n";
        std::cout << "Origin matrix:\n" << marker.originMatrix << "\n";
        std::cout << "Parent frame:\n" << marker.parentFrame << "\n";
        std::cout << "Child frame:\n" << marker.childFrame << "\n\n";
    }
}

void RagdollData::PrintJointInfo() {
    for (RagdollJoint& joint : m_joints) {
        std::cout << "Name:                          " << joint.name << "\n";
        std::cout << "Child name:                    " << joint.childName << "\n";
        std::cout << "Parent name:                   " << joint.parentName << "\n";
        std::cout << "Child (json id):               " << joint.childJsonId << "\n";
        std::cout << "Parent (json id):              " << joint.parentJsonId << "\n";
        std::cout << "Relative (json id):            " << joint.relativeJsonId << "\n";
        std::cout << "Limit linear:                  " << RdVector(joint.limitLinear) << "\n";
        std::cout << "Linear motion:                 " << RdMotionToString(joint.linearMotion) << "\n";
        std::cout << "Limit range (twist,s1,s2):     " << joint.limitRange << "\n";
        std::cout << "Limit stiffness:               " << joint.limitStiffness << "\n";
        std::cout << "Limit dampingRatio:            " << joint.limitDampingRatio << "\n";
        std::cout << "Limit auto-orient:             " << Hell::String::FormatBool(joint.limitAutoOrient) << "\n";
        std::cout << "Drive slerp:                   " << Hell::String::FormatBool(joint.driveSlerp) << "\n";
        std::cout << "Drive spring type:             " << joint.driveSpringType << "\n";
        std::cout << "Drive linear stiffness:        " << joint.driveLinearStiffness << "\n";
        std::cout << "Drive linear damping:          " << joint.driveLinearDamping << "\n";
        std::cout << "Drive angular stiffness:       " << joint.driveAngularStiffness << "\n";
        std::cout << "Drive angular damping:         " << joint.driveAngularDamping << "\n";
        std::cout << "Drive linear amount:           " << joint.driveLinearAmount << "\n";
        std::cout << "Drive angular amt (twist):     " << joint.driveAngularAmountTwist << "\n";
        std::cout << "Drive angular amt (swing):     " << joint.driveAngularAmountSwing << "\n";
        std::cout << "Drive max linear force:        " << joint.driveMaxLinearForce << "\n";
        std::cout << "Drive max angular force:       " << joint.driveMaxAngularForce << "\n";
        std::cout << "Drive worldspace:              " << Hell::String::FormatBool(joint.driveWorldspace) << "\n";
        std::cout << "Ignore mass:                   " << Hell::String::FormatBool(joint.ignoreMass) << "\n";
        std::cout << "Parent frame:\n" << joint.parentFrame << "\n";
        std::cout << "Child frame:\n" << joint.childFrame << "\n";
        std::cout << "Drive target:\n" << joint.driveTarget << "\n";
        std::cout << "\n";
    }
}

void RagdollData::PrintSolverInfo() {
    std::cout << "Ragdoll Solver\n";
    std::cout << "positionIterations: " << m_solver.positionIterations << "\n";
    std::cout << "gravity: " << m_solver.gravity << "\n";
    std::cout << "sceneScale: " << m_solver.sceneScale << "\n";
    std::cout << "linearLimitStiffness: " << m_solver.linearLimitStiffness << "\n";
    std::cout << "linearLimitDamping: " << m_solver.linearLimitDamping << "\n";
    std::cout << "angularLimitStiffness: " << m_solver.angularLimitStiffness << "\n";
    std::cout << "angularLimitDamping: " << m_solver.angularLimitDamping << "\n";
}

const RagdollMarker* RagdollData::GetMarkerByName(const RdString& name) const {
    for (const auto& m : m_markers)
        if (m.name == name) return &m;

    Logging::Error() << "Ragdoll::GetMarkerByName() failed to find " << name;
    return nullptr;
}
