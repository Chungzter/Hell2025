#include "Physics.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Util.h"

#include <cfloat>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/trigonometric.hpp>
#include <iostream> // TODO: cleanup logging
#include <unordered_map>
#include <vector>

namespace Hell::Physics {
    std::unordered_map<uint64_t, CharacterController> g_characterControllers;
    std::unordered_map<uint64_t, D6Joint> g_d6Joints;
    std::vector<HeightField> g_HeightFields;
    std::unordered_map<std::string, RagdollV1Data> g_ragdollV1DataSet;
    std::unordered_map<uint64_t, RagdollV1> g_ragdolls;
    std::unordered_map<uint64_t, RigidDynamic> g_rigidDynamics;
    std::vector<AABB> g_activeRigidDynamicAABBs;
    std::unordered_map<uint64_t, RigidStatic> g_rigidStatics;

    std::unordered_map<uint64_t, CharacterController>& GetCharacterControllers() { return g_characterControllers; }
    std::unordered_map<uint64_t, D6Joint>& GetD6Joints() { return g_d6Joints; }
    std::vector<HeightField>& GetHeightFields() { return g_HeightFields; }
    std::unordered_map<std::string, RagdollV1Data>& GetRagdollV1DataSet() { return g_ragdollV1DataSet; }
    std::unordered_map<uint64_t, RagdollV1>& GetRagdolls() { return g_ragdolls; }
    std::unordered_map<uint64_t, RigidDynamic>& GetRigidDynamics() { return g_rigidDynamics; }
    std::vector<AABB>& GetActiveRididDynamicAABBs() { return g_activeRigidDynamicAABBs; }
    std::unordered_map<uint64_t, RigidStatic>& GetRigidStatics() { return g_rigidStatics; }

    CharacterController* GetCharacterControllerById(uint64_t characterControllerId) {
        if (CharacterControllerExists(characterControllerId)) {
            return &GetCharacterControllers()[characterControllerId];
        }
        return nullptr;
    }

    void RemoveAnyCharacterControllerMarkedForRemoval() {
        for (auto it = GetCharacterControllers().begin(); it != GetCharacterControllers().end(); ) {
            CharacterController& characterController = it->second;
            if (characterController.IsMarkedForRemoval()) {
                PxController* pxController = characterController.GetPxController();
                pxController->release();
                pxController = nullptr;

                it = GetCharacterControllers().erase(it);
            }
            else {
                ++it;
            }
        }
    }

    void MarkCharacterControllerForRemoval(uint64_t characterControllerId) {
        if (CharacterControllerExists(characterControllerId)) {
            CharacterController& characterController = GetCharacterControllers()[characterControllerId];
            characterController.MarkForRemoval();
        }
    }

    bool CharacterControllerExists(uint64_t characterControllerId) {
        return GetCharacterControllers().find(characterControllerId) != GetCharacterControllers().end();
    }

    int GetCharacterControllerCount() {
        return GetCharacterControllers().size();
    }

    D6Joint* GetD6JointById(uint64_t d6JointId) {
        if (D6JointExists(d6JointId)) {
            return &GetD6Joints()[d6JointId];
        }
        return nullptr;
    }

    void RemoveAnyD6JointMarkedForRemoval() {
        for (auto it = GetD6Joints().begin(); it != GetD6Joints().end(); ) {
            D6Joint& d6Joint = it->second;
            if (d6Joint.IsMarkedForRemoval()) {
                PxD6Joint* pxD6Joint = d6Joint.GetPxD6Joint();
                pxD6Joint->release();
                pxD6Joint = nullptr;

                it = GetD6Joints().erase(it);
            }
            else {
                ++it;
            }
        }
    }

    void MarkD6JointForRemoval(uint64_t d6JointId) {
        if (D6JointExists(d6JointId)) {
            D6Joint& d6Joint = GetD6Joints()[d6JointId];
            d6Joint.MarkForRemoval();
        }
    }

    bool D6JointExists(uint64_t d6JointId) {
        return GetD6Joints().find(d6JointId) != GetD6Joints().end();
    }

    int GetD6JointCount() {
        return GetD6Joints().size();
    }

    void RemoveAnyHeightFieldMarkedForRemoval() {
        PxScene* pxScene = Hell::Physics::GetPxScene();

        for (int i = 0; i < GetHeightFields().size(); i++) {
            HeightField& heightField = GetHeightFields()[i];

            if (heightField.IsMarkedForRemoval()) {
                PxHeightField* pxHeightField = heightField.GetPxHeightField();
                PxRigidStatic* pxRigidStatic = heightField.GetPxRigidStatic();
                PxShape* pxShape = heightField.GetPxShape();

                if (pxRigidStatic && heightField.HasActivePhysics()) {
                    pxScene->removeActor(*pxRigidStatic);
                }

                if (pxShape) {
                    pxShape->release();
                    pxShape = nullptr;
                }

                if (pxHeightField) {
                    pxHeightField->release();
                    pxHeightField = nullptr;
                }

                if (pxRigidStatic) {
                    if (pxRigidStatic->userData) {
                        delete static_cast<PhysicsUserData*>(pxRigidStatic->userData);
                        pxRigidStatic->userData = nullptr;
                    }

                    pxRigidStatic->release();
                    pxRigidStatic = nullptr;
                }

                GetHeightFields().erase(GetHeightFields().begin() + i);
                i--;
            }
        }
    }

    void MarkAllHeightFieldsForRemoval() {
        for (HeightField& heightField : GetHeightFields()) {
            heightField.MarkForRemoval();
        }
    }

    int GetHeightFieldCount() {
        return (int)GetHeightFields().size();
    }

    RagdollV1* GetRagdollById(uint64_t ragdollId) {
        if (RagdollExists(ragdollId)) {
            return &GetRagdolls()[ragdollId];
        }
        return nullptr;
    }

    bool RagdollExists(uint64_t ragdollId) {
        return GetRagdolls().find(ragdollId) != GetRagdolls().end();
    }

    void MarkRagdollForRemoval(uint64_t ragdollId) {
        if (RagdollExists(ragdollId)) {
            RagdollV1& ragdoll = GetRagdolls()[ragdollId];
            ragdoll.MarkForRemoval();
        }
    }

    void RemoveAnyRagdollsMarkedForRemoval() {
        for (auto it = GetRagdolls().begin(); it != GetRagdolls().end(); ) {
            RagdollV1& ragdoll = it->second;

            if (ragdoll.IsMarkedForRemoval()) {
                for (uint64_t rigidDynamicId : ragdoll.m_rigidDynamicIds) {
                    Hell::Physics::MarkRigidDynamicForRemoval(rigidDynamicId);
                }
                for (uint64_t d6JointId : ragdoll.m_d6JointIds) {
                    Hell::Physics::MarkD6JointForRemoval(d6JointId);
                }

                it = GetRagdolls().erase(it);
            }
            else {
                ++it;
            }
        }
    }

    int GetRagdollCount() {
        return GetRagdolls().size();
    }

    bool RigidDynamicExists(uint64_t rigidDynamicId) {
        return GetRigidDynamics().find(rigidDynamicId) != GetRigidDynamics().end();
    }

    void MarkRigidDynamicForRemoval(uint64_t rigidDynamicId) {
        if (RigidDynamicExists(rigidDynamicId)) {
            RigidDynamic& rigidDynamic = GetRigidDynamics()[rigidDynamicId];
            rigidDynamic.MarkForRemoval();
        }
    }

    void RemoveAnyRigidDynamicMarkedForRemoval() {
        for (auto it = GetRigidDynamics().begin(); it != GetRigidDynamics().end(); ) {
            RigidDynamic& rigidDynamic = it->second;

            if (!rigidDynamic.IsMarkedForRemoval()) {
                ++it;
                continue;
            }

            PxRigidDynamic* pxRigidDynamic = rigidDynamic.GetPxRigidDynamic();

            if (pxRigidDynamic && pxRigidDynamic->getScene()) {
                pxRigidDynamic->getScene()->removeActor(*pxRigidDynamic);
            }

            for (PxShape* pxShape : rigidDynamic.GetPxShapes()) {
                if (pxShape) {
                    pxShape->release();
                }
            }

            if (pxRigidDynamic) {
                rigidDynamic.SetPxRigidDynamic(nullptr);
                pxRigidDynamic->release();
            }

            it = GetRigidDynamics().erase(it);
        }
    }

    RigidDynamic* GetRigidDynamicById(uint64_t rigidDynamicId) {
        if (RigidDynamicExists(rigidDynamicId)) {
            return &GetRigidDynamics()[rigidDynamicId];
        }
        return nullptr;
    }

    int GetRigidDynamicCount() {
        return GetRigidDynamics().size();
    }

    bool RigidStaticExists(uint64_t rigidStaticId) {
        return GetRigidStatics().find(rigidStaticId) != GetRigidStatics().end();
    }

    RigidStatic* GetRigidStaitcById(uint64_t rigidStaticId) {
        if (RigidStaticExists(rigidStaticId)) {
            return &GetRigidStatics()[rigidStaticId];
        }
        return nullptr;
    }

    void MarkRigidStaticForRemoval(uint64_t rigidStaticId) {
        if (RigidStaticExists(rigidStaticId)) {
            RigidStatic& rigidStatic = GetRigidStatics()[rigidStaticId];
            rigidStatic.MarkForRemoval();
        }
    }

    void RemoveRigidStatic(uint64_t rigidStaticId) {
        if (RigidStaticExists(rigidStaticId)) {
            PxScene* pxScene = Hell::Physics::GetPxScene();
            RigidStatic& rigidStatic = GetRigidStatics()[rigidStaticId];
            PxRigidStatic* pxRigidStatic = rigidStatic.GetPxRigidStatic();

            if (pxRigidStatic) {
                if (pxRigidStatic->userData) {
                    delete static_cast<PhysicsUserData*>(pxRigidStatic->userData);
                    pxRigidStatic->userData = nullptr;
                }
                if (pxRigidStatic->getScene() != nullptr) {
                    pxScene->removeActor(*pxRigidStatic);
                }
                pxRigidStatic->release();
                pxRigidStatic = nullptr;
            }

            std::vector<PxShape*>& pxShapes = rigidStatic.GetPxShapes();
            for (PxShape* pxShape : pxShapes) {
                if (pxShape) {
                    pxShape->release();
                }
            }

            GetRigidStatics().erase(rigidStaticId);
        }
    }

    void RemoveAnyRigidStaticMarkedForRemoval() {
        PxScene* pxScene = Hell::Physics::GetPxScene();

        for (auto it = GetRigidStatics().begin(); it != GetRigidStatics().end(); ) {
            RigidStatic& rigidStatic = it->second;
            if (rigidStatic.IsMarkedForRemoval()) {
                PxRigidStatic* pxRigidStatic = rigidStatic.GetPxRigidStatic();

                if (pxRigidStatic) {
                    if (pxRigidStatic->getScene() != nullptr) {
                        pxScene->removeActor(*pxRigidStatic);
                    }
                }

                std::vector<PxShape*>& pxShapes = rigidStatic.GetPxShapes();
                for (PxShape* pxShape : pxShapes) {
                    if (pxShape) {
                        pxShape->release();
                    }
                }

                if (pxRigidStatic) {
                    pxRigidStatic->release();
                }

                it = GetRigidStatics().erase(it);
            }
            else {
                ++it;
            }
        }
    }

    int GetRigidStaticCount() {
        return GetRigidStatics().size();
    }

    PxShape* CreateConvexShapeFromVertexList(std::span<Vertex>& vertices) {
        PxPhysics* pxPhysics = GetPxPhysics();
        PxMaterial* defaultMaterial = GetDefaultMaterial();

        std::vector<PxVec3> pxVertices;
        for (Vertex& vertex : vertices) {
            pxVertices.push_back(Hell::Physics::GlmVec3toPxVec3(vertex.position));
        }

        PxConvexMeshDesc convexDesc;
        convexDesc.points.count = pxVertices.size();
        convexDesc.points.stride = sizeof(PxVec3);
        convexDesc.points.data = pxVertices.data();
        convexDesc.flags = PxConvexFlag::eSHIFT_VERTICES | PxConvexFlag::eCOMPUTE_CONVEX;
        //  s
        PxTolerancesScale scale;
        PxCookingParams params(scale);

        PxDefaultMemoryOutputStream buf;
        PxConvexMeshCookingResult::Enum result;
        if (!PxCookConvexMesh(params, convexDesc, buf, &result)) {
            std::cout << "some convex mesh shit failed\n";
            return 0;
        }
        PxDefaultMemoryInputData input(buf.getData(), buf.getSize());
        PxConvexMesh* convexMesh = pxPhysics->createConvexMesh(input);
        PxConvexMeshGeometryFlags flags(~PxConvexMeshGeometryFlag::eTIGHT_BOUNDS);
        PxConvexMeshGeometry geometry(convexMesh, PxMeshScale(PxVec3(1.0f)), flags);

        PxShape* pxShape = pxPhysics->createShape(geometry, *defaultMaterial);
        return pxShape;
    }

    PxShape* CreateBoxShape(float width, float height, float depth, Transform shapeOffset, PxMaterial* material) {
        if (material == nullptr) {
            material = Hell::Physics::GetDefaultMaterial();
        }
        PxShape* shape = Hell::Physics::GetPxPhysics()->createShape(PxBoxGeometry(width, height, depth), *material, true);
        PxMat44 localShapeMatrix = GlmMat4ToPxMat44(shapeOffset.to_mat4());
        PxTransform localShapeTransform(localShapeMatrix);
        shape->setLocalPose(localShapeTransform);
        return shape;
    }

    PxRigidDynamic* CreateRigidDynamic(Transform worldTransform, PhysicsFilterData physicsFilterData, PxShape* shape, Transform shapeOffset) {
        PxQuat quat = GlmQuatToPxQuat(glm::quat(worldTransform.rotation));
        PxTransform pxTransform = PxTransform(PxVec3(worldTransform.position.x, worldTransform.position.y, worldTransform.position.z), quat);
        PxRigidDynamic* body = Hell::Physics::GetPxPhysics()->createRigidDynamic(pxTransform);

        // You are passing in a PxShape pointer and any shape offset will affects that actually object, wherever the fuck it is up the function chain.
        // Maybe look into this when you can be fucked, possibly you can just set the isExclusive bool to true, where and whenever the fuck that is and happens.
        PxFilterData filterData;
        filterData.word0 = (PxU32)physicsFilterData.raycastGroup;
        filterData.word1 = (PxU32)physicsFilterData.collisionGroup;
        filterData.word2 = (PxU32)physicsFilterData.collidesWith;
        shape->setQueryFilterData(filterData);       // ray casts
        shape->setSimulationFilterData(filterData);  // collisions
        PxMat44 localShapeMatrix = GlmMat4ToPxMat44(shapeOffset.to_mat4());
        PxTransform localShapeTransform(localShapeMatrix);
        shape->setLocalPose(localShapeTransform);

        body->attachShape(*shape);
        PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);
        Hell::Physics::GetPxScene()->addActor(*body);
        return body;
    }

    PxRigidDynamic* CreateRigidDynamic(PxShape* shape, glm::mat4 worldMatrix, glm::mat4 shapeOffsetMatrix, PhysicsFilterData filterData) {
        // Warning, you haven't vetted if this world matrix stuff is identical to the transform version above!!!
        // Warning, you haven't vetted if this world matrix stuff is identical to the transform version above!!!
        // Warning, you haven't vetted if this world matrix stuff is identical to the transform version above!!!

        PxTransform pxTransfrom = PxTransform(GlmMat4ToPxMat44(worldMatrix));
        PxRigidDynamic* body = Hell::Physics::GetPxPhysics()->createRigidDynamic(pxTransfrom);

        // You are passing in a PxShape pointer and any shape offset will affects that actually object, wherever the fuck it is up the function chain.
        // Maybe look into this when you can be fucked, possibly you can just set the isExclusive bool to true, where and whenever the fuck that is and happens.
        PxFilterData pxFilterData;
        pxFilterData.word0 = (PxU32)filterData.raycastGroup;
        pxFilterData.word1 = (PxU32)filterData.collisionGroup;
        pxFilterData.word2 = (PxU32)filterData.collidesWith;
        shape->setQueryFilterData(pxFilterData);       // ray casts
        shape->setSimulationFilterData(pxFilterData);  // collisions
        PxMat44 localShapeMatrix = GlmMat4ToPxMat44(shapeOffsetMatrix);
        PxTransform localShapeTransform(localShapeMatrix);
        shape->setLocalPose(localShapeTransform);

        body->attachShape(*shape);
        PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);
        Hell::Physics::GetPxScene()->addActor(*body);
        return body;
    }

    uint64_t CreateCharacterController(uint64_t parentObjectId, glm::vec3 position, float height, float radius, PhysicsFilterData physicsFilterData) {
        PxPhysics* pxPhysics = Hell::Physics::GetPxPhysics();

        PxMaterial* material = Hell::Physics::GetDefaultMaterial();
        PxCapsuleControllerDesc* desc = new PxCapsuleControllerDesc;
        desc->setToDefault();
        desc->height = height;
        desc->radius = radius;
        desc->position = PxExtendedVec3(position.x, position.y + (height / 2) + (radius * 2), position.z);
        desc->material = material;
        desc->stepOffset = 0.25f;
        desc->contactOffset = 0.001;
        desc->scaleCoeff = .99f;
        desc->reportCallback = &Hell::Physics::GetCharacterControllerHitCallback();
        desc->slopeLimit = cosf(glm::radians(85.0f));

        PxController* pxController = Hell::Physics::GetCharacterControllerManager()->createController(*desc);

        PxShape* shape;
        pxController->getActor()->getShapes(&shape, 1);

        PxFilterData filterData;
        filterData.word0 = (PxU32)physicsFilterData.raycastGroup;
        filterData.word1 = (PxU32)physicsFilterData.collisionGroup;
        filterData.word2 = (PxU32)physicsFilterData.collidesWith;

        // Create CharacterController
        uint64_t physicsID = CreatePhysicsId(PhysicsObjectType::CHARACTER_CONTROLLER);
        CharacterController& characterController = g_characterControllers[physicsID];

        PhysicsUserData physicsUserData;
        physicsUserData.objectId = parentObjectId;
        physicsUserData.physicsId = physicsID;
        physicsUserData.physicsType = PhysicsType::CHARACTER_CONTROLLER;
        pxController->getActor()->userData = new PhysicsUserData(physicsUserData);

        // Update its pointers
        characterController.SetPxController(pxController);

        return physicsID;
    }

    uint64_t CreateD6Joint(uint64_t parentRigidDynamicId, uint64_t childRigidDynamicId, glm::mat4 parentFrame, glm::mat4 childFrame) {
        PxPhysics* pxPhysics = Hell::Physics::GetPxPhysics();

        RigidDynamic* parentRigidDynamic = GetRigidDynamicById(parentRigidDynamicId);
        RigidDynamic* childRigidDynamic = GetRigidDynamicById(childRigidDynamicId);

        if (!parentRigidDynamic || !childRigidDynamic) {
            std::cout << "Hell::Physics::CreateD6Joint() failed to retrieve parent or child from rigid dynamic ids\n";
            return 0;
        }

        PxTransform pxParentFrame = PxTransform(Hell::Physics::GlmMat4ToPxMat44(parentFrame));
        PxTransform pxChildFrame = PxTransform(Hell::Physics::GlmMat4ToPxMat44(childFrame));

        PxD6Joint* pxD6joint = PxD6JointCreate(*pxPhysics, parentRigidDynamic->GetPxRigidDynamic(), pxParentFrame, childRigidDynamic->GetPxRigidDynamic(), pxChildFrame);
        
        // Do you really want this?
        pxD6joint->setConstraintFlag(PxConstraintFlag::eCOLLISION_ENABLED, false);
        pxD6joint->setConstraintFlag(PxConstraintFlag::eVISUALIZATION, false);

        // Create D6Joint
        uint64_t physicsID = CreatePhysicsId(PhysicsObjectType::D6_JOINT);
        D6Joint& d6Joint = g_d6Joints[physicsID];

        // Update its pointers
        d6Joint.SetPxD6Joint(pxD6joint);

        return physicsID;
    }

    void CreateHeightField(Hell::vecXZ& worldSpaceOffset, const float* heightValues) {
        HeightField& g_heightFields = g_HeightFields.emplace_back();
        g_heightFields.Create(worldSpaceOffset, heightValues);
    }

    inline PxTransform GlmMat4ToPxTransform(const glm::mat4& m) {
        glm::vec3 t = glm::vec3(m[3]);                  // column-major translation
        glm::quat q = glm::quat_cast(m);                // extract rotation
        return PxTransform(Hell::Physics::GlmVec3toPxVec3(t), Hell::Physics::GlmQuatToPxQuat(q));
    }

    uint64_t CreateRagdollByName(const std::string& name, float ragdollTotalWeight) {
        if (g_ragdollV1DataSet.find(name) == g_ragdollV1DataSet.end()) {
            std::cout << "Hell::Physics::CreateRagdollByName() failed: '" << name << "' not found\n";
            return 0;
        }

        uint64_t ragdollID = CreatePhysicsId(PhysicsObjectType::RAGDOLL_V1);

        RagdollV1& ragdoll = g_ragdolls[ragdollID];
        ragdoll.m_name = name;

        RagdollV1Data ragdollV1Data = g_ragdollV1DataSet[name];

        // Temp vector of RigidComponent Ids to lookup
        std::vector <uint32_t> rigidComponentIds;

        for (RigidComponent& rigidComponent : ragdollV1Data.rigids) {
            PxTransform shapeOffsetTranslation = PxTransform(PxVec3(rigidComponent.offset.x, rigidComponent.offset.y, rigidComponent.offset.z));
            PxQuat pxRotation = GlmQuatToPxQuat(rigidComponent.rotation);            
            PxTransform shapeOffsetRotation = PxTransform(pxRotation);
            glm::mat4 restMatrix = rigidComponent.restMatrix;

            glm::mat4 shapeOffsetMatrix = Hell::Physics::PxMat44ToGlmMat4(shapeOffsetTranslation.transform(shapeOffsetRotation));

            if (rigidComponent.shapeType == "Capsule") {
                rigidComponent.radius *= rigidComponent.scaleAbsoluteVector.x;
                rigidComponent.capsuleLength *= rigidComponent.scaleAbsoluteVector.y;
                float halfExtent = rigidComponent.capsuleLength * 0.5f;
                float radius = rigidComponent.radius;

                PxMaterial* material = Hell::Physics::GetDefaultMaterial();
                PxCapsuleGeometry geom = PxCapsuleGeometry(radius, halfExtent);
                PxShape* pxShape = Hell::Physics::GetPxPhysics()->createShape(geom, *material, true);

                uint64_t rigidDynmamicId = Hell::Physics::CreateRigidDynamicFromPxShape(pxShape, restMatrix, shapeOffsetMatrix);
                ragdoll.m_rigidDynamicIds.push_back(rigidDynmamicId);
                ragdoll.m_correspondingBoneNames.push_back(rigidComponent.correspondingJointName);

                rigidComponentIds.push_back(rigidComponent.ID);
            }
            else if (rigidComponent.shapeType == "Box") {
                rigidComponent.boxExtents.x *= rigidComponent.scaleAbsoluteVector.x;
                rigidComponent.boxExtents.y *= rigidComponent.scaleAbsoluteVector.y;
                rigidComponent.boxExtents.z *= rigidComponent.scaleAbsoluteVector.z;
                float halfExtent = rigidComponent.capsuleLength;
                float radius = rigidComponent.radius;

                PxMaterial* material = Hell::Physics::GetDefaultMaterial();
                PxBoxGeometry geom = PxBoxGeometry(rigidComponent.boxExtents.x * 0.5f, rigidComponent.boxExtents.y * 0.5f, rigidComponent.boxExtents.z * 0.5f);
                PxShape* pxShape = Hell::Physics::GetPxPhysics()->createShape(geom, *material, true);

                uint64_t rigidDynmamicId = Hell::Physics::CreateRigidDynamicFromPxShape(pxShape, restMatrix, shapeOffsetMatrix);
                ragdoll.m_rigidDynamicIds.push_back(rigidDynmamicId);
                ragdoll.m_correspondingBoneNames.push_back(rigidComponent.correspondingJointName);

                rigidComponentIds.push_back(rigidComponent.ID);
            }
            else if (rigidComponent.shapeType == "Sphere") {
                rigidComponent.radius *= rigidComponent.scaleAbsoluteVector.x;
                float radius = rigidComponent.radius;

                PxMaterial* material = Hell::Physics::GetDefaultMaterial();
                PxSphereGeometry geom = PxSphereGeometry(radius);
                PxShape* pxShape = Hell::Physics::GetPxPhysics()->createShape(geom, *material, true);

                uint64_t rigidDynmamicId = Hell::Physics::CreateRigidDynamicFromPxShape(pxShape, restMatrix, shapeOffsetMatrix);
                ragdoll.m_rigidDynamicIds.push_back(rigidDynmamicId);
                ragdoll.m_correspondingBoneNames.push_back(rigidComponent.correspondingJointName);

                rigidComponentIds.push_back(rigidComponent.ID);
            }
            else {
                std::cout << "RigidComponent '" << rigidComponent.name << "' had invalid shape\n";
                continue;
            }
        }

        // Compute the total volume of the ragdoll
        float totalVolume = 0.0f;
        for (uint64_t rigidDynamicId : ragdoll.m_rigidDynamicIds) {
            RigidDynamic* rigidDynamic = GetRigidDynamicById(rigidDynamicId);

            if (rigidDynamic) {
                totalVolume += rigidDynamic->GetVolume();
            }
        }

        // Compute the mass of each shape relative to the total weight of the ragdoll
        for (uint64_t rigidDynamicId : ragdoll.m_rigidDynamicIds) {
            RigidDynamic* rigidDynamic = GetRigidDynamicById(rigidDynamicId);

            if (rigidDynamic) {
                float volume = rigidDynamic->GetVolume();
                float mass = ragdollTotalWeight * (volume / totalVolume);
                float density = Util::GetDensity(mass, volume);
                rigidDynamic->UpdateMassAndInertia(density);
            }
        }

        // Does this do anything?
        for (uint64_t rigidDynamicId : ragdoll.m_rigidDynamicIds) {
            RigidDynamic* rigidDynamic = GetRigidDynamicById(rigidDynamicId);
            rigidDynamic->GetPxRigidDynamic()->setSolverIterationCounts(8, 1);
        }

        // Set some reasonable default filter data
        PhysicsFilterData filterData;
        filterData.raycastGroup = RaycastGroup::RAYCAST_ENABLED;
        filterData.collisionGroup = CollisionGroup::RAGDOLL_ENEMY;
        filterData.collidesWith = CollisionGroup(ENVIROMENT_OBSTACLE | CHARACTER_CONTROLLER);


        for (uint64_t rigidDynamicId : ragdoll.m_rigidDynamicIds) {
            RigidDynamic* rigidDynamic = GetRigidDynamicById(rigidDynamicId);
            if (rigidDynamic) {
                rigidDynamic->SetFilterData(filterData);
            }
        }

        PxScene* pxScene = Hell::Physics::GetPxScene();

        for (JointComponent& joint : ragdollV1Data.joints) {
            if (joint.parentID == 0) continue;

            uint64_t parentRigidDynamicId = 0, childRigidDynamicId = 0;
            for (int i = 0; i < rigidComponentIds.size(); i++) {
                if (rigidComponentIds[i] == joint.parentID)
                    parentRigidDynamicId = ragdoll.m_rigidDynamicIds[i];
                else if (rigidComponentIds[i] == joint.childID)
                    childRigidDynamicId = ragdoll.m_rigidDynamicIds[i];
            }
            auto* parentRigidDynamic = GetRigidDynamicById(parentRigidDynamicId);
            auto* childRigidDynamic = GetRigidDynamicById(childRigidDynamicId);
            if (!parentRigidDynamic || !childRigidDynamic) {
                std::cout << "�failed to retrieve parent or child\n";
                continue;
            }

            PxRigidDynamic* pPx = parentRigidDynamic->GetPxRigidDynamic();
            PxRigidDynamic* cPx = childRigidDynamic->GetPxRigidDynamic();

            PxTransform absP = GlmMat4ToPxTransform(joint.absParentFrame);
            PxTransform absC = GlmMat4ToPxTransform(joint.absChildFrame);

            PxTransform localP = pPx->getGlobalPose().transformInv(absP);
            PxTransform localC = cPx->getGlobalPose().transformInv(absC);

            glm::mat4 glmLocalP = Hell::Physics::PxMat44ToGlmMat4(localP);
            glm::mat4 glmLocalC = Hell::Physics::PxMat44ToGlmMat4(localC);

            uint64_t d6JointId = Hell::Physics::CreateD6Joint(
                parentRigidDynamicId,
                childRigidDynamicId,
                joint.parentFrame,
                joint.childFrame 
            );

            ragdoll.m_d6JointIds.push_back(d6JointId);


            D6Joint* d6Joint = Hell::Physics::GetD6JointById(d6JointId);
            PxD6Joint* pxD6Joint = d6Joint->GetPxD6Joint();
            if (!d6Joint) {
                std::cout << "Hell::Physics::CreateRagdollByName() failed to retrieve d6Joint by id " << d6JointId << "\n";
                continue;
            }

           //PxD6Joint* pxD6Joint = d6Joint->GetPxD6Joint();
           //if (!pxD6Joint) {
           //    std::cout << "Hell::Physics::CreateRagdollByName() failed because a pxD6Joint was nullptr\n";
           //    continue;
           //}

            // Linear spring
            //joint.limit_linearStiffness = 10000;
            //joint.limit_linearDampening = 1000000;
            //joint.drive_angularStiffness = 10000;
            //joint.drive_angularDamping = 1000000;


            const PxSpring linearSpring = PxSpring(joint.limit_linearStiffness, joint.limit_linearDampening);

            if (joint.limit.x > -1) {
                const PxJointLinearLimitPair limitX = PxJointLinearLimitPair(-joint.limit.x, joint.limit.x, linearSpring);
                pxD6Joint->setLinearLimit(PxD6Axis::eX, limitX);
            }

            if (joint.limit.y > -1) {
                const PxJointLinearLimitPair limitY = PxJointLinearLimitPair(-joint.limit.y, joint.limit.y, linearSpring);
                pxD6Joint->setLinearLimit(PxD6Axis::eY, limitY);
            }

            if (joint.limit.z > -1) {
                const PxJointLinearLimitPair limitZ = PxJointLinearLimitPair(-joint.limit.z, joint.limit.z, linearSpring);
                pxD6Joint->setLinearLimit(PxD6Axis::eZ, limitZ);
            }

            const PxSpring angularSpring = PxSpring(joint.drive_angularStiffness, joint.drive_angularDamping);
            const PxJointAngularLimitPair twistLimit = PxJointAngularLimitPair(-joint.twist, joint.twist, angularSpring);
            const PxJointLimitCone swingLimit = PxJointLimitCone(joint.swing1, joint.swing2, angularSpring);

            pxD6Joint->setTwistLimit(twistLimit);
            pxD6Joint->setSwingLimit(swingLimit);

           // pxD6Joint->setConstraintFlag(PxConstraintFlag::ePROJECTION, true);
           // pxD6Joint->setConstraintFlag(PxD6JointFlag::ePROJECTION, true);
           // pxD6Joint->setProjectionLinearTolerance(0.05f);
           // pxD6Joint->setProjectionAngularTolerance(glm::radians(0.5f));

            if (joint.limit.x > 0) pxD6Joint->setMotion(PxD6Axis::eX, PxD6Motion::eLIMITED);
            if (joint.limit.y > 0) pxD6Joint->setMotion(PxD6Axis::eY, PxD6Motion::eLIMITED);
            if (joint.limit.z > 0) pxD6Joint->setMotion(PxD6Axis::eZ, PxD6Motion::eLIMITED);

            if (joint.limit.x < 0) pxD6Joint->setMotion(PxD6Axis::eX, PxD6Motion::eLOCKED);
            if (joint.limit.y < 0) pxD6Joint->setMotion(PxD6Axis::eY, PxD6Motion::eLOCKED);
            if (joint.limit.z < 0) pxD6Joint->setMotion(PxD6Axis::eZ, PxD6Motion::eLOCKED);

            if (joint.twist > 0) pxD6Joint->setMotion(PxD6Axis::eTWIST, PxD6Motion::eLIMITED);
            if (joint.swing1 > 0) pxD6Joint->setMotion(PxD6Axis::eSWING1, PxD6Motion::eLIMITED);
            if (joint.swing2 > 0) pxD6Joint->setMotion(PxD6Axis::eSWING2, PxD6Motion::eLIMITED);

            if (joint.twist < 0) pxD6Joint->setMotion(PxD6Axis::eTWIST, PxD6Motion::eLOCKED);
            if (joint.swing1 < 0) pxD6Joint->setMotion(PxD6Axis::eSWING1, PxD6Motion::eLOCKED);
            if (joint.swing2 < 0) pxD6Joint->setMotion(PxD6Axis::eSWING2, PxD6Motion::eLOCKED);


            if (joint.drive_enabled) {
                float driveForceLimit = FLT_MAX;    // Maximum force; ignored
                bool isAcceleration = true;         // read from the json file some day when you aren't lazy

                // Temp hack because you don't think you need drives. Do you?
                if (false) {
                    PxD6JointDrive linearDrive = PxD6JointDrive(joint.limit_linearStiffness, joint.limit_linearDampening, driveForceLimit, isAcceleration);
                    pxD6Joint->setDrive(PxD6Drive::eX, linearDrive);
                    pxD6Joint->setDrive(PxD6Drive::eY, linearDrive);
                    pxD6Joint->setDrive(PxD6Drive::eZ, linearDrive);
                }

                // Temp hack because you don't think you need drives. Do you?
                if (false) {
                    PxD6JointDrive angularDrive = PxD6JointDrive(joint.drive_angularStiffness, joint.drive_angularDamping, driveForceLimit, isAcceleration);
                    pxD6Joint->setDrive(PxD6Drive::eTWIST, angularDrive);
                    pxD6Joint->setDrive(PxD6Drive::eSWING, angularDrive);
                    pxD6Joint->setDrive(PxD6Drive::eSLERP, angularDrive);
                }

                // TODO: Allow for changes to be made to both parent and child frames without affecting the drive target
                // Currently you cannot edit the child anchorpoint!!!

                // It's been too many years since you wrote the above comment, one day unravel what the fuck you're talking about there
            }

            else {
                // Temp hack because you don't think you need drives. Do you?
                if (false) {
                    const PxD6JointDrive defaultDrive{ 0.0f, 0.0f, 0.0f, false };
                    pxD6Joint->setDrive(PxD6Drive::eX, defaultDrive);
                    pxD6Joint->setDrive(PxD6Drive::eY, defaultDrive);
                    pxD6Joint->setDrive(PxD6Drive::eZ, defaultDrive);
                    pxD6Joint->setDrive(PxD6Drive::eTWIST, defaultDrive);
                    pxD6Joint->setDrive(PxD6Drive::eSWING, defaultDrive);
                    pxD6Joint->setDrive(PxD6Drive::eSLERP, defaultDrive);
                }
            }
        }

        ragdoll.m_components = ragdollV1Data;
        return ragdollID;
    }

    uint64_t CreateRigidDynamicWithCompoundConvexMeshesFromModel(const std::string& modelName, float mass, bool kinematic, PhysicsFilterData filterData) {
        Model* model = Hell::ResourceManager::GetModelByName(modelName);
        if (!model) return 0;

        PxPhysics* pxPhysics = Hell::Physics::GetPxPhysics();
        PxScene* pxScene = Hell::Physics::GetPxScene();
        PxMaterial* material = Hell::Physics::GetDefaultMaterial();

        PxFilterData pxFilterData;
        pxFilterData.word0 = (PxU32)filterData.raycastGroup;
        pxFilterData.word1 = (PxU32)filterData.collisionGroup;
        pxFilterData.word2 = (PxU32)filterData.collidesWith;

        // Create RigidDynamic
        uint64_t physicsID = CreatePhysicsId(PhysicsObjectType::RIGID_DYNAMIC);
        RigidDynamic& rigidDynamic = g_rigidDynamics[physicsID];

        // Create rigid dynamic
        PxTransform pxTransform = PxTransform(GlmMat4ToPxMat44(glm::mat4(1.0f)));
        PxRigidDynamic* pxRigidDynamic = pxPhysics->createRigidDynamic(pxTransform);
        pxRigidDynamic->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, kinematic);

        std::vector<PxShape*> pxShapes;
        float volume = 0.0f;
        Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("AssetGeometry");

        for (uint32_t meshId : model->GetMeshIndices()) {
            Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(meshId);
            if (!mesh) continue;

            std::span<Vertex> vertices(meshBuffer.GetVertices().data() + mesh->baseVertex, mesh->vertexCount);
            std::span<uint32_t> indices(meshBuffer.GetIndices().data() + mesh->baseIndex, mesh->indexCount);

            volume += Util::GetConvexHullVolume(vertices, indices);

            // Create convex shape
            std::vector<PxVec3> pxVertices;
            for (Vertex& vertex : vertices) {
                pxVertices.push_back(Hell::Physics::GlmVec3toPxVec3(vertex.position));
            }

            PxConvexMeshDesc convexDesc;
            convexDesc.points.count = pxVertices.size();
            convexDesc.points.stride = sizeof(PxVec3);
            convexDesc.points.data = pxVertices.data();
            convexDesc.flags = PxConvexFlag::eSHIFT_VERTICES | PxConvexFlag::eCOMPUTE_CONVEX;
            
            PxTolerancesScale scale;
            PxCookingParams params(scale);

            PxDefaultMemoryOutputStream buf;
            PxConvexMeshCookingResult::Enum result;
            if (!PxCookConvexMesh(params, convexDesc, buf, &result)) {
                std::cout << "some convex mesh shit failed\n";
                return 0;
            }

            PxDefaultMemoryInputData input(buf.getData(), buf.getSize());
            PxConvexMesh* convexMesh = pxPhysics->createConvexMesh(input);
            PxConvexMeshGeometryFlags flags(~PxConvexMeshGeometryFlag::eTIGHT_BOUNDS);
            PxConvexMeshGeometry geometry(convexMesh, PxMeshScale(PxVec3(1.0f)), flags);

            PxShape* pxShape = pxPhysics->createShape(geometry, *material);
            pxShape->setQueryFilterData(pxFilterData);       // ray casts
            pxShape->setSimulationFilterData(pxFilterData);  // collisions

            pxRigidDynamic->attachShape(*pxShape);
            pxShapes.push_back(pxShape);
        }

        float density = Util::GetDensity(mass, volume);
        PxRigidBodyExt::updateMassAndInertia(*pxRigidDynamic, density);

        pxScene->addActor(*pxRigidDynamic);

        // Update its pointers
        rigidDynamic.SetPxRigidDynamic(pxRigidDynamic);
        rigidDynamic.SetPxShapes(pxShapes);

        return physicsID;
    }

    uint64_t CreateRigidDynamicFromConvexMeshVertices(Transform transform, const std::span<Vertex>& vertices, const std::span<uint32_t>& indices, float mass, PhysicsFilterData filterData, glm::vec3 initialForce, glm::vec3 initialTorque) {
        PxPhysics* pxPhysics = Hell::Physics::GetPxPhysics();
        PxScene* pxScene = Hell::Physics::GetPxScene();
        PxMaterial* material = Hell::Physics::GetDefaultMaterial();

        float volume = Util::GetConvexHullVolume(vertices, indices);
        float density = Util::GetDensity(mass, volume);

        PxFilterData pxFilterData;
        pxFilterData.word0 = (PxU32)filterData.raycastGroup;
        pxFilterData.word1 = (PxU32)filterData.collisionGroup;
        pxFilterData.word2 = (PxU32)filterData.collidesWith;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        
        // Create convex shape
        std::vector<PxVec3> pxVertices;
        for (Vertex& vertex : vertices) {
            pxVertices.push_back(Hell::Physics::GlmVec3toPxVec3(vertex.position));
        }

        PxConvexMeshDesc convexDesc;
        convexDesc.points.count = pxVertices.size();
        convexDesc.points.stride = sizeof(PxVec3);
        convexDesc.points.data = pxVertices.data();
        convexDesc.flags = PxConvexFlag::eSHIFT_VERTICES | PxConvexFlag::eCOMPUTE_CONVEX;
        //  s
        PxTolerancesScale scale;
        PxCookingParams params(scale);

        PxDefaultMemoryOutputStream buf;
        PxConvexMeshCookingResult::Enum result;
        if (!PxCookConvexMesh(params, convexDesc, buf, &result)) {
            std::cout << "some convex mesh shit failed\n";
            return 0;
        }
        PxDefaultMemoryInputData input(buf.getData(), buf.getSize());
        PxConvexMesh* convexMesh = pxPhysics->createConvexMesh(input);
        PxConvexMeshGeometryFlags flags(~PxConvexMeshGeometryFlag::eTIGHT_BOUNDS);
        PxConvexMeshGeometry geometry(convexMesh, PxMeshScale(PxVec3(1.0f)), flags);

        PxShape* pxShape = pxPhysics->createShape(geometry, *material);
        pxShape->setQueryFilterData(pxFilterData);       // ray casts
        pxShape->setSimulationFilterData(pxFilterData);  // collisions

        ///////////////////////////////////////////////////////////////////////////////////////////////

        // Create rigid dynamic
        PxQuat quat = Hell::Physics::GlmQuatToPxQuat(glm::quat(transform.rotation));
        PxTransform pxTransform = PxTransform(PxVec3(transform.position.x, transform.position.y, transform.position.z), quat);
        PxRigidDynamic* pxRigidDynamic = pxPhysics->createRigidDynamic(pxTransform);
        pxRigidDynamic->attachShape(*pxShape);
        PxRigidBodyExt::updateMassAndInertia(*pxRigidDynamic, density);
        pxScene->addActor(*pxRigidDynamic);

        // Apply impulse
        PxVec3 force = PxVec3(initialForce.x, initialForce.y, initialForce.z);
        pxRigidDynamic->addForce(force, PxForceMode::eIMPULSE);

        // Apply torque
        PxVec3 torque = PxVec3(initialTorque.x, initialTorque.y, initialTorque.z);
        pxRigidDynamic->addTorque(torque);

        // Create RigidDynamic
        uint64_t physicsID = CreatePhysicsId(PhysicsObjectType::RIGID_DYNAMIC);
        RigidDynamic& rigidDynamic = g_rigidDynamics[physicsID];

        // Update its pointers
        rigidDynamic.SetPxRigidDynamic(pxRigidDynamic);
        rigidDynamic.SetPxShapes({ pxShape });

        return physicsID;
    }

    uint64_t CreateRigidDynamicFromBoxExtents(Transform transform, glm::vec3 boxExtents, float mass, PhysicsFilterData filterData, glm::vec3 initialForce, glm::vec3 initialTorque) {
        PxPhysics* pxPhysics = Hell::Physics::GetPxPhysics();
        PxScene* pxScene = Hell::Physics::GetPxScene();
        PxMaterial* material = Hell::Physics::GetDefaultMaterial();

        float halfWidth = boxExtents.x * 0.5f;
        float halfHeight = boxExtents.y * 0.5f;
        float halfDepth = boxExtents.z * 0.5f;
        float volume = Util::GetCubeVolume(halfWidth, halfHeight, halfDepth);
        float density = Util::GetDensity(mass, volume);

        PxFilterData pxFilterData;
        pxFilterData.word0 = (PxU32)filterData.raycastGroup;
        pxFilterData.word1 = (PxU32)filterData.collisionGroup;
        pxFilterData.word2 = (PxU32)filterData.collidesWith;

        // Create shape
        PxShape* pxShape = pxPhysics->createShape(PxBoxGeometry(halfWidth, halfHeight, halfDepth), *material, true);
        pxShape->setQueryFilterData(pxFilterData);       // ray casts
        pxShape->setSimulationFilterData(pxFilterData);  // collisions

        // Create rigid dynamic
        PxQuat quat = Hell::Physics::GlmQuatToPxQuat(glm::quat(transform.rotation));
        PxTransform pxTransform = PxTransform(PxVec3(transform.position.x, transform.position.y, transform.position.z), quat);
        PxRigidDynamic* pxRigidDynamic = pxPhysics->createRigidDynamic(pxTransform);
        pxRigidDynamic->attachShape(*pxShape);
        PxRigidBodyExt::updateMassAndInertia(*pxRigidDynamic, density);
        pxScene->addActor(*pxRigidDynamic);

        // Apply impulse
        PxVec3 force = PxVec3(initialForce.x, initialForce.y, initialForce.z);
        pxRigidDynamic->addForce(force, PxForceMode::eIMPULSE);

        // Apply torque
        PxVec3 torque = PxVec3(initialTorque.x, initialTorque.y, initialTorque.z);
        pxRigidDynamic->addTorque(torque);

        // Create DynamicBox
        uint64_t physicsID = CreatePhysicsId(PhysicsObjectType::RIGID_DYNAMIC);
        RigidDynamic& rigidDynamic = g_rigidDynamics[physicsID];

        // Update its pointers
        rigidDynamic.SetPxRigidDynamic(pxRigidDynamic);
        rigidDynamic.SetPxShapes({ pxShape });

        return physicsID;
    }

    uint64_t CreateRigidDynamicFromBoxExtents(const Transform& transform, const glm::vec3& boxExtents, bool kinematic, float mass, PhysicsFilterData filterData, const Transform& localOffset) {
        return CreateRigidDynamicFromBoxExtents(transform, boxExtents, kinematic, mass, filterData, localOffset.to_mat4());
    }

    uint64_t CreateRigidDynamicFromBoxExtents(const Transform& transform, const glm::vec3& boxExtents, bool kinematic, float mass, PhysicsFilterData filterData, const glm::mat4& localOffset) {
        return CreateRigidDynamicFromBoxExtents(transform.to_mat4(), boxExtents, kinematic, mass, filterData, localOffset);
    }

    uint64_t CreateRigidDynamicFromBoxExtents(const glm::mat4& transform, const glm::vec3& boxExtents, bool kinematic, float mass, PhysicsFilterData filterData, const Transform& localOffset) {
        return CreateRigidDynamicFromBoxExtents(transform, boxExtents, kinematic, mass, filterData, localOffset.to_mat4());
    }

    uint64_t CreateRigidDynamicFromBoxExtents(const glm::mat4& transform, const glm::vec3& boxExtents, bool kinematic, float mass, PhysicsFilterData filterData, const glm::mat4& localOffset) {
        PxPhysics* pxPhysics = Hell::Physics::GetPxPhysics();
        PxScene* pxScene = Hell::Physics::GetPxScene();
        PxMaterial* material = Hell::Physics::GetDefaultMaterial();

        float halfWidth = boxExtents.x * 0.5f;
        float halfHeight = boxExtents.y * 0.5f;
        float halfDepth = boxExtents.z * 0.5f;

        PxFilterData pxFilterData;
        pxFilterData.word0 = (PxU32)filterData.raycastGroup;
        pxFilterData.word1 = (PxU32)filterData.collisionGroup;
        pxFilterData.word2 = (PxU32)filterData.collidesWith;

        // Create shape
        PxShape* pxShape = pxPhysics->createShape(PxBoxGeometry(halfWidth, halfHeight, halfDepth), *material, true);
        pxShape->setQueryFilterData(pxFilterData);       // ray casts
        pxShape->setSimulationFilterData(pxFilterData);  // collisions
        pxShape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, true);
        pxShape->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, true);

        PxTransform localOffsetTransform = PxTransform(GlmMat4ToPxMat44(localOffset));
        pxShape->setLocalPose(localOffsetTransform);

        // Create rigid dynamic
        PxTransform pxTransform = PxTransform(Hell::Physics::GlmMat4ToPxMat44(transform));
        PxRigidDynamic* pxRigidDynamic = pxPhysics->createRigidDynamic(pxTransform);
        pxRigidDynamic->attachShape(*pxShape);
        pxScene->addActor(*pxRigidDynamic);

        // Kinematic flag
        pxRigidDynamic->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, kinematic);

        // Mass stuff
        float volume = Util::GetCubeVolume(halfWidth, halfHeight, halfDepth);
        float density = Util::GetDensity(mass, volume);
        PxRigidBodyExt::updateMassAndInertia(*pxRigidDynamic, density);

        // Create DynamicBox
        uint64_t physicsID = CreatePhysicsId(PhysicsObjectType::RIGID_DYNAMIC);
        RigidDynamic& rigidDynamic = g_rigidDynamics[physicsID];

        // Update its pointers
        rigidDynamic.SetPxRigidDynamic(pxRigidDynamic);
        rigidDynamic.SetPxShapes({ pxShape });

        return physicsID;
    }

    uint64_t CreateRigidDynamicFromPxShape(PxShape* pxShape, glm::mat4 initialPose, glm::mat4 shapeOffsetMatrix) {
        PxPhysics* pxPhysics = Hell::Physics::GetPxPhysics();
        PxScene* pxScene = Hell::Physics::GetPxScene();

        // Create rigid dynamic
        PxTransform pxTransform = PxTransform(GlmMat4ToPxMat44(initialPose));

        PxRigidDynamic* pxRigidDynamic = pxPhysics->createRigidDynamic(pxTransform);
        pxRigidDynamic->attachShape(*pxShape);
        pxScene->addActor(*pxRigidDynamic);

        // Create RigidDynamic
        uint64_t physicsID = CreatePhysicsId(PhysicsObjectType::RIGID_DYNAMIC);
        RigidDynamic& rigidDynamic = g_rigidDynamics[physicsID];

        PxMat44 localShapeMatrix = GlmMat4ToPxMat44(shapeOffsetMatrix);
        PxTransform localShapeTransform(localShapeMatrix);
        pxShape->setLocalPose(localShapeTransform);

        // Update its pointers
        rigidDynamic.SetPxRigidDynamic(pxRigidDynamic);
        rigidDynamic.SetPxShapes({ pxShape });
/*
        std::cout << " - created " << Hell::Physics::GetPxShapeTypeAsString(pxShape) << "\n";

        if (pxShape == nullptr) {
            std::cerr << "ERROR: PxPhysics::createShape returned NULL!" << std::endl;
            // The shape was not created by PhysX, possibly due to invalid geometry params
            // or other issues that didn't trigger the error callback but resulted in failure.
        }


        if (pxShape) {
            physx::PxTransform localPose = pxShape->getLocalPose();
            if (!localPose.isValid()) { // Checks for finite q and p, and normalized q
                std::cerr << "WARNING: Shape " << pxShape << " has an invalid local pose!" << std::endl;
                std::cerr << "  Pose P: (" << localPose.p.x << "," << localPose.p.y << "," << localPose.p.z << ")" << std::endl;
                std::cerr << "  Pose Q: (" << localPose.q.x << "," << localPose.q.y << "," << localPose.q.z << "," << localPose.q.w << ")" << std::endl;
            }
        }
        physx::PxRigidActor* actor = nullptr;
        if (pxShape) {
            actor = pxShape->getActor();
            if (actor == nullptr) {
                std::cout << "INFO: Shape " << pxShape << " is not attached to any PxRigidActor." << std::endl;
                // This means it cannot be in a scene.
                // This might happen if you create a shape but haven't attached it to an actor yet,
                // or if the actor it was attached to has been released and the shape detached.
            }
        }
        else {
         // Shape itself is null, so it can't be in a scene.
        }
        if (actor) {
            physx::PxScene* scenesActorIsIn = actor->getScene();
            if (scenesActorIsIn != nullptr) {
                // The actor (and thus its shape) is in a scene.
                // You can compare it to your expected scene instance:
                if (scenesActorIsIn == pxScene) { // gMyExpectedPxScene is your PxScene*
                    std::cout << "INFO: Shape " << pxShape << " (actor " << actor << ") IS in the expected scene." << std::endl;
                }
                else {
                    std::cerr << "WARNING: Shape " << pxShape << " (actor " << actor << ") IS in a scene, but NOT the expected one. Scene ptr: " << scenesActorIsIn << std::endl;
                }
            }
            else {
                std::cout << "INFO: Shape " << pxShape << "'s actor (" << actor << ") is NOT currently in any PxScene." << std::endl;
                // This means pxScene->addActor(*actor) was either not called, failed implicitly,
                // or pxScene->removeActor(*actor) was called.
            }
        }

        if (pxShape) { // Ensure pxShape is not null first
            physx::PxGeometryHolder geomHolder = pxShape->getGeometry(); // Get the geometry holder
            physx::PxGeometryType::Enum geomType = geomHolder.getType(); // Get the actual type

            // You can specifically exclude types you don't want to check, like eHEIGHTFIELD
            if (geomType == physx::PxGeometryType::eHEIGHTFIELD) {
                // std::cout << "INFO: Shape " << pxShape << " is a HeightField, skipping detailed geometry param check." << std::endl;
            }
            else {
             // Proceed with checks for other types
                switch (geomType) {
                    case physx::PxGeometryType::eSPHERE: {
                        const physx::PxSphereGeometry& sphereGeom = geomHolder.sphere(); // Access the sphere geometry
                        if (sphereGeom.radius <= 0.0f || !physx::PxIsFinite(sphereGeom.radius)) {
                            std::cerr << "WARNING: Shape " << pxShape << " (Sphere) has invalid radius: " << sphereGeom.radius << std::endl;
                        }
                        break;
                    }
                    case physx::PxGeometryType::eCAPSULE: {
                        const physx::PxCapsuleGeometry& capsuleGeom = geomHolder.capsule(); // Access the capsule geometry
                        if (capsuleGeom.radius <= 0.0f || !physx::PxIsFinite(capsuleGeom.radius) ||
                            capsuleGeom.halfHeight <= 0.0f || !physx::PxIsFinite(capsuleGeom.halfHeight)) {
                            std::cerr << "WARNING: Shape " << pxShape << " (Capsule) has invalid dimensions. Radius: " << capsuleGeom.radius << ", HalfHeight: " << capsuleGeom.halfHeight << std::endl;
                        }
                        break;
                    }
                    case physx::PxGeometryType::eBOX: {
                        const physx::PxBoxGeometry& boxGeom = geomHolder.box(); // Access the box geometry
                        if (boxGeom.halfExtents.x <= 0.0f || !physx::PxIsFinite(boxGeom.halfExtents.x) ||
                            boxGeom.halfExtents.y <= 0.0f || !physx::PxIsFinite(boxGeom.halfExtents.y) ||
                            boxGeom.halfExtents.z <= 0.0f || !physx::PxIsFinite(boxGeom.halfExtents.z)) {
                            std::cerr << "WARNING: Shape " << pxShape << " (Box) has invalid extents: (" << boxGeom.halfExtents.x << ", " << boxGeom.halfExtents.y << ", " << boxGeom.halfExtents.z << ")" << std::endl;
                        }
                        break;
                    }
                    case physx::PxGeometryType::eCONVEXMESH:
                        // For PxConvexMeshGeometry, you might check if its PxConvexMesh* is null
                        // or if its scale is valid. The mesh itself is created separately.
                        // const physx::PxConvexMeshGeometry& convexGeom = geomHolder.convexMesh();
                        // if (convexGeom.convexMesh == nullptr) std::cerr << "WARNING: Shape " << pxShape << " (ConvexMesh) has null mesh pointer." << std::endl;
                        // if (!convexGeom.scale.isValid() || !convexGeom.scale.isFinite())  std::cerr << "WARNING: Shape " << pxShape << " (ConvexMesh) has invalid scale." << std::endl;
                    break;
                    case physx::PxGeometryType::eTRIANGLEMESH:
                        // Similar checks for PxTriangleMeshGeometry
                        // const physx::PxTriangleMeshGeometry& triGeom = geomHolder.triangleMesh();
                        // if (triGeom.triangleMesh == nullptr) std::cerr << "WARNING: Shape " << pxShape << " (TriangleMesh) has null mesh pointer." << std::endl;
                        // if (!triGeom.scale.isValid() || !triGeom.scale.isFinite())  std::cerr << "WARNING: Shape " << pxShape << " (TriangleMesh) has invalid scale." << std::endl;
                    break;
                    case physx::PxGeometryType::ePLANE:
                        // PxPlaneGeometry doesn't have dimensions like radius/extents to validate in this context,
                        // it's defined by a normal and distance, implicitly infinite.
                    break;
                    default:
                    std::cout << "INFO: Shape " << pxShape << " has unhandled geometry type " << geomType << std::endl;
                    break;
                }
            }
        }*/

        return physicsID;
    }

    uint64_t CreateRigidStaticBoxFromExtents(Transform transform, glm::vec3 boxExtents, PhysicsFilterData filterData, Transform localOffset) {
        PxPhysics* pxPhysics = Hell::Physics::GetPxPhysics();
        PxScene* pxScene = Hell::Physics::GetPxScene();
        PxMaterial* material = Hell::Physics::GetDefaultMaterial();

        float halfWidth = boxExtents.x * 0.5f;
        float halfHeight = boxExtents.y * 0.5f;
        float halfDepth = boxExtents.z * 0.5f;

        PxFilterData pxFilterData;
        pxFilterData.word0 = (PxU32)filterData.raycastGroup;
        pxFilterData.word1 = (PxU32)filterData.collisionGroup;
        pxFilterData.word2 = (PxU32)filterData.collidesWith;

        // Create shape
        PxShape* pxShape = pxPhysics->createShape(PxBoxGeometry(halfWidth, halfHeight, halfDepth), *material, true);
        pxShape->setQueryFilterData(pxFilterData);       // ray casts
        pxShape->setSimulationFilterData(pxFilterData);  // collisions
        pxShape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, true);
        pxShape->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, true);

        PxTransform localOffsetTransform = PxTransform(GlmMat4ToPxMat44(localOffset.to_mat4()));
        pxShape->setLocalPose(localOffsetTransform);

        // Create rigid dynamic
        PxQuat quat = Hell::Physics::GlmQuatToPxQuat(glm::quat(transform.rotation));
        PxTransform pxTransform = PxTransform(PxVec3(transform.position.x, transform.position.y, transform.position.z), quat);
        PxRigidStatic* pxRigidStatic = pxPhysics->createRigidStatic(pxTransform);
        pxRigidStatic->attachShape(*pxShape);
        pxScene->addActor(*pxRigidStatic);

        // Create DynamicBox
        uint64_t physicsID = CreatePhysicsId(PhysicsObjectType::RIGID_STATIC);
        RigidStatic& rigidDynamic = g_rigidStatics[physicsID];

        // Update its pointers
        rigidDynamic.SetPxRigidStatic(pxRigidStatic);
        rigidDynamic.AddPxShape(pxShape);

        return physicsID;
    }

    uint64_t CreateRigidStaticPlane(glm::vec3 planeOrigin, glm::vec3 planeNormal, PhysicsFilterData filterData) {
        PxPhysics* pxPhysics = Hell::Physics::GetPxPhysics();
        PxScene* pxScene = Hell::Physics::GetPxScene();
        PxMaterial* material = Hell::Physics::GetDefaultMaterial();

        planeNormal = glm::normalize(planeNormal);

        PxFilterData pxFilterData;
        pxFilterData.word0 = (PxU32)filterData.raycastGroup;
        pxFilterData.word1 = (PxU32)filterData.collisionGroup;
        pxFilterData.word2 = (PxU32)filterData.collidesWith;

        PxShape* pxShape = pxPhysics->createShape(PxPlaneGeometry(), *material, true);
        pxShape->setQueryFilterData(pxFilterData);
        pxShape->setSimulationFilterData(pxFilterData);
        pxShape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, true);
        pxShape->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, true);

        PxVec3 pxPlaneOrigin = PxVec3(planeOrigin.x, planeOrigin.y, planeOrigin.z);
        PxVec3 pxPlaneNormal = PxVec3(planeNormal.x, planeNormal.y, planeNormal.z);

        PxPlane pxPlane = PxPlane(pxPlaneOrigin, pxPlaneNormal);
        PxTransform pxTransform = PxTransformFromPlaneEquation(pxPlane);

        PxRigidStatic* pxRigidStatic = pxPhysics->createRigidStatic(pxTransform);
        pxRigidStatic->attachShape(*pxShape);
        pxScene->addActor(*pxRigidStatic);

        uint64_t physicsID = CreatePhysicsId(PhysicsObjectType::RIGID_STATIC);
        RigidStatic& rigidStatic = g_rigidStatics[physicsID];

        rigidStatic.SetPxRigidStatic(pxRigidStatic);
        rigidStatic.AddPxShape(pxShape);

        return physicsID;
    }

    uint64_t CreateRigidStaticFromCapsule(Transform transform, float radius, float halfHeight, PhysicsFilterData filterData, Transform localOffset) {
        PxPhysics* pxPhysics = Hell::Physics::GetPxPhysics();
        PxScene* pxScene = Hell::Physics::GetPxScene();
        PxMaterial* material = Hell::Physics::GetDefaultMaterial();

        PxFilterData pxFilterData;
        pxFilterData.word0 = (PxU32)filterData.raycastGroup;
        pxFilterData.word1 = (PxU32)filterData.collisionGroup;
        pxFilterData.word2 = (PxU32)filterData.collidesWith;

        PxCapsuleGeometry geo;
        geo.radius = radius;
        geo.halfHeight = halfHeight;

        // Create shape
        PxShape* pxShape = pxPhysics->createShape(geo, *material, true);
        pxShape->setQueryFilterData(pxFilterData);       // ray casts
        pxShape->setSimulationFilterData(pxFilterData);  // collisions
        pxShape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, true);
        pxShape->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, true);

        PxTransform localOffsetTransform = PxTransform(GlmMat4ToPxMat44(localOffset.to_mat4()));
        pxShape->setLocalPose(localOffsetTransform);

        // Create rigid dynamic
        PxQuat quat = Hell::Physics::GlmQuatToPxQuat(glm::quat(transform.rotation));
        PxTransform pxTransform = PxTransform(PxVec3(transform.position.x, transform.position.y, transform.position.z), quat);
        PxRigidStatic* pxRigidStatic = pxPhysics->createRigidStatic(pxTransform);
        pxRigidStatic->attachShape(*pxShape);
        pxScene->addActor(*pxRigidStatic);

        // Create DynamicBox
        uint64_t physicsID = CreatePhysicsId(PhysicsObjectType::RIGID_STATIC);
        RigidStatic& rigidDynamic = g_rigidStatics[physicsID];

        // Update its pointers
        rigidDynamic.SetPxRigidStatic(pxRigidStatic);
        rigidDynamic.AddPxShape(pxShape);

        return physicsID;
    }

    uint64_t CreateRigidStaticConvexMeshFromVertices(Transform transform, const std::span<Vertex>& vertices, PhysicsFilterData filterData) {
       PxPhysics* pxPhysics = Hell::Physics::GetPxPhysics();
       PxScene* pxScene = Hell::Physics::GetPxScene();
       PxMaterial* material = Hell::Physics::GetDefaultMaterial();
  
       PxFilterData pxFilterData;
       pxFilterData.word0 = (PxU32)filterData.raycastGroup;
       pxFilterData.word1 = (PxU32)filterData.collisionGroup;
       pxFilterData.word2 = (PxU32)filterData.collidesWith;
  
       // Create convex shape
       std::vector<PxVec3> pxVertices;
       for (Vertex& vertex : vertices) {
           pxVertices.push_back(Hell::Physics::GlmVec3toPxVec3(vertex.position));
       }
  
       PxConvexMeshDesc convexDesc;
       convexDesc.points.count = pxVertices.size();
       convexDesc.points.stride = sizeof(PxVec3);
       convexDesc.points.data = pxVertices.data();
       convexDesc.flags = PxConvexFlag::eSHIFT_VERTICES | PxConvexFlag::eCOMPUTE_CONVEX;
       
       PxTolerancesScale scale;
       PxCookingParams params(scale);
  
       PxDefaultMemoryOutputStream buf;
       PxConvexMeshCookingResult::Enum result;
       if (!PxCookConvexMesh(params, convexDesc, buf, &result)) {
           std::cout << "some convex mesh shit failed\n";
           return 0;
       }
       PxDefaultMemoryInputData input(buf.getData(), buf.getSize());
       PxConvexMesh* convexMesh = pxPhysics->createConvexMesh(input);
       PxConvexMeshGeometryFlags flags(~PxConvexMeshGeometryFlag::eTIGHT_BOUNDS);
       PxConvexMeshGeometry geometry(convexMesh, PxMeshScale(PxVec3(1.0f)), flags);
  
       PxShape* pxShape = pxPhysics->createShape(geometry, *material);
       pxShape->setQueryFilterData(pxFilterData);       // ray casts
       pxShape->setSimulationFilterData(pxFilterData);  // collisions
  
       // Create PxRigidStatic
       PxQuat quat = Hell::Physics::GlmQuatToPxQuat(glm::quat(transform.rotation));
       PxTransform pxTransform = PxTransform(PxVec3(transform.position.x, transform.position.y, transform.position.z), quat);
       PxRigidStatic* pxRigidStatic = pxPhysics->createRigidStatic(pxTransform);
       pxRigidStatic->attachShape(*pxShape);
       pxScene->addActor(*pxRigidStatic);
    
       // Create RigidStatic
       uint64_t physicsID = CreatePhysicsId(PhysicsObjectType::RIGID_STATIC);
       RigidStatic& rigidStatic = g_rigidStatics[physicsID];
  
       // Update its pointers
       rigidStatic.SetPxRigidStatic(pxRigidStatic);
       rigidStatic.AddPxShape(pxShape);
  
       return physicsID;
   }

    uint64_t CreateRigidStaticConvexMeshFromModel(Transform transform, const std::string& modelName, PhysicsFilterData filterData) {
       PxPhysics* pxPhysics = Hell::Physics::GetPxPhysics();
       PxScene* pxScene = Hell::Physics::GetPxScene();
       PxMaterial* material = Hell::Physics::GetDefaultMaterial();

       PxFilterData pxFilterData;
       pxFilterData.word0 = (PxU32)filterData.raycastGroup;
       pxFilterData.word1 = (PxU32)filterData.collisionGroup;
       pxFilterData.word2 = (PxU32)filterData.collidesWith;

       Model* model = Hell::ResourceManager::GetModelByName(modelName);
       if (!model) {
           std::cout << "Hell::Physics::CreateRigidStaticFromConvexMeshFromModel() failed: '" << modelName << "' was not found \"n";
           return 0;
       }

       // Create PxRigidStatic
       PxQuat quat = Hell::Physics::GlmQuatToPxQuat(glm::quat(transform.rotation));
       PxTransform pxTransform = PxTransform(PxVec3(transform.position.x, transform.position.y, transform.position.z), quat);
       PxRigidStatic* pxRigidStatic = pxPhysics->createRigidStatic(pxTransform);

       // Create RigidStatic
       uint64_t physicsID = CreatePhysicsId(PhysicsObjectType::RIGID_STATIC);
       RigidStatic& rigidStatic = g_rigidStatics[physicsID];

       // Create convex shapes
       Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("AssetGeometry");
       for (uint32_t meshId : model->GetMeshIndices()) {
           Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(meshId);
           std::span<Vertex> vertices(meshBuffer.GetVertices().data() + mesh->baseVertex, mesh->vertexCount);

           std::vector<PxVec3> pxVertices;
           for (Vertex& vertex : vertices) {
               pxVertices.push_back(Hell::Physics::GlmVec3toPxVec3(vertex.position));
           }

           PxConvexMeshDesc convexDesc;
           convexDesc.points.count = pxVertices.size();
           convexDesc.points.stride = sizeof(PxVec3);
           convexDesc.points.data = pxVertices.data();
           convexDesc.flags = PxConvexFlag::eSHIFT_VERTICES | PxConvexFlag::eCOMPUTE_CONVEX;

           PxTolerancesScale scale;
           PxCookingParams params(scale);

           PxDefaultMemoryOutputStream buf;
           PxConvexMeshCookingResult::Enum result;
           if (!PxCookConvexMesh(params, convexDesc, buf, &result)) {
               std::cout << "some convex mesh shit failed\n";
               return 0;
           }
           PxDefaultMemoryInputData input(buf.getData(), buf.getSize());
           PxConvexMesh* convexMesh = pxPhysics->createConvexMesh(input);
           PxConvexMeshGeometryFlags flags(~PxConvexMeshGeometryFlag::eTIGHT_BOUNDS);
           PxConvexMeshGeometry geometry(convexMesh, PxMeshScale(PxVec3(1.0f)), flags);

           PxShape* pxShape = pxPhysics->createShape(geometry, *material);
           pxShape->setQueryFilterData(pxFilterData);       // ray casts
           pxShape->setSimulationFilterData(pxFilterData);  // collisions
           pxRigidStatic->attachShape(*pxShape);
           rigidStatic.AddPxShape(pxShape);
       }

       pxScene->addActor(*pxRigidStatic);

       // Update its pointers
       rigidStatic.SetPxRigidStatic(pxRigidStatic);

       std::cout << "APPARENTLY CRAETED CONVEX MESH: " << modelName << "\n";

       return physicsID;
   }

    uint64_t CreateRigidStaticTriangleMeshFromModel(Transform transform, const std::string& modelName, PhysicsFilterData filterData) {
       Model* model = Hell::ResourceManager::GetModelByName(modelName);
       if (!model) {
           std::cout << "Hell::Physics::CreateRigidStaticTriangleMeshFromModel() failed: model name '" << modelName << "' not found\n";
           return 0;
       }

       Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("AssetGeometry");
       std::vector<Vertex>& globalVertices = meshBuffer.GetVertices();
       std::vector<uint32_t>& globalIndices = meshBuffer.GetIndices();

       std::vector<Vertex> vertices;
       std::vector<uint32_t> indices;

       for (uint32_t meshId : model->GetMeshIndices()) {
           Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(meshId);
           if (!mesh) continue;

           for (int i = mesh->baseIndex; i < mesh->baseIndex + mesh->indexCount; i++) {
               uint32_t index = globalIndices[i];
               const Vertex& vertex = globalVertices[index + mesh->baseVertex];
               vertices.push_back(vertex);
               indices.push_back(static_cast<uint32_t>(vertices.size() - 1));
           }
       }

       return CreateRigidStaticTriangleMeshFromVertexData(transform, vertices, indices, filterData);
    }

    uint64_t CreateRigidStaticTriangleMeshFromVertexData(Transform transform, const std::span<Vertex>& vertices, const std::span<uint32_t>& indices, PhysicsFilterData filterData) {
       PxPhysics* pxPhysics = Hell::Physics::GetPxPhysics();
       PxScene* pxScene = Hell::Physics::GetPxScene();
       PxMaterial* material = Hell::Physics::GetDefaultMaterial();

       PxFilterData pxFilterData;
       pxFilterData.word0 = (PxU32)filterData.raycastGroup;
       pxFilterData.word1 = (PxU32)filterData.collisionGroup;
       pxFilterData.word2 = (PxU32)filterData.collidesWith;

       PxTriangleMeshDesc meshDesc;
       meshDesc.points.count = vertices.size();
       meshDesc.points.data = vertices.data();
       meshDesc.points.stride = sizeof(Vertex);
       meshDesc.triangles.count = indices.size() / 3;
       meshDesc.triangles.data = indices.data();
       meshDesc.triangles.stride = 3 * sizeof(PxU32);

       if (!meshDesc.isValid()) {
           std::cout << "PxTriangleMeshDesc is invalid!\n";
           std::cout << "Vertex count: " << meshDesc.points.count << "\n";
           std::cout << "Vertex stride: " << meshDesc.points.stride << "\n";
           std::cout << "Triangle count: " << meshDesc.triangles.count << "\n";
           std::cout << "Triangle stride: " << meshDesc.triangles.stride << "\n";
           return 0;
       }
       
       PxCookingParams params{ PxTolerancesScale() };
       params.midphaseDesc = PxMeshMidPhase::eBVH33;
       params.suppressTriangleMeshRemapTable = true;

       // Clear preprocessing flags using the proper PhysX API
       params.meshPreprocessParams.clear(PxMeshPreprocessingFlag::eDISABLE_CLEAN_MESH);
       params.meshPreprocessParams.clear(PxMeshPreprocessingFlag::eDISABLE_ACTIVE_EDGES_PRECOMPUTE);

       // Optimize BVH33 midphase for simulation performance
       params.midphaseDesc.mBVH33Desc.meshCookingHint = PxMeshCookingHint::eSIM_PERFORMANCE;
       params.midphaseDesc.mBVH33Desc.meshSizePerformanceTradeOff = 0.0f;

       // Validate the mesh cooking parameters
       PX_ASSERT(PxValidateTriangleMesh(params, meshDesc));

       // Create the triangle mesh
       PxTriangleMesh* pxTriangleMesh = PxCreateTriangleMesh(params, meshDesc, pxPhysics->getPhysicsInsertionCallback());

       // Create PxShape
       PxMeshGeometryFlags flags(~PxMeshGeometryFlag::eDOUBLE_SIDED);
       PxMeshScale meshScale = PxVec3(1.0f, 1.0f, 1.0f);
       PxTriangleMeshGeometry geometry(pxTriangleMesh, meshScale, flags);

       PxShapeFlags shapeFlags(PxShapeFlag::eSCENE_QUERY_SHAPE); // NOT eSIMULATION_SHAPE. PhysX does not allow for triangle mesh
       PxShape* pxShape = pxPhysics->createShape(geometry, *material, shapeFlags);
       pxShape->setQueryFilterData(pxFilterData);       // ray casts
       pxShape->setSimulationFilterData(pxFilterData);  // collisions

       // Create rigid static
       PxQuat quat = Hell::Physics::GlmQuatToPxQuat(glm::quat(transform.rotation));
       PxTransform pxTransform = PxTransform(PxVec3(transform.position.x, transform.position.y, transform.position.z), quat);
       PxRigidStatic* pxRigidStatic = pxPhysics->createRigidStatic(pxTransform);
       pxScene->addActor(*pxRigidStatic);
       pxRigidStatic->attachShape(*pxShape);

       // Create Rigid Static
       uint64_t physicsID = CreatePhysicsId(PhysicsObjectType::RIGID_STATIC);
       RigidStatic& rigidStatic = g_rigidStatics[physicsID];
       rigidStatic.AddPxShape(pxShape);

       // Update its pointers
       rigidStatic.SetPxRigidStatic(pxRigidStatic);

       return physicsID;
   }

    void Destroy(PxRigidDynamic*& rigidDynamic) {
        if (rigidDynamic) {
            if (rigidDynamic->userData) {
                //delete static_cast<PhysicsObjectData*>(rigidDynamic->userData);
                rigidDynamic->userData = nullptr;
            }
            Hell::Physics::GetPxScene()->removeActor(*rigidDynamic);
            rigidDynamic->release();
            rigidDynamic = nullptr;
        }
    }

    void Destroy(PxRigidStatic*& rigidStatic) {
        if (rigidStatic) {
            if (rigidStatic->userData) {
                //delete static_cast<PhysicsObjectData*>(rigidStatic->userData);
                rigidStatic->userData = nullptr;
            }
            Hell::Physics::GetPxScene()->removeActor(*rigidStatic);
            rigidStatic->release();
            rigidStatic = nullptr;
        }
    }

    void Destroy(PxShape*& shape) {
        if (shape) {
            if (shape->userData) {
                //delete static_cast<PhysicsObjectData*>(shape->userData);
                shape->userData = nullptr;
            }
            shape->release();
            shape = nullptr;
        }
    }

    void Destroy(PxRigidBody*& rigidBody) {
        if (rigidBody) {
            if (rigidBody->userData) {
                //delete static_cast<PhysicsObjectData*>(rigidBody->userData);
                rigidBody->userData = nullptr;
            }
            Hell::Physics::GetPxScene()->removeActor(*rigidBody);
            rigidBody->release();
            rigidBody = nullptr;
        }
    }

    void Destroy(PxTriangleMesh*& triangleMesh) {
        if (triangleMesh) {
            triangleMesh = nullptr;
        }
    }
}
