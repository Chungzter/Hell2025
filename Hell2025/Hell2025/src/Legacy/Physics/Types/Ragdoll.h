#pragma once
#include "Types/Game/AnimatedGameObject.h"
#include <vector>
#include <string>

struct RigidComponent {
    int ID;
    std::string name;
    std::string shapeType;
    std::string correspondingJointName;
    float capsuleLength;
    float radius;
    float mass;
    float friction;
    float restitution;
    float linearDamping;
    float angularDamping;
    float sleepThreshold;
    glm::vec3 angularMass;
    glm::quat rotation;
    glm::mat4 restMatrix;
    glm::vec3 scaleAbsoluteVector;
    glm::vec3 boxExtents, offset;
};

struct JointComponent {
    std::string name;
    int parentID;
    int childID;
    glm::mat4 parentFrame;
    glm::mat4 childFrame;
    glm::mat4 absParentFrame;   // absolute
    glm::mat4 absChildFrame;    // absolute

    // Drive component
    float drive_angularDamping;
    float drive_angularStiffness;
    float drive_linearDampening;
    float drive_linearStiffness;
    bool drive_enabled;
    glm::mat4 target;

    // Limit component
    float twist;
    float swing1;
    float swing2;
    float limit_angularStiffness;
    float limit_angularDampening;
    float limit_linearStiffness;
    float limit_linearDampening;

    glm::vec3 limit;
    bool joint_enabled;
};

struct RagdollComponents {
    std::vector<RigidComponent> rigids;
    std::vector<JointComponent> joints;
    std::unordered_map<std::string, JointComponent> jointMap;
};

struct Ragdoll {
    Ragdoll() = default;

    void SetFilterData(PhysicsFilterData physicsFilterData);
    void SetPhysicsData(uint64_t objectId);
    void ActivatePhysics();
    void PrintCorrespondingBoneNames();
    void SetRigidGlobalPosesFromAnimatedGameObject(AnimatedGameObject* animatedGameObject);

    glm::mat4 GetRigidWorlTransform(const std::string& correspondingBoneName);

    std::string m_name = "";
    std::vector<std::string> m_correspondingBoneNames;
    std::vector<uint64_t> m_rigidDynamicIds;
    std::vector<uint64_t> m_d6JointIds;

    RagdollComponents m_components;
    void MarkForRemoval();

    bool m_markedForRemoval = false;

    bool IsMarkedForRemoval() { return m_markedForRemoval; }
};