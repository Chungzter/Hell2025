#include "Physics.h"
#include "Hell/Physics/Types/RagdollV1.h"
#include "File/JSON.h"
#include "Util.h"

#include <glm/trigonometric.hpp>
#include <unordered_map>

#include <iostream> // TODO: cleanup logging

namespace Hell::Physics {

    void LoadRagdollsFromDisk() {
        GetRagdollV1DataSet().clear();

        for (FileInfo& fileInfo : Util::IterateDirectory("res/ragdolls/v1/", { "rag" })) {
            RagdollV1Data& ragdollV1Data = GetRagdollV1DataSet()[fileInfo.name] = RagdollV1Data();

            nlohmann::json json;
            if (!JSON::LoadJsonFromFile(json, fileInfo.path)) {
                std::cerr << "LoadRagdollsFromDisk() failed to open file '" << fileInfo.path << "'\n";
                return;
            }

            for (auto& [entityName, entity] : json["entities"].items()) {
                const auto& comps = entity["components"];

                // Rigid components
                if (comps.contains("RigidComponent")) {
                    RigidComponent rigidComponent;
                    rigidComponent.ID = entity["id"].get<int>();
                    rigidComponent.name = comps["NameComponent"]["members"]["value"].get<std::string>();

                    rigidComponent.restMatrix = comps["RestComponent"]["members"]["matrix"]["values"].get<glm::mat4>();
                    rigidComponent.scaleAbsoluteVector = comps["ScaleComponent"]["members"]["absolute"]["values"].get<glm::vec3>();

                    auto& geom = comps["GeometryDescriptionComponent"]["members"];
                    rigidComponent.radius = geom["radius"].get<float>();
                    rigidComponent.capsuleLength = geom["length"].get<float>();
                    rigidComponent.shapeType = geom["type"].get<std::string>();
                    rigidComponent.boxExtents = geom["extents"]["values"].get<glm::vec3>();
                    rigidComponent.offset = geom["offset"]["values"].get<glm::vec3>();

                    rigidComponent.rotation = geom["rotation"]["values"].get<glm::quat>();

                    auto& rigidM = comps["RigidComponent"]["members"];
                    rigidComponent.mass = rigidM["mass"].get<float>();
                    rigidComponent.friction = rigidM["friction"].get<float>();
                    rigidComponent.restitution = rigidM["restitution"].get<float>();
                    rigidComponent.linearDamping = rigidM["linearDamping"].get<float>();
                    rigidComponent.angularDamping = rigidM["angularDamping"].get<float>();
                    rigidComponent.sleepThreshold = rigidM["sleepThreshold"].get<float>();
                    rigidComponent.angularMass = rigidM["angularMass"]["values"].get<glm::vec3>();

                    std::string src = comps["MarkerUIComponent"]["members"]["sourceTransform"].get<std::string>();
                    rigidComponent.correspondingJointName = src.substr(src.rfind("|") + 1);

                    if (rigidComponent.name != "rSceneShape" && rigidComponent.correspondingJointName != "rScene") {
                        ragdollV1Data.rigids.push_back(rigidComponent);
                    }
                }

                // � Joint components �
                if (comps.contains("JointComponent")) {

                    std::string rawName = comps["NameComponent"]["members"]["value"].get<std::string>();
                    bool isAbs = rawName.find("Absolute") != std::string::npos;
                    bool isRel = rawName.find("Relative") != std::string::npos;

                    std::string baseName = rawName;
                    if (isAbs) baseName.erase(baseName.find("Absolute"), 8);
                    if (isRel) baseName.erase(baseName.find("Relative"), 8);

                    JointComponent& jc = ragdollV1Data.jointMap[baseName];
                    jc.name = baseName;

                    auto& jm = comps["JointComponent"]["members"];
                    glm::mat4 pFrame = jm["parentFrame"]["values"].get<glm::mat4>();
                    glm::mat4 cFrame = jm["childFrame"]["values"].get<glm::mat4>();

                    jc.parentID = jm["parent"]["value"].get<int>();
                    jc.childID = jm["child"]["value"].get<int>();

                    if (isAbs) {
                        jc.absParentFrame = pFrame;
                        jc.absChildFrame = cFrame;
                    }
                    if (isRel) {
                        jc.parentFrame = pFrame;
                        jc.childFrame = cFrame;

                        // parse drive
                        auto& d = comps["DriveComponent"]["members"];
                        jc.drive_angularDamping = d["angularDamping"].get<float>();
                        jc.drive_angularStiffness = d["angularStiffness"].get<float>();
                        jc.drive_linearDampening = d["linearDamping"].get<float>();
                        jc.drive_linearStiffness = d["linearStiffness"].get<float>();
                        jc.drive_enabled = d["enabled"].get<bool>();
                        jc.target = d["target"]["values"].get<glm::mat4>();

                        // parse limit
                        auto& L = comps["LimitComponent"]["members"];
                        float twistDeg = L.contains("twist") ? L["twist"].get<float>() : 0.0f;
                        float swing1Deg = L["swing1"].get<float>();
                        float swing2Deg = L["swing2"].get<float>();

                        jc.twist = glm::radians(twistDeg);
                        jc.swing1 = glm::radians(swing1Deg);
                        jc.swing2 = glm::radians(swing2Deg);

                        jc.limit = glm::vec3(L["x"].get<float>(),
                                             L["y"].get<float>(),
                                             L["z"].get<float>());

                        jc.limit_linearStiffness = L["linearStiffness"].get<float>();
                        jc.limit_linearDampening = L["linearDamping"].get<float>();
                        jc.limit_angularStiffness = L["angularStiffness"].get<float>();
                        jc.limit_angularDampening = L["angularDamping"].get<float>();
                       //
                       //std::cout << "jc.limit_linearStiffness: " << jc.limit_linearStiffness << "\n";
                       //std::cout << "jc.limit_linearDampening: " << jc.limit_linearDampening << "\n";
                       //std::cout << "jc.limit_angularStiffness: " << jc.limit_angularStiffness << "\n";
                       //std::cout << "jc.limit_angularDampening: " << jc.limit_angularDampening << "\n";
                       //
                       //
                       //jc.limit_linearStiffness *= 0.00000001f;
                       //jc.limit_angularStiffness *= 0.00000001f;
                       //jc.limit_linearDampening  *= 0.1f;
                       //jc.limit_angularDampening *= 0.1f;

                        //jc.limit_linearStiffness = 500.0f;
                        //jc.limit_linearDampening = 50.0f;
                        //jc.limit_angularStiffness = 200.0f;
                        //jc.limit_angularDampening = 20.0f;

                        jc.limit_linearStiffness *= 0.001f;
                        jc.limit_angularStiffness *= 0.001f;
                        jc.limit_linearDampening *= 0.001f;
                        jc.limit_angularDampening *= 0.001f;

                        jc.limit_linearStiffness = 10000;
                        jc.limit_linearDampening = 1000000;
                        jc.drive_angularStiffness = 10000;
                        jc.drive_angularDamping = 1000000;

                        if (jc.parentID != 0) {
                            ragdollV1Data.joints.push_back(jc);
                        }
                    }
                }
            }

           // std::cout << "Loaded ragdoll: '" << fileInfo.name << ".rag'\n";
           // std::cout << " - rigid count: " << ragdollV1Data.rigids.size() << "\n";
           // std::cout << " - joint count: " << ragdollV1Data.joints.size() << "\n";
        }
    }

    /*void RemoveRagdoll(uint64_t ragdollId) {
        if (RagdollExists(ragdollId)) {
            PxPhysics* pxPhysics = Hell::Physics::GetPxPhysics();
            PxScene* pxScene = Hell::Physics::GetPxScene();
            RagdollV1& ragdoll = GetRagdolls()[ragdollId];

            for (uint64_t rigidDynamicId : ragdoll.m_rigidDynamicIds) {
                RigidDynamic* rigidDynamic = GetRigidDynamicById(rigidDynamicId);

                if (rigidDynamic) {
                    PxRigidDynamic* pxRigidDynamic = rigidDynamic->GetPxRigidDynamic();

                    if (pxRigidDynamic) {
                        // Clean up its user data
                        if (pxRigidDynamic->userData) {
                            delete static_cast<PhysicsUserData*>(pxRigidDynamic->userData);
                            pxRigidDynamic->userData = nullptr;
                        }
                        // Remove it from PxScene
                        if (pxRigidDynamic->getScene() != nullptr) {
                            pxScene->removeActor(*pxRigidDynamic);
                        }
                        // Release it
                        pxRigidDynamic->release();
                        pxRigidDynamic = nullptr;
                    }
                }
            }

            for (uint64_t d6JointId : ragdoll.m_d6JointIds) {
                D6Joint* d6Joint = GetD6JointById(d6JointId);

                if (d6Joint) {
                    PxD6Joint* pxD6Joint = d6Joint->GetPxD6Joint();

                    if (pxD6Joint) {
                        pxD6Joint->release();
                        pxD6Joint = nullptr;
                    }
                }
            }

            // Remove from container
            GetRagdolls().erase(ragdollId);
        }
    }*/

    std::vector<PxRigidDynamic*> GetRagdollPxRigidDynamics(uint64_t ragdollId) {
        std::vector<PxRigidDynamic*> result;

        if (RagdollExists(ragdollId)) {
            RagdollV1& ragdoll = GetRagdolls()[ragdollId];

            for (int i = 0; i < ragdoll.m_rigidDynamicIds.size(); i++) {
                RigidDynamic* rigidDynamic = Hell::Physics::GetRigidDynamicById(ragdoll.m_rigidDynamicIds[i]);
                if (rigidDynamic && rigidDynamic->GetPxRigidDynamic()) {
                    result.push_back(rigidDynamic->GetPxRigidDynamic());
                }
            }
        }
        return result;
    }

    std::vector<PxRigidActor*> GetRagdollPxRigidActors(uint64_t ragdollId) {
        std::vector<PxRigidActor*> result;

        if (RagdollExists(ragdollId)) {
            RagdollV1& ragdoll = GetRagdolls()[ragdollId];

            for (int i = 0; i < ragdoll.m_rigidDynamicIds.size(); i++) {
                RigidDynamic* rigidDynamic = Hell::Physics::GetRigidDynamicById(ragdoll.m_rigidDynamicIds[i]);
                if (rigidDynamic && rigidDynamic->GetPxRigidDynamic()) {
                    result.push_back(rigidDynamic->GetPxRigidDynamic());
                }
            }
        }
        return result;
    }

    void PrintSceneRagdollInfo() {

    }
}
