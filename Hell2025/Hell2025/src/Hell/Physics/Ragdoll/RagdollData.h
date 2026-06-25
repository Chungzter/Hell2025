#pragma once
#include "RagdollTypes.h"
#include "Hell/Common.h"
#include "Hell/File/FileInfo.h"

#include <cstddef>
#include <string>

struct RdGeometryDescriptionComponent {
    float length{ 1.0f };
    float radius{ 1.0f };
    float radiusEnd{ 1.0f };
    RdGeometryType type{ RdGeometryType::kSphere };
    RdVector extents{ 1.0, 1.0, 1.0 };
    RdVector offset{ 0.0, 0.0, 0.0 };
    RdEulerRotation rotation{ 0.0, 0.0, 0.0 };
    bool capsuleLengthAlongY = true;
    unsigned short vertexLimit{ 255 };
    int quantisedCount{ 0 };
    bool checkZeroAreaTriangles{ false };
    bool cookInputGeometry{ true };
    enum RdConvexDecomposition convexDecomposition = RdConvexDecomposition::Off;
};

struct RdScaleComponent {
    RdVector value{ 1.0, 1.0, 1.0 };
    RdVector absolute{ 1.0, 1.0, 1.0 };
};

struct RagdollGroup {
    RdString name = UNDEFINED_STRING;
    RdString jsonId = UNDEFINED_STRING;

    RdBoolean enabled{ true };
    RdEnum inputType{ static_cast<RdEnum>(RdBehaviour::kDynamic) };
    RdMotion linearMotion{ RdMotion::RdMotionLocked };
    RdBoolean selfCollide{ false };

    RdScalar linearStiffness{ 1.0 };
    RdScalar linearDampingRatio{ 1.0 };
    RdScalar angularStiffness{ 1.0 };
    RdScalar angularDampingRatio{ 1.0 };

    std::vector<RdString> markerJsonIds;
    std::vector<RdString> markerNames;
};

struct RagdollMarker {
    // Transforms
    RdString name = UNDEFINED_STRING;
    RdString jsonId = UNDEFINED_STRING;
    RdMatrix inputMatrix{ RdIdentityInit };
    RdMatrix restMatrix{ RdIdentityInit };
    RdMatrix originMatrix{ RdIdentityInit };
    RdMatrix parentFrame{ RdIdentityInit };
    RdMatrix childFrame{ RdIdentityInit };
    RdVector rotatePivot{ 0.0, 0.0, 0.0 };

    RdGeometryDescriptionComponent geometryDescriptionComponent;
    RdScaleComponent scaleComponent;

    RdPoints convexMeshVertices{};
    RdUints convexMeshIndices{};

    // Limits (angular)
    RdEulerRotation limitRange{};

    // Contact/rigid properties
    RdScalar contactStiffness{ 1.0 };
    RdScalar contactDampingRatio{ 1.0 };
    RdBoolean contactAccelerationSpring{ true };
    RdCombineMode contactCombineMode{ RdCombineMode::kMultiply };
    RdCombineMode frictionCombineMode{ RdCombineMode::kMultiply };
    RdScalar thickness{ 0.02f };
    RdBoolean useSceneScaleForThickness{ false };
    RdScalar sleepThreshold{ 5e-6f };
    RdUint wakeCounter{ 0 };
    RdScalar mass{ 0.0 };

    // Drive UI bits mirrored on the marker
    RdBoolean driveSlerp{ false };
    RdEnum driveSpringType{ 0 }; // 0=force, 1=accel

    // Stiffness/damping UI
    RdScalar linearStiffness{ 1.0 };
    RdScalar linearDampingRatio{ 1.0 };
    RdScalar angularStiffness{ 1.0 };
    RdScalar angularDampingRatio{ 1.0 };
    RdScalar limitStiffness{ 1.0 };
    RdScalar limitDampingRatio{ 1.0 };
    RdMotion linearMotion{ RdMotion::RdMotionLocked };

    // Misc UI
    RdEnum inputType{ static_cast<RdEnum>(RdBehaviour::kInherit) };
    RdInteger collisionGroup{ 0 };
    RdBoolean useRootStiffness{ false };
    RdBoolean useLinearAngularStiffness{ false };
    RdColor color;
    RdString groupJsonId = UNDEFINED_STRING;
    RdString groupName = UNDEFINED_STRING;
    RdInteger groupIndex{ -1 };

    RdBoolean isKinematic = false;
    RdBoolean enableCCD = false;
    RdScalar densityCustom = 1.0f;
    RdScalar linearDamping = 0.0f;
    RdScalar angularDamping = 0.05f;
    RdScalar friction = 0.8;
    RdScalar restitution = 0.1;
    RdUint positionIterations{ 8 };
    RdUint velocityIterations{ 1 };

    // Resolved after group inheritance and solver UI multipliers are applied
    RdEnum resolvedInputType{ static_cast<RdEnum>(RdBehaviour::kDynamic) };
    RdMotion resolvedLinearMotion{ RdMotion::RdMotionLocked };
    RdInteger resolvedCollisionGroup{ 0 };
    RdBoolean resolvedSelfCollide{ false };
    RdScalar resolvedLinearStiffness{ 0.0 };
    RdScalar resolvedLinearDampingRatio{ 0.0 };
    RdScalar resolvedAngularStiffness{ 0.0 };
    RdScalar resolvedAngularDampingRatio{ 0.0 };
    RdScalar resolvedDriveLinearStiffness{ 0.0 };
    RdScalar resolvedDriveLinearDamping{ 0.0 };
    RdScalar resolvedDriveAngularStiffness{ 0.0 };
    RdScalar resolvedDriveAngularDamping{ 0.0 };

    // Animation
    std::string bonePath = UNDEFINED_STRING;
    std::string boneName = UNDEFINED_STRING;

    // ???
    float maxContactImpulse{ -1.0f };
    float maxDepenetrationVelocity{ -1.0f };
};

struct RagdollJoint {
    RdString name = UNDEFINED_STRING;
    RdString childJsonId = UNDEFINED_STRING;
    RdString parentJsonId = UNDEFINED_STRING;
    RdString childName = UNDEFINED_STRING;
    RdString parentName = UNDEFINED_STRING;
    RdString relativeJsonId = UNDEFINED_STRING;

    // Joint frames
    RdMatrix parentFrame{ RdIdentityInit };
    RdMatrix childFrame{ RdIdentityInit };

    // Angular limits
    RdEulerRotation limitRange;
    RdScalar limitStiffness = 0.0;
    RdScalar limitDampingRatio = 0.0;
    RdBoolean limitEnabled = true;
    RdBoolean limitAutoOrient = false;
    RdMotion linearMotion = RdMotion::RdMotionLocked;
    RdScalar limitLinearStiffness = 0.0;
    RdScalar limitLinearDamping = 0.0;
    RdScalar limitAngularStiffness = 0.0;
    RdScalar limitAngularDamping = 0.0;

    // Drive settings
    RdBoolean driveSlerp = false;
    RdEnum    driveSpringType = 0;
    RdScalar  driveLinearStiffness = 0.0;
    RdScalar  driveLinearDamping = 0.0;
    RdScalar  driveAngularStiffness = 0.0;
    RdScalar  driveAngularDamping = 0.0;
    RdVector  driveLinearAmount{ 0,0,0 };
    RdScalar  driveAngularAmountTwist = 0.0;
    RdScalar  driveAngularAmountSwing = 0.0;
    RdScalar  driveMaxLinearForce = -1.0;
    RdScalar  driveMaxAngularForce = -1.0;
    RdBoolean driveWorldspace = false;
    RdMatrix  driveTarget{ RdIdentityInit };
    RdBoolean ignoreMass = false;
    RdBoolean disableCollision = false;
    RdVectorF limitLinear = RdVectorF(0, 0, 0);
};

struct RagdollSolver {
    RdUint positionIterations { 1 };
    RdUint velocityIterations { 1 };
    RdInteger substeps;
    RdVector gravity;
    RdScalar sceneScale{ 1.0 };
    RdBoolean applySceneScaleToPhysics{ true };
    RdScalar linearLimitStiffness{ 1'000'000.0 };
    RdScalar linearLimitDamping{ 10'000.0 };
    RdScalar angularLimitStiffness{ 1'000'000.0 };
    RdScalar angularLimitDamping{ 10'000.0 };
    RdScalar linearDriveStiffness{ 10'000.0 };
    RdScalar linearDriveDamping{ 1'000.0 };
    RdScalar angularDriveStiffness{ 100'000.0 };
    RdScalar angularDriveDamping{ 10'000.0 };
    RdScalar contactStiffness{ 10'000.0 };
    RdScalar contactDamping{ 1'000.0 };
    RdScalar timeMultiplier{ 1.0 };
};

struct RagdollData {
    RagdollData() = default;
    RagdollData(const std::string& name);

    void SetFileInfo(const FileInfo& fileInfo) { m_fileInfo = fileInfo; }
    const std::string& GetName() const         { return m_name; }
    const FileInfo& GetFileInfo() const        { return m_fileInfo; }
    size_t GetCPUAllocatedByteCount() const;

    RagdollSolver m_solver;
    std::vector<RagdollGroup> m_groups;
    std::vector<RagdollMarker> m_markers;
    std::vector<RagdollJoint> m_joints;

    void PrintJointInfo();
    void PrintMarkerInfo();
    void PrintSolverInfo();

private:
    const RagdollMarker* GetMarkerByName(const RdString& name) const;

    std::string m_name = UNDEFINED_STRING;
    FileInfo m_fileInfo;
};
