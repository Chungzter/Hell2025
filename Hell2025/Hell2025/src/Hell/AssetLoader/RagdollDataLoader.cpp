#include "AssetLoader.h"

#include "Hell/File.h"
#include "Hell/Logging.h"
#include "Hell/Physics/Ragdoll/RagdollAPI.h"
#include "Hell/Physics/Ragdoll/RagdollData.h"
#include "Hell/ResourceManagement/ResourceManager.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <string>
#include <unordered_map>
#include <utility>

namespace {
inline RdEnum toInputType(std::string type) {
    return type == "Inherit" ? static_cast<short>(RdBehaviour::kInherit) : type == "Kinematic" ? static_cast<short>(RdBehaviour::kKinematic) : static_cast<short>(RdBehaviour::kDynamic);
}

inline RdInteger FindGroupIndexByJsonId(const RagdollData& ragdoll, const RdString& jsonId) {
    for (RdInteger i = 0; i < static_cast<RdInteger>(ragdoll.m_groups.size()); ++i) {
        if (ragdoll.m_groups[i].jsonId == jsonId) {
            return i;
        }
    }
    return -1;
}

inline RagdollMarker* FindMarkerByJsonId(RagdollData& ragdoll, const RdString& jsonId) {
    for (RagdollMarker& marker : ragdoll.m_markers) {
        if (marker.jsonId == jsonId) {
            return &marker;
        }
    }
    return nullptr;
}

inline bool IsBehaviour(RdEnum value, RdBehaviour behaviour) {
    return value == static_cast<RdEnum>(behaviour);
}

struct RagdollPd {
    RdScalar kp{ 0.0 };
    RdScalar kd{ 0.0 };
};

inline RagdollPd StandardStiffness(RdScalar stiffness, RdScalar dampingRatio) {
    return { stiffness, stiffness * dampingRatio };
}

inline std::string LastBone(const std::string& path) {
    const size_t p = path.find_last_of('|');
    return p == std::string::npos ? path : path.substr(p + 1);
}

inline void sanitizePath(std::string& s) {
    // Trim whitespace
    auto notspace = [](unsigned char c) { return !std::isspace(c); };
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), notspace));
    s.erase(std::find_if(s.rbegin(), s.rend(), notspace).base(), s.end());

    // Replace '/' with '|' and strip '\r'
    for (char& c : s) {
        if (c == '/') c = '|';
    }
    s.erase(std::remove(s.begin(), s.end(), '\r'), s.end());

    // Strip leading/trailing pipes
    while (!s.empty() && s.front() == '|') s.erase(s.begin());
    while (!s.empty() && s.back() == '|') s.pop_back();
}

inline std::string lastSegment(std::string s) {
    sanitizePath(s);
    if (s.empty()) return {};
    const auto pos = s.rfind('|');
    return (pos == std::string::npos) ? s : s.substr(pos + 1);
}

inline bool IsLegacyRagFile(const FileInfo& fileInfo) {
    std::string directory = fileInfo.dir;
    for (char& c : directory) {
        if (c == '\\') c = '/';
    }
    return directory.find("res/ragdolls/v1") != std::string::npos;
}

inline double InverseScaleOrOne(double value) {
    return value == 0.0 ? 1.0 : 1.0 / value;
}

inline RdPoint DescalePoint(const RdPoint& point, const RdVector& scale) {
    return RdPoint(
        point.x() * InverseScaleOrOne(scale.x()),
        point.y() * InverseScaleOrOne(scale.y()),
        point.z() * InverseScaleOrOne(scale.z())
    );
}

inline RdMatrix DescaleMatrix(const RdMatrix& matrix) {
    auto tm = matrix.transposed();

    RdVector column1{ tm[0][0], tm[1][0], tm[2][0] };
    RdVector column2{ tm[0][1], tm[1][1], tm[2][1] };

    RdVector newColumn1 = column1.normalized();
    RdVector newColumn2 = (column2 - newColumn1 * Magnum::Math::dot(newColumn1, column2)).normalized();
    RdVector newColumn3 = Magnum::Math::cross(newColumn1, newColumn2).normalized();

    RdMatrix transformed{
        { newColumn1, 0.0 },
        { newColumn2, 0.0 },
        { newColumn3, 0.0 },
        tm.row(3)
    };

    return transformed;
}

inline RdMatrix& SetTranslation(RdMatrix& matrix, const RdVector& translation) {
    matrix[3] = RdVector4(translation, 1.0);
    return matrix;
}

inline RdMatrix& PreTranslate(RdMatrix& matrix, const RdVector& translation) {
    matrix = matrix * RdMatrix::translation(translation);
    return matrix;
}
}

namespace Hell::AssetLoader {

    namespace {
        void LoadGroups(RagdollData& ragdoll, rapidjson::Document& doc);
        void LoadMarkers(RagdollData& ragdoll, rapidjson::Document& doc);
        void LoadJoints(RagdollData& ragdoll, rapidjson::Document& doc);
        void LoadSolver(RagdollData& ragdoll, rapidjson::Document& doc);
        void PostProcessRagdollData(RagdollData& ragdoll);

        RagdollData LoadRagdollData(const FileInfo& fileInfo) {
            std::ifstream file(fileInfo.path, std::ios::binary);
            if (!file) {
                Logging::Error() << fileInfo.path << " not found";
                return {};
            }

            std::string jsonString = std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());

            rapidjson::Document doc;
            doc.Parse(jsonString.c_str());

            RagdollData ragdollData(fileInfo.name);
            ragdollData.SetFileInfo(fileInfo);
            ragdollData.m_solver.applySceneScaleToPhysics = !IsLegacyRagFile(fileInfo);
            LoadSolver(ragdollData, doc);
            LoadGroups(ragdollData, doc);
            LoadMarkers(ragdollData, doc);
            LoadJoints(ragdollData, doc);
            PostProcessRagdollData(ragdollData);

            return ragdollData;
        }

        void LoadRagdollDataDirectory(const std::string& directory) {
            for (FileInfo& fileInfo : File::IterateDirectory(directory, { "rag" })) {
                RagdollData ragdollData = LoadRagdollData(fileInfo);

                if (ragdollData.GetName() == UNDEFINED_STRING) {
                    continue;
                }

                ResourceManager::CreateRagdollData(std::move(ragdollData));
                AddLoadLogItem("Loaded " + fileInfo.path);
            }
        }

    void LoadSolver(RagdollData& ragdoll, rapidjson::Document& doc) {
        RdJsonRegistry registry{ doc };

        for (const auto& m : doc["entities"].GetObject()) {
            RdString jentity = m.name.GetString();

            JsonComponent nameComponent = registry.get(jentity, "NameComponent");
            std::string name = nameComponent.getString("value");

            if (registry.has(jentity, "SolverUIComponent")) {

                RagdollSolver& solver = ragdoll.m_solver;

                auto SolverComponent = registry.get(jentity, "SolverComponent");
                solver.positionIterations = SolverComponent.getInteger("positionIterations");
                solver.velocityIterations = SolverComponent.getInteger("velocityIterations", solver.velocityIterations);
                solver.substeps = SolverComponent.getInteger("substeps");
                solver.gravity = SolverComponent.getVector("gravity");
                solver.timeMultiplier = SolverComponent.getFloat("timeMultiplier", solver.timeMultiplier);

                solver.sceneScale = { 1.0 };
                if (SolverComponent.has("sceneScale")) {
                    solver.sceneScale = SolverComponent.getFloat("sceneScale");
                }
                else {
                    solver.sceneScale = SolverComponent.getFloat("spaceMultiplier");
                }

                auto SolverUIComponent = registry.get(jentity, "SolverUIComponent");
                solver.linearLimitStiffness = SolverUIComponent.getFloat("linearLimitStiffness", solver.linearLimitStiffness);
                solver.linearLimitDamping = SolverUIComponent.getFloat("linearLimitDamping", solver.linearLimitDamping);
                solver.angularLimitStiffness = SolverUIComponent.getFloat("angularLimitStiffness", solver.angularLimitStiffness);
                solver.angularLimitDamping = SolverUIComponent.getFloat("angularLimitDamping", solver.angularLimitDamping);
                solver.linearDriveStiffness = SolverUIComponent.getFloat("linearDriveStiffness", solver.linearDriveStiffness);
                solver.linearDriveDamping = SolverUIComponent.getFloat("linearDriveDamping", solver.linearDriveDamping);
                solver.angularDriveStiffness = SolverUIComponent.getFloat("angularDriveStiffness", solver.angularDriveStiffness);
                solver.angularDriveDamping = SolverUIComponent.getFloat("angularDriveDamping", solver.angularDriveDamping);
                solver.contactStiffness = SolverUIComponent.getFloat("contactStiffness", solver.contactStiffness);
                solver.contactDamping = SolverUIComponent.getFloat("contactDamping", solver.contactDamping);
            }
        }
    }

    void LoadGroups(RagdollData& ragdoll, rapidjson::Document& doc) {
        RdJsonRegistry registry{ doc };

        for (const auto& m : doc["entities"].GetObject()) {
            RdString entity = m.name.GetString();

            if (!registry.has(entity, "GroupUIComponent")) continue;
            if (registry.has(entity, "SolverComponent")) continue;

            JsonComponent groupUI = registry.get(entity, "GroupUIComponent");
            JsonComponent nameComponent = registry.get(entity, "NameComponent");

            RagdollGroup& group = ragdoll.m_groups.emplace_back();
            group.name = nameComponent.getString("value");
            group.jsonId = entity;
            group.enabled = groupUI.getBoolean("enabled", true);
            group.inputType = toInputType(groupUI.getString("inputType", "Inherit"));
            group.linearMotion = StringToRdMotion(groupUI.getString("linearMotion", "Locked"));
            group.selfCollide = groupUI.getBoolean("selfCollide", false);
            group.linearStiffness = groupUI.getFloat("linearStiffness", groupUI.getFloat("stiffness", 1.0f));
            group.linearDampingRatio = groupUI.getFloat("linearDampingRatio", groupUI.getFloat("dampingRatio", 1.0f));
            group.angularStiffness = groupUI.getFloat("angularStiffness", groupUI.getFloat("stiffness", 1.0f));
            group.angularDampingRatio = groupUI.getFloat("angularDampingRatio", groupUI.getFloat("dampingRatio", 1.0f));
        }
    }

    void LoadMarkers(RagdollData& ragdoll, rapidjson::Document& doc) {
        RdJsonRegistry registry{ doc };

        for (const auto& m : doc["entities"].GetObject()) {
            RdString entity = m.name.GetString();

            // Pre 2022.01.01 the solver was a rigid body too (ground plane)
            if (registry.has(entity, "SolverComponent")) continue;
            if (!registry.has(entity, "MarkerUIComponent")) continue;

            JsonComponent color = registry.get(entity, "ColorComponent");
            JsonComponent convexMesh = registry.get(entity, "ConvexMeshComponents");
            JsonComponent geometryDescription = registry.get(entity, "GeometryDescriptionComponent");
            JsonComponent markerUI = registry.get(entity, "MarkerUIComponent");
            JsonComponent nameComponent = registry.get(entity, "NameComponent");
            JsonComponent restComponent = registry.get(entity, "RestComponent");
            JsonComponent rigid = registry.get(entity, "RigidComponent");
            JsonComponent scaleComponent = registry.get(entity, "ScaleComponent");
            JsonComponent sceneComponent = registry.get(entity, "SceneComponent");
            JsonComponent subs = registry.get(entity, "SubEntitiesComponent");

            RdString relEntity = subs.getEntity("relative");
            JsonComponent limit = registry.get(relEntity, "LimitComponent");
            JsonComponent drive = registry.get(relEntity, "DriveComponent");

            RdMatrix restMatrix = restComponent.getMatrix("matrix");
            RdVector scaleAbsolute = scaleComponent.getVector("absolute");
            RdVector scaleValue = scaleComponent.getVector("value");
            RdMatrix originMatrix{ RdIdentityInit };
            if (registry.has(entity, "OriginComponent")) {
                JsonComponent originComponent = registry.get(entity, "OriginComponent");
                originMatrix = originComponent.getMatrix("matrix");
            }
            else {
                // Otherwise it's safe to assume the rest matrix
                originMatrix = restMatrix;
            }

            bool isKinematic = false;
            bool enableCCD = false;
            float contactStiffness = 1.0f;
            float contactDampingRatio = 1.0f;
            bool contactAccelerationSpring = true;
            RdCombineMode contactCombineMode = RdCombineMode::kMultiply;
            RdCombineMode frictionCombineMode = RdCombineMode::kMultiply;
            float thickness = 0.02f;
            bool useSceneScaleForThickness = false;
            float sleepThreshold = 5e-6f;
            unsigned wakeCounter = 0;
            float friction = 0.8f;
            float restitution = 0.1f;
            float densityCustom = 1.0f;
            float linearDamping = 0.5f;
            float angularDamping = 1.0f;
            int positionIterations = 8;
            int velocityIterations = 1;
            float maxContactImpulse = -1.0f;
            float maxDepenetrationVelocity = -1.0f;

            if (rigid.has("kinematic"))         isKinematic = rigid.getBoolean("kinematic");
            if (rigid.has("enableCCD"))         enableCCD = rigid.getBoolean("enableCCD");
            if (rigid.has("linearDamping"))     linearDamping = rigid.getFloat("linearDamping");
            if (rigid.has("angularDamping"))    angularDamping = rigid.getFloat("angularDamping");
            if (rigid.has("contactStiffness"))  contactStiffness = rigid.getFloat("contactStiffness");
            if (rigid.has("contactDamping"))    contactDampingRatio = rigid.getFloat("contactDamping");
            if (rigid.has("contactDampingRatio")) contactDampingRatio = rigid.getFloat("contactDampingRatio");
            if (rigid.has("contactAccelerationSpring")) contactAccelerationSpring = rigid.getBoolean("contactAccelerationSpring");
            if (rigid.has("contactCombineMode")) contactCombineMode = StringToRdCombineMode(rigid.getString("contactCombineMode"));
            if (rigid.has("frictionCombineMode")) frictionCombineMode = StringToRdCombineMode(rigid.getString("frictionCombineMode"));
            if (rigid.has("thickness"))         thickness = rigid.getFloat("thickness");
            if (rigid.has("useSceneScaleForThickness")) useSceneScaleForThickness = rigid.getBoolean("useSceneScaleForThickness");
            if (rigid.has("sleepThreshold"))    sleepThreshold = rigid.getFloat("sleepThreshold");
            if (rigid.has("wakeCounter"))       wakeCounter = static_cast<unsigned>(std::max(0, rigid.getInteger("wakeCounter")));
            if (rigid.has("friction"))          friction = rigid.getFloat("friction");
            if (rigid.has("restitution"))       restitution = rigid.getFloat("restitution");
            if (rigid.has("densityCustom"))     densityCustom = rigid.getFloat("densityCustom");
            if (rigid.has("positionIterations")) positionIterations = rigid.getInteger("positionIterations");
            if (rigid.has("velocityIterations")) velocityIterations = rigid.getInteger("velocityIterations");
            if (rigid.has("maxContactImpulse")) maxContactImpulse = rigid.getFloat("maxContactImpulse");
            if (rigid.has("maxDepenetrationVelocity")) maxDepenetrationVelocity = rigid.getFloat("maxDepenetrationVelocity");

            RdGeometryDescriptionComponent geometryDescriptionComponent;
            geometryDescriptionComponent.type = StringToRdGeometryType(geometryDescription.getString("type"));
            geometryDescriptionComponent.extents = geometryDescription.getVector("extents");
            geometryDescriptionComponent.offset = geometryDescription.getVector("offset");
            geometryDescriptionComponent.rotation = RdToEulerRotation(geometryDescription.getQuaternion("rotation"));
            geometryDescriptionComponent.radius = geometryDescription.getFloat("radius");
            geometryDescriptionComponent.radiusEnd = geometryDescription.getFloat("radiusEnd", geometryDescriptionComponent.radius);
            geometryDescriptionComponent.length = geometryDescription.getFloat("length");
            geometryDescriptionComponent.capsuleLengthAlongY = geometryDescription.getBoolean("capsuleLengthAlongY", geometryDescriptionComponent.capsuleLengthAlongY);
            geometryDescriptionComponent.vertexLimit = static_cast<unsigned short>(std::clamp(
                geometryDescription.getInteger("vertexLimit", geometryDescriptionComponent.vertexLimit), 0, 255));
            geometryDescriptionComponent.quantisedCount = geometryDescription.getInteger(
                "quantisedCount",
                geometryDescription.getInteger("quantizedCount", geometryDescriptionComponent.quantisedCount));
            geometryDescriptionComponent.checkZeroAreaTriangles = geometryDescription.getBoolean(
                "checkZeroAreaTriangles", geometryDescriptionComponent.checkZeroAreaTriangles);
            geometryDescriptionComponent.cookInputGeometry = geometryDescription.getBoolean(
                "cookInputGeometry", geometryDescriptionComponent.cookInputGeometry);

            if (geometryDescription.has("convexDecomposition")) {
                geometryDescriptionComponent.convexDecomposition = StringToRdConvexDecomposition(geometryDescription.getString("convexDecomposition"));
            }

            if (isKinematic) {
                // Skip the ground marker
                continue;
            }

            RagdollMarker& marker = ragdoll.m_markers.emplace_back();
            marker.name = nameComponent.getString("value");
            marker.jsonId = entity;
            marker.inputMatrix = restMatrix * RdMatrix::scaling(scaleValue);
            marker.restMatrix = restMatrix;
            marker.originMatrix = originMatrix;
            marker.parentFrame = DescaleMatrix(markerUI.getMatrix("parentFrame"));
            marker.childFrame = DescaleMatrix(markerUI.getMatrix("childFrame"));
            marker.rotatePivot = markerUI.has("rotatePivot")
                ? scaleValue * markerUI.getVector("rotatePivot")
                : RdVector(0.0, 0.0, 0.0);
            marker.limitRange = { limit.getFloat("twist"), limit.getFloat("swing1"), limit.getFloat("swing2") };
            for (const RdPoint& vertex : convexMesh.getPoints("vertices")) {
                marker.convexMeshVertices.push_back(DescalePoint(vertex, scaleValue));
            }
            marker.convexMeshIndices = convexMesh.getUints("indices");
            marker.driveSlerp = drive.getBoolean("slerp");
            marker.driveSpringType = (int)drive.getBoolean("acceleration");
            marker.mass = markerUI.getFloat("mass");
            marker.linearStiffness = markerUI.getFloat("linearStiffness", marker.linearStiffness);
            marker.linearDampingRatio = markerUI.getFloat("linearDampingRatio", marker.linearDampingRatio);
            marker.angularStiffness = markerUI.getFloat("angularStiffness", marker.angularStiffness);
            marker.angularDampingRatio = markerUI.getFloat("angularDampingRatio", marker.angularDampingRatio);
            marker.limitStiffness = markerUI.getFloat("limitStiffness", marker.limitStiffness);
            marker.limitDampingRatio = markerUI.getFloat("limitDampingRatio", marker.limitDampingRatio);
            marker.linearMotion = StringToRdMotion(markerUI.getString("linearMotion", "Locked"));
            marker.inputType = toInputType(markerUI.getString("inputType", "Inherit"));
            marker.collisionGroup = markerUI.getInteger("collisionGroup", marker.collisionGroup);
            marker.useRootStiffness = markerUI.getBoolean("useRootStiffness", false);
            marker.useLinearAngularStiffness = markerUI.getBoolean("useLinearAngularStiffness");
            marker.color = color.getColor("value");
            if (registry.has(entity, "GroupComponent")) {
                JsonComponent groupComponent = registry.get(entity, "GroupComponent");
                marker.groupJsonId = groupComponent.getEntity("entity");
                marker.groupIndex = FindGroupIndexByJsonId(ragdoll, marker.groupJsonId);
                if (marker.groupIndex >= 0) {
                    RagdollGroup& group = ragdoll.m_groups[marker.groupIndex];
                    marker.groupName = group.name;
                    group.markerJsonIds.push_back(marker.jsonId);
                    group.markerNames.push_back(marker.name);
                }
            }
            marker.contactStiffness = contactStiffness;
            marker.contactDampingRatio = contactDampingRatio;
            marker.contactAccelerationSpring = contactAccelerationSpring;
            marker.contactCombineMode = contactCombineMode;
            marker.frictionCombineMode = frictionCombineMode;
            marker.thickness = std::max(0.0f, thickness);
            marker.useSceneScaleForThickness = useSceneScaleForThickness;
            marker.sleepThreshold = sleepThreshold;
            marker.wakeCounter = wakeCounter;
            marker.friction = friction;
            marker.restitution = restitution;
            marker.isKinematic = isKinematic;
            marker.enableCCD = enableCCD;
            marker.densityCustom = densityCustom;
            marker.linearDamping = linearDamping;
            marker.angularDamping = angularDamping;
            marker.positionIterations = std::clamp(positionIterations, 0, 255);
            marker.velocityIterations = std::clamp(velocityIterations, 0, 255);
            marker.maxContactImpulse = maxContactImpulse;
            marker.maxDepenetrationVelocity = maxDepenetrationVelocity;
            marker.geometryDescriptionComponent = geometryDescriptionComponent;

            marker.scaleComponent.absolute = scaleAbsolute;
            marker.scaleComponent.value = scaleValue;
            
            // Find corresponding bone name
            const std::vector<std::string> dst = markerUI.getStrings("destinationTransforms");
            for (const std::string& string : dst) {
                size_t pos = string.rfind("|");
                if (pos != std::string::npos) {
                    marker.boneName = string.substr(pos + 1);
                    break;
                }
            }
        }
    }

    void LoadJoints(RagdollData& ragdoll, rapidjson::Document& doc) {
        RdJsonRegistry registry{ doc };

        // Build a map of jsonEntity to marker name
        std::unordered_map<RdString, RdString> entityToMarkerName;
        for (const auto& m : doc["entities"].GetObject()) {
            RdString jentity = m.name.GetString();
            if (!registry.has(jentity, "MarkerUIComponent")) continue;
            auto Name = registry.get(jentity, "NameComponent");
            entityToMarkerName[jentity] = Name.getString("value");
        }

        // For each marker, pull its joint aka the "relative" subentity
        for (const auto& m : doc["entities"].GetObject()) {
            RdString childId = m.name.GetString();

            // Only markers participate
            if (!registry.has(childId, "MarkerUIComponent")) continue;

            // Find parent (new: ParentComponent, legacy: RigidComponent.parentRigid)
            RdString parentId = "-1";
            if (registry.has(childId, "ParentComponent")) {
                auto Parent = registry.get(childId, "ParentComponent");
                parentId = Parent.getEntity("entity");
            }
            else if (registry.has(childId, "RigidComponent")) {
                auto Rigid = registry.get(childId, "RigidComponent");
                parentId = Rigid.getEntity("parentRigid");
            }

            // Skip roots / invalid parents
            if (!doc["entities"].HasMember(parentId.c_str())) {
                continue;
            }

            // Joint sub entity that carries joint/limit/drive data
            if (!registry.has(childId, "SubEntitiesComponent")) {
                continue;
            }
            auto Subs = registry.get(childId, "SubEntitiesComponent");
            RdString jJoint = Subs.getEntity("relative");

            // Some very old scenes may be missing the joint sub entity
            if (!doc["entities"].HasMember(jJoint.c_str())) {
                continue;
            }

            // Components on the joint sub-entity
            auto Joint = registry.get(jJoint, "JointComponent");
            auto Limit = registry.get(jJoint, "LimitComponent");
            auto Drive = registry.get(jJoint, "DriveComponent");

            RdString childName = entityToMarkerName.count(childId) ? entityToMarkerName[childId] : childId;
            RdString parentName = entityToMarkerName.count(parentId) ? entityToMarkerName[parentId] : parentId;

            auto MarkerUI = registry.get(childId, "MarkerUIComponent");

            RagdollJoint& ragdollJoint = ragdoll.m_joints.emplace_back();
            ragdollJoint.name = childName + "_to_" + parentName;
            ragdollJoint.parentJsonId = parentId;
            ragdollJoint.childJsonId = childId;
            ragdollJoint.parentName = parentName;
            ragdollJoint.childName = childName;
            ragdollJoint.parentFrame = Joint.getMatrix("parentFrame");
            ragdollJoint.childFrame = Joint.getMatrix("childFrame");
            ragdollJoint.limitRange = { Limit.getFloat("twist"), Limit.getFloat("swing1"), Limit.getFloat("swing2") };
            ragdollJoint.limitEnabled = Limit.getBoolean("enabled", true);
            ragdollJoint.driveSlerp = Drive.getBoolean("slerp");
            ragdollJoint.driveSpringType = static_cast<int>(Drive.getBoolean("acceleration"));
            ragdollJoint.driveAngularAmountTwist = Drive.getFloat("angularAmountTwist", 0.0f);
            ragdollJoint.driveAngularAmountSwing = Drive.getFloat("angularAmountSwing", 0.0f);
            ragdollJoint.linearMotion = StringToRdMotion(MarkerUI.getString("linearMotion"));
            ragdollJoint.relativeJsonId = jJoint;
            ragdollJoint.driveLinearAmount = Drive.getVector("linearAmount");

            // Limit tuning, prefer LimitComponent, fallback to MarkerUI
            if (Limit.has("stiffness")) {
                ragdollJoint.limitStiffness = Limit.getFloat("stiffness");
                ragdollJoint.limitDampingRatio = Limit.getFloat("dampingRatio");
                ragdollJoint.limitAutoOrient = Limit.getBoolean("autoOrient", false);
            }
            else {
                ragdollJoint.limitStiffness = MarkerUI.getFloat("limitStiffness", 0.0f);
                ragdollJoint.limitDampingRatio = MarkerUI.getFloat("limitDampingRatio", 0.0f);
                ragdollJoint.limitAutoOrient = MarkerUI.getBoolean("limitAutoOrient", false);
            }

            // Drove
            ragdollJoint.driveLinearStiffness = Drive.getFloat("linearStiffness", 0.0f);
            ragdollJoint.driveLinearDamping = Drive.getFloat("linearDamping", 0.0f);
            ragdollJoint.driveAngularStiffness = Drive.getFloat("angularStiffness", 0.0f);
            ragdollJoint.driveAngularDamping = Drive.getFloat("angularDamping", 0.0f);

            // Per axis linear drive amount
            ragdollJoint.driveLinearAmount = Drive.getVector("linearAmount");

            // Drive caps and space
            ragdollJoint.driveMaxLinearForce = Drive.getFloat("maxLinearForce", -1.0f);    // -1 = infinite
            ragdollJoint.driveMaxAngularForce = Drive.getFloat("maxAngularForce", -1.0f);  // -1 = infinite
            ragdollJoint.driveWorldspace = Drive.getBoolean("worldspace", false);
            if (Drive.has("target")) {
                ragdollJoint.driveTarget = Drive.getMatrix("target");
            }
            ragdollJoint.ignoreMass = MarkerUI.getBoolean("ignoreMass", false);

            // Joint flags
            if (Joint.has("disableCollision")) {
                ragdollJoint.disableCollision = Joint.getBoolean("disableCollision");
            }
            if (Joint.has("ignoreMass")) {
                ragdollJoint.ignoreMass = Joint.getBoolean("ignoreMass");
            }

            // Linear limits
            float limitLinearX = -1;
            float limitLinearY = -1;
            float limitLinearZ = -1;
            if (Limit.has("x")) limitLinearX = Limit.getFloat("x");
            if (Limit.has("y")) limitLinearY = Limit.getFloat("y");
            if (Limit.has("z")) limitLinearZ = Limit.getFloat("z");
            ragdollJoint.limitLinear = RdVectorF(limitLinearX, limitLinearY, limitLinearZ);
        }
    }

    void PostProcessRagdollData(RagdollData& ragdoll) {
        const RagdollSolver& solver = ragdoll.m_solver;

        for (RagdollMarker& marker : ragdoll.m_markers) {
            RdEnum inputType = marker.inputType;
            RdMotion linearMotion = marker.linearMotion;
            RdScalar linearStiffness = marker.linearStiffness;
            RdScalar linearDampingRatio = marker.linearDampingRatio;
            RdScalar angularStiffness = marker.angularStiffness;
            RdScalar angularDampingRatio = marker.angularDampingRatio;

            if (linearMotion == RdMotion::RdMotionLocked) {
                linearStiffness = 0.0;
            }

            const bool hasEnabledGroup = marker.groupIndex >= 0 &&
                                         marker.groupIndex < static_cast<RdInteger>(ragdoll.m_groups.size()) &&
                                         ragdoll.m_groups[marker.groupIndex].enabled;
            if (hasEnabledGroup) {
                const RagdollGroup& group = ragdoll.m_groups[marker.groupIndex];

                if (linearMotion == RdMotion::RdMotionInherit) {
                    linearMotion = group.linearMotion;
                }

                linearDampingRatio *= group.linearDampingRatio;
                linearStiffness *= group.linearStiffness;
                angularStiffness *= group.angularStiffness;
                angularDampingRatio *= group.angularDampingRatio;

                if (IsBehaviour(marker.inputType, RdBehaviour::kInherit)) {
                    inputType = group.inputType;
                }

                marker.resolvedSelfCollide = group.selfCollide;
                if (marker.collisionGroup == -1) {
                    marker.resolvedCollisionGroup = 0;
                }
                else if (marker.collisionGroup == 0 && !group.selfCollide) {
                    marker.resolvedCollisionGroup = marker.groupIndex + 256;
                }
                else {
                    marker.resolvedCollisionGroup = marker.collisionGroup;
                }
            }
            else {
                inputType = IsBehaviour(marker.inputType, RdBehaviour::kKinematic)
                    ? static_cast<RdEnum>(RdBehaviour::kKinematic)
                    : static_cast<RdEnum>(RdBehaviour::kDynamic);
                marker.resolvedSelfCollide = false;
                marker.resolvedCollisionGroup = marker.collisionGroup;
            }

            if (IsBehaviour(inputType, RdBehaviour::kInherit)) {
                inputType = static_cast<RdEnum>(RdBehaviour::kDynamic);
            }
            if (linearMotion == RdMotion::RdMotionInherit) {
                linearMotion = RdMotion::RdMotionLocked;
            }

            if (!marker.useRootStiffness && linearMotion != RdMotion::RdMotionFree) {
                linearStiffness = 0.0;
                linearDampingRatio = 0.0;
            }

            const RagdollPd linearDrive = linearStiffness < 0.0
                ? RagdollPd{}
                : StandardStiffness(linearStiffness, linearDampingRatio);
            const RagdollPd angularDrive = angularStiffness < 0.0
                ? RagdollPd{}
                : StandardStiffness(angularStiffness, angularDampingRatio);

            marker.resolvedInputType = inputType;
            marker.resolvedLinearMotion = linearMotion;
            marker.resolvedLinearStiffness = linearStiffness;
            marker.resolvedLinearDampingRatio = linearDampingRatio;
            marker.resolvedAngularStiffness = angularStiffness;
            marker.resolvedAngularDampingRatio = angularDampingRatio;
            marker.resolvedDriveLinearStiffness = linearDrive.kp * solver.linearDriveStiffness;
            marker.resolvedDriveLinearDamping = linearDrive.kd * solver.linearDriveDamping;
            marker.resolvedDriveAngularStiffness = angularDrive.kp * solver.angularDriveStiffness;
            marker.resolvedDriveAngularDamping = angularDrive.kd * solver.angularDriveDamping;
            marker.isKinematic = IsBehaviour(inputType, RdBehaviour::kKinematic);
        }

        for (RagdollJoint& joint : ragdoll.m_joints) {
            RagdollMarker* child = FindMarkerByJsonId(ragdoll, joint.childJsonId);
            if (!child) {
                continue;
            }
            RagdollMarker* parent = FindMarkerByJsonId(ragdoll, joint.parentJsonId);
            if (parent) {
                RdMatrix rest = child->restMatrix;
                const RdMatrix parentRest = parent->restMatrix;
                RdVector pivot = child->rotatePivot;

                const RdVector& scale = child->scaleComponent.value;
                pivot.x() /= scale.x();
                pivot.y() /= scale.y();
                pivot.z() /= scale.z();

                PreTranslate(rest, pivot);

                RdMatrix parentFrame = DescaleMatrix(child->parentFrame);
                RdMatrix childFrame = DescaleMatrix(child->childFrame);
                RdMatrix parentPos{ parentRest.inverted() * rest };
                SetTranslation(parentFrame, parentPos.translation());
                SetTranslation(childFrame, pivot);

                joint.parentFrame = parentFrame;
                joint.childFrame = childFrame;
            }

            joint.linearMotion = child->resolvedLinearMotion;
            joint.limitLinear = child->resolvedLinearMotion == RdMotion::RdMotionFree
                ? RdVectorF(0.0f, 0.0f, 0.0f)
                : RdVectorF(-1.0f, -1.0f, -1.0f);
            joint.limitStiffness = child->limitStiffness;
            joint.limitDampingRatio = child->limitDampingRatio;
            joint.limitAngularStiffness = child->limitStiffness * solver.angularLimitStiffness;
            joint.limitAngularDamping = child->limitStiffness * child->limitDampingRatio * solver.angularLimitDamping;
            joint.limitLinearStiffness = child->limitStiffness * solver.linearLimitStiffness;
            joint.limitLinearDamping = child->limitStiffness * child->limitDampingRatio * solver.linearLimitDamping;
            joint.driveLinearStiffness = child->resolvedDriveLinearStiffness;
            joint.driveLinearDamping = child->resolvedDriveLinearDamping;
            joint.driveAngularStiffness = child->resolvedDriveAngularStiffness;
            joint.driveAngularDamping = child->resolvedDriveAngularDamping;
        }
    }
    }

    RagdollData LoadRagdollData(const std::string& path) {
        return LoadRagdollData(File::GetInfo(path));
    }

    void LoadRagdollDataFiles() {
        LoadRagdollDataDirectory("res/ragdolls/v1/");
        LoadRagdollDataDirectory("res/ragdolls/v2/");
    }
}
