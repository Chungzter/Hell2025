#include "MeshNodes.h"
#include "Debug/DebugDraw.h"
#include <Game/RendereringConstants.h>
#include "Hell/Logging.h"
#include "Editor/Editor.h"
#include "Managers/MirrorManager.h"
#include "Managers/OpenableManager.h"
#include "Renderer/RenderDataManager.h"
#include "World/LegacyWorld.h"
#include "Hell/Physics/Physics.h"
#include "Game/UniqueID.h"
#include "Util.h"

#include "Hell/ResourceManagement/ResourceManager.h"

void MeshNodes::Init(uint64_t parentId, const std::string& modelName, const std::vector<MeshNodeCreateInfo>& meshNodeCreateInfoSet) {
    Model* model = Hell::ResourceManager::GetModelByName(modelName);
    if (!model) {
        Logging::Error() << "MeshNodes::Init() failed because modelName was not found";
        return;
    }

    CleanUp();

    m_modelName = modelName;
    m_nodeCount = model->GetMeshCount();
    m_meshNodes.resize(m_nodeCount);

    // Defaults
    for (size_t i = 0; i < m_nodeCount; i++) {
        int globalMeshIndex = model->GetMeshIndices()[i];

        Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(globalMeshIndex);
        if (!mesh) {
            Logging::Error() << "MeshNodes::Init(...) failed to find mesh by id " << globalMeshIndex;
            continue;
        }

        // Map mesh name to local index for easy lookup
        m_localIndexMap[mesh->name] = i;

        MeshNode& meshNode = m_meshNodes[i];
        meshNode.blendingMode = BlendingMode::DEFAULT;
        meshNode.materialIndex = Hell::ResourceManager::GetMaterialIndexByName(DEFAULT_MATERIAL_NAME);
		meshNode.transform = Transform();
		meshNode.worldMatrix = glm::mat4(1.0f);
		meshNode.prevWorldMatrix = glm::mat4(1.0f);
        meshNode.inverseWorldMatrix = glm::mat4(1.0f);
        meshNode.localParentIndex = mesh->parentIndex;
        meshNode.localTransform = mesh->localTransform;
        meshNode.inverseBindTransform = mesh->inverseBindTransform;
        meshNode.localMatrix = glm::mat4(1.0f);
        meshNode.worldspaceAabb = AABB();
        meshNode.parentObjectId = parentId;
        meshNode.globalMeshIndex = globalMeshIndex;
        meshNode.customId = 0;
        meshNode.nodeIndex = i;
        meshNode.meshBvhId = mesh->meshBvhId;
        meshNode.forceDynamic = false;
        meshNode.castShadows = true;
        meshNode.castCSMShadows = false;
        meshNode.emissiveColor = glm::vec3(0.0f);
        meshNode.tintColor = glm::vec3(1.0f);
        meshNode.rigidDynamicId = 0;
        meshNode.worldSpaceObb.SetLocalBounds(AABB(mesh->aabbMin, mesh->aabbMax));
        meshNode.addToNavMesh = false;
        meshNode.baseVertex = mesh->baseVertex;
        meshNode.baseIndex = mesh->baseIndex;
    }

    // If the model contains armatures, store the first one (TODO: allow more maybe)
    if (!model->m_armatures.empty()) {
        m_armatureData = model->m_armatures[0];
    }

    // Apply any mesh node create info
    for (const MeshNodeCreateInfo& createInfo : meshNodeCreateInfoSet) {
		MeshNode* meshNode = GetMeshNodeByMeshName(createInfo.meshName);
        Mesh* mesh = nullptr;
        if (meshNode) {
            mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(meshNode->globalMeshIndex);
        }

        // Validate
        if (!mesh || !meshNode) {
            Logging::Error() << "MeshNodes::Init(...) failed to process meshNodeCreateInfoSet, mesh name '" << createInfo.meshName << "' not found in model '" << modelName << "'";
            PrintMeshNames();
			continue;
		}

        // Base color texture override
        if (createInfo.baseColorOverrideTextureName != UNDEFINED_STRING) {
            meshNode->baseColorOverrideTextureIndex = Hell::ResourceManager::GetTextureBindlessIndexByName(createInfo.baseColorOverrideTextureName);
        }

        meshNode->materialIndex = Hell::ResourceManager::GetMaterialIndexByName(createInfo.materialName);
        meshNode->blendingMode = createInfo.blendingMode;
        meshNode->customId = createInfo.customId;
        meshNode->decalType = createInfo.decalType;
        meshNode->forceDynamic = createInfo.forceDynamic;
        meshNode->castShadows = createInfo.castShadows;
        meshNode->emissiveColor = createInfo.emissiveColor;
        meshNode->tintColor = createInfo.tintColor;
        meshNode->addToNavMesh = createInfo.addtoNavMesh;
        meshNode->scaleMatrix = glm::scale(glm::mat4(1.0f), createInfo.scale);

        int nodeIndex = m_localIndexMap[createInfo.meshName];

        if (createInfo.openable.isOpenable) {
            uint32_t openableId = OpenableManager::CreateOpenable(createInfo.openable, parentId);
            meshNode->openableId = openableId;
            meshNode->ownsOpenableId = true;

            for (const std::string& triggerMeshName : createInfo.openable.additionalTriggerMeshNames) {
                if (MeshNode* triggerMeshNode = GetMeshNodeByMeshName(triggerMeshName)) {
                    triggerMeshNode->openableId = openableId;
                }
            }
        }

        // Rigid dynamic
        if (createInfo.rigidDynamic.createObject) {

            // Create RigidDynamic if it doesn't exist
			if (meshNode->rigidDynamicId == 0) {
				glm::vec3 extents = mesh->aabbMax - mesh->aabbMin;
				glm::vec3 localCenter = 0.5f * (mesh->aabbMin + mesh->aabbMax);

				Transform spawnTransform;
				Transform offsetTransform;
                offsetTransform.position = localCenter;

                PhysicsFilterData filterData = createInfo.rigidDynamic.filterData;

                float mass = createInfo.rigidDynamic.mass;
                bool kinematic = createInfo.rigidDynamic.kinematic;

                if (createInfo.rigidDynamic.shapeType == PhysicsShapeType::BOX) {
                    meshNode->rigidDynamicId = Hell::Physics::CreateRigidDynamicFromBoxExtents(spawnTransform, extents, kinematic, mass, filterData, offsetTransform);
                }
                else if (createInfo.rigidDynamic.shapeType == PhysicsShapeType::CONVEX_MESH) {

                    // Everything in here is sketchy. Fix it!
                    if (Model* physicsModel = Hell::ResourceManager::GetModelByName(createInfo.rigidDynamic.convexMeshModelName)) {
                        Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("AssetGeometry");
                        int32_t meshId = physicsModel->GetMeshIndices()[0]; // you wanna do all mesh not just the first
                        Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(meshId);
                        if (mesh) {
                            std::span<Vertex> vertices(meshBuffer.GetVertices().data() + mesh->baseVertex, mesh->vertexCount);
                            std::span<uint32_t> indices(meshBuffer.GetIndices().data() + mesh->baseIndex, mesh->indexCount);
                            //meshNode->rigidDynamicId = Hell::Physics::CreateRigidDynamicFromConvexMeshVertices(spawnTransform, vertices, indices, mass, filterData);
                            meshNode->rigidDynamicId = Hell::Physics::CreateRigidDynamicWithCompoundConvexMeshesFromModel(createInfo.rigidDynamic.convexMeshModelName, mass, kinematic, filterData);

                            // DIRTY (fix me)
                            if (kinematic) {
                                if (RigidDynamic* rigidDynamic = Hell::Physics::GetRigidDynamicById(meshNode->rigidDynamicId)) {
                                    if (PxRigidDynamic* pxRigidDynamic = rigidDynamic->GetPxRigidDynamic()) {
                                        pxRigidDynamic->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
                                    }
                                }
                            }
                        }
                    }
                }

                // Configure physics user data
                if (meshNode->rigidDynamicId != 0) {
                    PhysicsUserData userData;
                    userData.physicsId = meshNode->rigidDynamicId;
                    userData.objectId = meshNode->parentObjectId;
                    userData.physicsType = PhysicsType::RIGID_DYNAMIC;
                    //userData.objectType = UniqueID::GetType(meshNode->parentObjectId);
                    Hell::Physics::SetRigidDynamicUserData(meshNode->rigidDynamicId, userData);
                }
            }
            else {
                Logging::Warning() << "You tried to create an rigidDynamicAABB for mesh node " << mesh->name << " but it already had a rigidDynamicId\n";
            }
        }

        // If blending mode is mirror, then add a new mirror to the world
        if (meshNode->blendingMode == BlendingMode::MIRROR) {
            MirrorManager::AddMirror(parentId, meshNode->nodeIndex, meshNode->globalMeshIndex);
        }
    }
}

void MeshNodes::PrintMeshNames() {
    std::cout << m_modelName << "\n";

    for (size_t i = 0; i < m_meshNodes.size(); i++) {
        MeshNode& meshNode = m_meshNodes[i];
        Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(meshNode.globalMeshIndex);
        if (!mesh) continue;

        std::cout << "-" << i << ": " << mesh->name << "\n";
    }
}

bool MeshNodes::NodeExists(const std::string& meshName) {
    const auto it = m_localIndexMap.find(meshName);
    return (it != m_localIndexMap.end());
}

bool MeshNodes::BoneExists(const std::string& boneName) {
    for (Bone& bone : m_armatureData.bones) {
        if (bone.name == boneName)
            return true;
    }
    return false;
}

bool MeshNodes::HasNodeWithObjectId(uint64_t objectId) const {
    for (const MeshNode& meshNode : m_meshNodes) {
        if (meshNode.parentObjectId == objectId) {
            return true;
        }
    }
    return false;
}

bool MeshNodes::MeshNodeIsOpen(const std::string& meshName) {
    const MeshNode* meshNode = GetMeshNodeByMeshName(meshName);
    if (!meshNode) return false;

    if (Openable* openable = OpenableManager::GetOpenableByOpenableId(meshNode->openableId)) {
        if (openable->IsOpen()) {
            return false;
        }
    }
    return true;
}

bool MeshNodes::MeshNodeIsClosed(const std::string& meshName) {
    const MeshNode* meshNode = GetMeshNodeByMeshName(meshName);
    if (!meshNode) return false;

    if (Openable* openable = OpenableManager::GetOpenableByOpenableId(meshNode->openableId)) {
        if (openable->IsClosed()) {
            return false;
        }
    }
    return true;
}

bool MeshNodes::MeshNodeIsStatic(int localNodeIndex) {
    MeshNode* currentNode = GetMeshNodeByLocalIndex(localNodeIndex);

	while (currentNode) {
		if (currentNode->forceDynamic) return false;
		if (currentNode->openableId != 0) return false;
		if (currentNode->rigidDynamicId != 0) return false;

        // Walk up the tree via parent index
        currentNode = GetMeshNodeByLocalIndex(currentNode->localParentIndex);
    }

    return true;
}

bool MeshNodes::MeshNodeIsNonKinematicRigidDynamic(int localNodeIndex) {
    MeshNode* currentNode = GetMeshNodeByLocalIndex(localNodeIndex);

    while (currentNode) {
        if (currentNode->rigidDynamicId != 0 && !Hell::Physics::RigidDynamicIsKinematic(currentNode->rigidDynamicId)) return true;

        // Walk up the tree via parent index
        currentNode = GetMeshNodeByLocalIndex(currentNode->localParentIndex);
    }

    return false;
}

void MeshNodes::CleanUp() {
    // First remove physics shapes
    for (MeshNode& meshNode : m_meshNodes) {
        if (meshNode.rigidDynamicId != 0) {
            Hell::Physics::MarkRigidDynamicForRemoval(meshNode.rigidDynamicId);
        }
    }

    m_modelName = UNDEFINED_STRING;
    m_nodeCount = 0;
    m_meshNodes.clear();
    m_localIndexMap.clear();
    m_renderItems.clear();
    m_worldMatrixPreviousFrame = glm::mat4(0.0f); // Intentionally (0.0f) to force a dirty update on first use
    // Write this: m_armatureData.CleanUp();
    m_renderItems.clear();
    m_isDirty = true;
    m_forceDirty = true;
    m_firstFrame = true;
}

void MeshNodes::SetTransformByMeshName(const std::string& meshName, Transform transform) {
    if (MeshNode* meshNode = GetMeshNodeByMeshName(meshName)) {
        if (!Util::NearlyEqualTransform(transform, meshNode->transform)) {
            meshNode->transform = transform;
            ForceDirty();
        }
    }
}

void MeshNodes::SetMeshMaterials(const std::string& materialName) {
    int materialIndex = Hell::ResourceManager::GetMaterialIndexByName(materialName);
    if (materialIndex == -1) {
        Logging::Error() << "MeshNodes::SetMeshMaterials() failed: '" << materialName << "' not found";
        return;
    }
    for (MeshNode& meshNode : m_meshNodes) {
        meshNode.materialIndex = materialIndex;
    }
}

void MeshNodes::SetMaterialByMeshName(const std::string& meshName, const std::string& materialName) {
    int materialIndex = Hell::ResourceManager::GetMaterialIndexByName(materialName);
    if (materialIndex == -1) {
        Logging::Error() << "MeshNodes::SetMaterialByMeshName() failed: '" << materialName << "' not found";
        return;
    }
    MeshNode* meshNode = GetMeshNodeByMeshName(meshName);
    if (meshNode) {
        meshNode->materialIndex = materialIndex;
    }
}

void MeshNodes::SetBlendingModes(BlendingMode blendingMode) {
    for (MeshNode& meshNode : m_meshNodes) {
        meshNode.blendingMode = blendingMode;
    }
    ForceDirty();
}
void MeshNodes::SetBlendingModeByMeshName(const std::string& meshName, BlendingMode blendingMode) {
    MeshNode* meshNode = GetMeshNodeByMeshName(meshName);
    if (meshNode) {
        meshNode->blendingMode = blendingMode;
        ForceDirty();
    }

    if (!meshNode) {
        Logging::Error() << "MeshNodes::SetBlendingModeByMeshName() failed: '" << meshName << "' not found";
        PrintMeshNames();
        return;
    }
}

void MeshNodes::SetObjectIdByMeshName(const std::string& meshName, uint64_t id) {
    MeshNode* meshNode = GetMeshNodeByMeshName(meshName);
    if (meshNode) {
        meshNode->parentObjectId = id;
    }
}

void MeshNodes::SetOpenableByMeshName(const std::string& meshName, uint64_t openableId, uint64_t parentObjectId) {
    Openable* openable = OpenableManager::GetOpenableByOpenableId(openableId);
    if (!openable) {
        Logging::Error() << "MeshNodes::SetOpenableByMeshName failed: openableId " << openableId << " not found\n";
        return;
    }

    // A little messy. Maybe rethink me??? It overwrites objectId
    MeshNode* meshNode = GetMeshNodeByMeshName(meshName);
    if (meshNode) {
        openable->SetParentObjectId(meshNode->parentObjectId);
        meshNode->parentObjectId = openableId;
    }
    else {
        Logging::Error() << "MeshNodes::SetOpenableByMeshName failed because meshName '" << meshName << "' not found\n";
    }
}

void MeshNodes::DrawWorldspaceAABB(glm::vec4 color) {
    DebugDraw::DrawAABB(m_worldspaceAABB, color);
}

void MeshNodes::DrawWorldspaceAABBs(glm::vec4 color) {
    for (MeshNode& meshNode : m_meshNodes) {
        DebugDraw::DrawAABB(meshNode.worldspaceAabb, color);
    }
}

void MeshNodes::ForceDirty() {
    m_forceDirty = true;
}

void MeshNodes::ResetFirstFrame() {
    m_firstFrame = true;
}

const AABB* MeshNodes::GetWorldSpaceAabbByMeshName(const std::string& meshName) {
    const static AABB invalidAabb;

    MeshNode* meshNode = GetMeshNodeByMeshName(meshName);
    if (meshNode) {
        return &meshNode->worldspaceAabb;
    }

    Logging::Error() << "MeshNodes::GetWorldSpaceAabbByMeshName failed because meshName '" << meshName << "' not found\n";
    return &invalidAabb;
}

int32_t MeshNodes::GetGlobalMeshIndex(int nodeIndex) {
    MeshNode* meshNode = GetMeshNodeByLocalIndex(nodeIndex);
    if (meshNode) {
        return meshNode->globalMeshIndex;
    }
    Logging::Error() << "MeshNodes::GetGlobalMeshIndex failed because node index '" << nodeIndex << "' not found\n";
    return -1;
}

Material* MeshNodes::GetMaterial(int nodeIndex) {
    MeshNode* meshNode = GetMeshNodeByLocalIndex(nodeIndex);
    if (!meshNode) return Hell::ResourceManager::GetDefaultMaterial();

    return Hell::ResourceManager::GetMaterialByIndex(meshNode->materialIndex);
}

void MeshNodes::UpdateHierarchy() {
    for (MeshNode& meshNode : m_meshNodes) {
        //meshNode.prevlocalMatrix = meshNode.localMatrix;

        MeshNode* parentMeshNode = GetMeshNodeByLocalIndex(meshNode.localParentIndex);
        if (parentMeshNode) {
            meshNode.localMatrix = parentMeshNode->localMatrix * meshNode.localTransform * meshNode.transform.to_mat4() * meshNode.scaleMatrix;
        }
        else {
            meshNode.localMatrix = meshNode.localTransform * meshNode.transform.to_mat4() * meshNode.scaleMatrix;

            // Overwrite with non-kinematic rigid transform if this node has one
            if (!m_firstFrame && meshNode.rigidDynamicId != 0 && !Hell::Physics::RigidDynamicIsKinematic(meshNode.rigidDynamicId) && Editor::IsClosed()) {
                if (RigidDynamic* rigidDynamic = Hell::Physics::GetRigidDynamicById(meshNode.rigidDynamicId)) {
                    meshNode.localMatrix = rigidDynamic->GetWorldTransform() * meshNode.scaleMatrix;
                }
            }
        }

        meshNode.movedThisFrame = true;
    }
}

void MeshNodes::Update(const glm::mat4& worldMatrix) {
    for (MeshNode& meshNode : m_meshNodes) {
        meshNode.movedThisFrame = false;
    }

    //for (MeshNode& meshNode : m_meshNodes) {
    //    Renderer::DrawOBB(meshNode.worldSpaceObb, GREEN);
    //}

    // Check if the world matrix changed this frame
    const bool worldMatrixDirty = (m_firstFrame || !Util::Mat4NearlyEqual(worldMatrix, m_worldMatrixPreviousFrame));

    // Is the hierarchy dirty?
    bool hierarchyDirty = false;

    if (m_forceDirty) {
        hierarchyDirty = true;
    }

    // Openables
    for (MeshNode& meshNode : m_meshNodes) {
        if (meshNode.ownsOpenableId) {
            if (Openable* openable = OpenableManager::GetOpenableByOpenableId(meshNode.openableId)) {
                if (openable->IsDirty()) {
                    meshNode.transform = openable->m_transform;
                    hierarchyDirty = true;
                }
            }
        }
    }

    // Dirty if any node was rigid and moved
    for (MeshNode& meshNode : m_meshNodes) {
        if (meshNode.rigidDynamicId != 0 && Hell::Physics::RigidDynamicIsDirty(meshNode.rigidDynamicId)) {
            hierarchyDirty = true;
            break;
        }
    }

    if (hierarchyDirty) {
        UpdateHierarchy();
    }

    // If neither are dirty, then no need to recompute RenderItems
    if (!worldMatrixDirty && !hierarchyDirty) {
        m_isDirty = false;
        return;
    }
    m_isDirty = worldMatrixDirty || hierarchyDirty;

    // DrawWorldspaceAABBs(YELLOW); // NOTE THIS only draws aabbs when the hierarchy was dirty

    // Compute world matrices FIRST
    for (size_t i = 0; i < m_meshNodes.size(); i++) {
        MeshNode& meshNode = m_meshNodes[i];
        //meshNode.prevWorldMatrix = meshNode.worldMatrix;

        const bool physicsDrivesThis =
            (!m_firstFrame) &&
            (Editor::IsClosed()) &&
            (MeshNodeIsNonKinematicRigidDynamic((int32_t)i));

        if (physicsDrivesThis) {
            meshNode.worldMatrix = meshNode.localMatrix; // localMatrix already IS world (from rigid)
        }
        else {
            meshNode.worldMatrix = worldMatrix * meshNode.localMatrix;
        }

        meshNode.inverseWorldMatrix = glm::inverse(meshNode.worldMatrix);
    }

    // Now compute AABBs from the final world matrices
    UpdateAABBsFromWorldMatrices();

    m_renderItems.clear();

    for (size_t i = 0; i < m_meshNodes.size(); i++) {
        MeshNode& meshNode = m_meshNodes[i];
        Material* material = GetMaterial(i);
        if (!material) continue;

        // Update render item
        meshNode.inverseWorldMatrix = glm::inverse(meshNode.worldMatrix);
        meshNode.renderItem.objectType = (int)UniqueID::GetType(meshNode.parentObjectId);
        meshNode.renderItem.openableId = meshNode.openableId;
        meshNode.renderItem.customId = meshNode.customId;
        meshNode.renderItem.modelMatrix = meshNode.worldMatrix;
        meshNode.renderItem.prevModelMatrix = meshNode.prevWorldMatrix;
        meshNode.renderItem.inverseModelMatrix = meshNode.inverseWorldMatrix;
        meshNode.renderItem.meshId = GetGlobalMeshIndex(i);
        meshNode.renderItem.baseColorTextureIndex = material->m_basecolor;
		meshNode.renderItem.normalMapTextureIndex = material->m_normal;
		meshNode.renderItem.rmaTextureIndex = material->m_rma;
		meshNode.renderItem.emissiveTextureIndex = material->m_emissive;
        meshNode.renderItem.aabbMin = glm::vec4(meshNode.worldspaceAabb.GetBoundsMin(), 0.0f);
        meshNode.renderItem.aabbMax = glm::vec4(meshNode.worldspaceAabb.GetBoundsMax(), 0.0f);
        meshNode.renderItem.localMeshNodeIndex = i;
		meshNode.renderItem.emissiveR = meshNode.emissiveColor.r;
		meshNode.renderItem.emissiveG = meshNode.emissiveColor.g;
        meshNode.renderItem.emissiveB = meshNode.emissiveColor.b;
        meshNode.renderItem.tintColorR = meshNode.tintColor.r;
        meshNode.renderItem.tintColorG = meshNode.tintColor.g;
        meshNode.renderItem.tintColorB = meshNode.tintColor.b;
        meshNode.renderItem.opacityTextureIndex = material->m_opacity;
        meshNode.renderItem.hairMapTextureIndex = material->m_hairMaps;
        meshNode.renderItem.baseVertex = meshNode.baseVertex;
        meshNode.renderItem.baseIndex = meshNode.baseIndex;
        meshNode.renderItem.blendingMode = (int)meshNode.blendingMode;

        Util::SetBitState(meshNode.renderItem.shadowBit, SHADOW_BIT_CAST_SHADOW, meshNode.castShadows);
        Util::SetBitState(meshNode.renderItem.shadowBit, SHADOW_BIT_CAST_CSM_SHADOW, meshNode.castCSMShadows);

        if (meshNode.baseColorOverrideTextureIndex != -1) {
            meshNode.renderItem.baseColorTextureIndex = meshNode.baseColorOverrideTextureIndex;
        }

        if (m_firstFrame) {
            meshNode.renderItem.prevModelMatrix = meshNode.worldMatrix;
        }

        Util::PackUint64(meshNode.parentObjectId, meshNode.renderItem.objectIdLowerBit, meshNode.renderItem.objectIdUpperBit);

        //switch (meshNode.blendingMode) {
        //case BlendingMode::DEFAULT:       m_renderItems.push_back(meshNode.renderItem);                 break;
        //case BlendingMode::ALPHA_DISCARD: m_renderItemsAlphaDiscarded.push_back(meshNode.renderItem);   break;
        //case BlendingMode::BLENDED:       m_renderItemsBlended.push_back(meshNode.renderItem);          break;
        //case BlendingMode::GLASS:         m_renderItemsGlass.push_back(meshNode.renderItem);            break;
        //case BlendingMode::HAIR:          m_renderItemsHair.push_back(meshNode.renderItem);     break;
        //case BlendingMode::TOILET_WATER:  m_renderItemsToiletWater.push_back(meshNode.renderItem);      break;
        //case BlendingMode::STAINED_GLASS: m_renderItemsStainedGlass.push_back(meshNode.renderItem);     break;
        //case BlendingMode::PLASTIC:       m_renderItemsPlastic.push_back(meshNode.renderItem);          break;
        //default: break;
        //}

        if (meshNode.blendingMode != BlendingMode::DO_NOT_RENDER) {
            m_renderItems.push_back(meshNode.renderItem);
        }

        //RenderDataManager::SubmitRenderItem(meshNode.renderItem);

        // If this is a static node and its transform is different than the previous frame, mark the World's static scene as dirty
        if (m_marksStaticSceneBvhAsDirty && MeshNodeIsStatic(i) && !Util::Mat4NearlyEqual(meshNode.worldMatrix, meshNode.prevWorldMatrix)) {
            LegacyWorld::MarkStaticSceneBvhDirty();

            if (Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(meshNode.globalMeshIndex)) {
                //std::cout << mesh->name << " triggered shit\n";
            }
        }
    }

    // Store previous frame data
    for (MeshNode& meshNode : m_meshNodes) {
        meshNode.prevWorldMatrix = meshNode.worldMatrix;

        //if (!m_firstFrame) {
        //    meshNode.movedThisFrame = !Util::Mat4NearlyEqual(meshNode.worldMatrix, meshNode.worldModelMatrixPreviousFrame);
        //}
        //else {
        //    meshNode.movedThisFrame = true;
        //}

    }

    m_worldMatrixPreviousFrame = worldMatrix;
    m_forceDirty = false;

    if (m_firstFrame) {
        InitPhysicsTransforms();
    }

    UpdateKinematicPhysicsTransforms();

    m_firstFrame = false;
}


void MeshNodes::SleepAllPhysics() {
    for (MeshNode& meshNode : m_meshNodes) {
        if (meshNode.rigidDynamicId != 0) {
            if (RigidDynamic* rigidDynamic = Hell::Physics::GetRigidDynamicById(meshNode.rigidDynamicId)) {
                if (PxRigidDynamic* pxRigidDynamic = rigidDynamic->GetPxRigidDynamic()) {
                    pxRigidDynamic->clearForce(PxForceMode::eFORCE);
                    pxRigidDynamic->clearTorque(PxForceMode::eFORCE);
                    pxRigidDynamic->setLinearVelocity(PxVec3(0.0f));
                    pxRigidDynamic->setAngularVelocity(PxVec3(0.0f));
                    pxRigidDynamic->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
                }
            }
        }
    }
}

void MeshNodes::WakeAllPhysics() {
    for (MeshNode& meshNode : m_meshNodes) {
        if (meshNode.rigidDynamicId == 0) {
            continue;
        }

        if (RigidDynamic* rigidDynamic = Hell::Physics::GetRigidDynamicById(meshNode.rigidDynamicId)) {
            if (PxRigidDynamic* pxRigidDynamic = rigidDynamic->GetPxRigidDynamic()) {

                pxRigidDynamic->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, false);

                // Nuke any leftover velocities/forces so waking is deterministic.
                pxRigidDynamic->clearForce(PxForceMode::eFORCE);
                pxRigidDynamic->clearTorque(PxForceMode::eFORCE);
                pxRigidDynamic->setLinearVelocity(PxVec3(0.0f));
                pxRigidDynamic->setAngularVelocity(PxVec3(0.0f));

                pxRigidDynamic->setWakeCounter(0.4f);
                pxRigidDynamic->wakeUp();
            }
        }
    }
}

void MeshNodes::AddForceToPhsyics(const glm::vec3 force) {
    for (MeshNode& meshNode : m_meshNodes) {
        if (meshNode.rigidDynamicId != 0) {
            if (!Hell::Physics::RigidDynamicIsKinematic(meshNode.rigidDynamicId)) {
                 Hell::Physics::AddFoceToRigidDynamic(meshNode.rigidDynamicId, force);
            }
        }
    }
}

void MeshNodes::EnableCSMShadows() {
    for (MeshNode& meshNode : m_meshNodes) {
        meshNode.castCSMShadows = true;
    }
}

void MeshNodes::EnablePointLightShadows() {
    for (MeshNode& meshNode : m_meshNodes) {
        meshNode.castShadows = true;
    }
}

void MeshNodes::DisablePointLightShadows() {
    for (MeshNode& meshNode : m_meshNodes) {
        meshNode.castShadows = false;
    }
}

void MeshNodes::DisableCSMShadows() {
    for (MeshNode& meshNode : m_meshNodes) {
        meshNode.castCSMShadows = false;
    }
}

void MeshNodes::DisableMarkingStaticSceneBvhAsDirty() {
    m_marksStaticSceneBvhAsDirty = false;
}

void MeshNodes::InitPhysicsTransforms() {
	for (MeshNode& meshNode : m_meshNodes) {
        if (meshNode.rigidDynamicId != 0 && Editor::IsClosed()) {
            Hell::Physics::SetRigidDynamicGlobalPose(meshNode.rigidDynamicId, meshNode.worldMatrix);
		}
	}
}


void MeshNodes::UpdateKinematicPhysicsTransforms() {
	for (MeshNode& meshNode : m_meshNodes) {
        if (meshNode.rigidDynamicId != 0 && meshNode.openableId != 0 && Editor::IsClosed()) {
            if (Hell::Physics::RigidDynamicIsKinematic(meshNode.rigidDynamicId)) {
                if (meshNode.movedThisFrame) {
                    Hell::Physics::SetRigidDynamicKinematicTarget(meshNode.rigidDynamicId, meshNode.worldMatrix);
               }
            }
        }
	}
}

void MeshNodes::UpdateAABBsFromWorldMatrices() {
    glm::vec3 minBounds(std::numeric_limits<float>::max());
    glm::vec3 maxBounds(-std::numeric_limits<float>::max());
    bool found = false;

    for (MeshNode& meshNode : m_meshNodes) {
        Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(meshNode.globalMeshIndex);
        if (!mesh) continue;

        const glm::vec3 localMin = mesh->aabbMin;
        const glm::vec3 localMax = mesh->aabbMax;
        const glm::vec3 localCenter = 0.5f * (localMin + localMax);
        const glm::vec3 localExtents = 0.5f * (localMax - localMin);

        const glm::mat4& M = meshNode.worldMatrix;

        const glm::vec3 worldCenter = glm::vec3(M * glm::vec4(localCenter, 1.0f));

        const glm::vec3 col0 = glm::vec3(M[0]);
        const glm::vec3 col1 = glm::vec3(M[1]);
        const glm::vec3 col2 = glm::vec3(M[2]);

        const glm::vec3 worldExtents =
            glm::abs(col0) * localExtents.x +
            glm::abs(col1) * localExtents.y +
            glm::abs(col2) * localExtents.z;

        const glm::vec3 worldMin = worldCenter - worldExtents;
        const glm::vec3 worldMax = worldCenter + worldExtents;

        meshNode.worldspaceAabb = AABB(worldMin, worldMax);
        meshNode.worldSpaceObb.SetTransform(M);

        minBounds = glm::min(minBounds, worldMin);
        maxBounds = glm::max(maxBounds, worldMax);
        found = true;
    }

    m_worldspaceAABB = found ? AABB(minBounds, maxBounds) : AABB();
}

/*/void MeshNodes::UpdateAABBs(const glm::mat4& worldMatrix) {
    glm::vec3 min(std::numeric_limits<float>::max());
    glm::vec3 max(-std::numeric_limits<float>::max());
    bool found = false;

    for (MeshNode& meshNode : m_meshNodes) {
        Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(meshNode.globalMeshIndex);
        if (!mesh) continue;

        glm::vec3 localMin = mesh->aabbMin;
        glm::vec3 localMax = mesh->aabbMax;
        glm::vec3 c = 0.5f * (localMin + localMax);
        glm::vec3 e = 0.5f * (localMax - localMin);

        glm::mat4 M = worldMatrix * meshNode.localMatrix;

        // UGLY
        // UGLY
        // UGLY
        if (MeshNodeIsNonKinematicRigidDynamic(meshNode.nodeIndex)) {
            M = meshNode.localMatrix;
        }
        // UGLY
        // UGLY
        // UGLY

        glm::vec3 worldCenter = glm::vec3(M * glm::vec4(c, 1.0f));

        glm::vec3 col0 = glm::vec3(M[0]);
        glm::vec3 col1 = glm::vec3(M[1]);
        glm::vec3 col2 = glm::vec3(M[2]);

        glm::vec3 worldExtents =
            glm::abs(col0) * e.x +
            glm::abs(col1) * e.y +
            glm::abs(col2) * e.z;

        glm::vec3 worldMin = worldCenter - worldExtents;
        glm::vec3 worldMax = worldCenter + worldExtents;

        meshNode.worldspaceAabb = AABB(worldMin, worldMax);
        meshNode.worldSpaceObb.SetTransform(M);

        min = glm::min(min, worldMin);
        max = glm::max(max, worldMax);
        found = true;
    }

    m_worldspaceAABB = found ? AABB(min, max) : AABB();

}*/

const glm::mat4& MeshNodes::GetLocalModelMatrix(int32_t nodeIndex) const {
    static const glm::mat4 identity = glm::mat4(1.0f);
    if (nodeIndex < 0) return identity;

    size_t i = static_cast<size_t>(nodeIndex);
    if (i >= m_meshNodes.size()) return identity;

    return m_meshNodes[i].localMatrix;
}

const glm::mat4& MeshNodes::GetWorldModelMatrix(int32_t nodeIndex) const {
    static const glm::mat4 identity = glm::mat4(1.0f);
    if (nodeIndex < 0) return identity;

    size_t i = static_cast<size_t>(nodeIndex);
    if (i >= m_meshNodes.size()) return identity;

    return m_meshNodes[i].worldMatrix;
}

const glm::mat4& MeshNodes::GetBoneLocalMatrix(const std::string& boneName) const {
    static glm::mat4 identity = glm::mat4(1.0f);

    for (const Bone& bone : m_armatureData.bones) {
        if (bone.name == boneName) {
            return bone.localRestPose;
        }
    }
    return identity;
}

const glm::mat4& MeshNodes::GetLocalTransform(int32_t nodeIndex) const {
    static const glm::mat4 identity = glm::mat4(1.0f);
    if (nodeIndex < 0) return identity;

    size_t i = static_cast<size_t>(nodeIndex);
    if (i >= m_meshNodes.size()) return identity;

    return m_meshNodes[i].localTransform;
}

const glm::mat4& MeshNodes::GetInverseBindTransform(int32_t nodeIndex) const {
    static const glm::mat4 identity = glm::mat4(1.0f);
    if (nodeIndex < 0) return identity;

    size_t i = static_cast<size_t>(nodeIndex);
    if (i >= m_meshNodes.size()) return identity;

    return m_meshNodes[i].inverseBindTransform;
}

const std::string& MeshNodes::GetMeshNameByNodeIndex(int32_t nodeIndex) const {
    static std::string notFound = "NOT_FOUND";

    for (const auto& [name, idx] : m_localIndexMap) {
        if (idx == nodeIndex) return name;
    }
    return notFound;
}

MeshNode* MeshNodes::GetMeshNodeByLocalIndex(int32_t nodeIndex) {
    if (nodeIndex < 0 || nodeIndex >= (int32_t)m_meshNodes.size()) return nullptr;
    return &m_meshNodes[nodeIndex];
}

MeshNode* MeshNodes::GetMeshNodeByMeshName(const std::string& meshName) {
    int32_t nodeIndex = GetMeshNodeIndexByMeshName(meshName);
    if (nodeIndex == -1 || nodeIndex >= (int32_t)m_meshNodes.size()) return nullptr;
    return &m_meshNodes[(size_t)nodeIndex];
}

int32_t MeshNodes::GetMeshNodeIndexByMeshName(const std::string& meshName) {
    auto it = m_localIndexMap.find(meshName);
    if (it == m_localIndexMap.end()) return -1;
    return (int32_t)it->second;
}
