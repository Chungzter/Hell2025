#include "Door.h"
#include "Hell/Audio.h"
#include "Bible/Bible.h"
#include "Debug/DebugDraw.h"
#include "Editor/Editor.h"
#include "Physics/Physics.h"
#include "Renderer/RenderDataManager.h"
#include "Managers/OpenableManager.h"
#include "Game/UniqueID.h"
#include "Util.h"
#include "World/World.h"

#include "Core/GameOLD.h"

#include "Hell/Logging.h"

Door::Door(uint64_t id, DoorCreateInfo& createInfo, SpawnOffset& spawnOffset) {
    m_objectId = id;
	m_createInfo = createInfo;
	m_spawnOffset = spawnOffset;

    m_position = createInfo.position + spawnOffset.translation;
    m_rotation = createInfo.rotation + glm::vec3(0.0f, spawnOffset.yRotation, 0.0f);

    // Sensible defaults if for whatever reason your map file was missing this field
    if (m_createInfo.type == DoorType::UNDEFINED)                           m_createInfo.type = DoorType::STANDARD_A;
    if (m_createInfo.materialTypeFront == DoorMaterialType::UNDEFINED)      m_createInfo.materialTypeFront = DoorMaterialType::RESIDENT_EVIL;
    if (m_createInfo.materialTypeBack == DoorMaterialType::UNDEFINED)       m_createInfo.materialTypeBack = DoorMaterialType::RESIDENT_EVIL;
    if (m_createInfo.materialTypeFrameFront == DoorMaterialType::UNDEFINED) m_createInfo.materialTypeFrameFront = DoorMaterialType::RESIDENT_EVIL;
    if (m_createInfo.materialTypeFrameBack == DoorMaterialType::UNDEFINED)  m_createInfo.materialTypeFrameBack = DoorMaterialType::RESIDENT_EVIL;

    Bible::ConfigureDoorMeshNodes(id, m_createInfo, &m_meshNodes);

    if (m_createInfo.deadLockedAtInit) {
        m_deadLocked = true;

        // Iterate the mesh nodes, find any openable ID, and lock the cunt
        for (const MeshNode& meshNode : m_meshNodes.GetNodes()) {
            if (meshNode.openableId != 0) {
                OpenableManager::LockOpenablebyId(meshNode.openableId);
            }
        }
    }

   // DeadLock& deadLock = m_deadLocks.emplace_back();
   // deadLock.Init(m_objectId, glm::vec3(0.0f, 0.131056f, 0.0f), DeadLockType::BOLT);
   //
   // DeadLock& deadLock2 = m_deadLocks.emplace_back();
   // deadLock2.Init(m_objectId, glm::vec3(0.0f, 1.95425f, 0.0f), DeadLockType::BOLT);

    UpdateFloor();
    UpdateWorldForward();
    CreateRaytracingVertices();
}

void Door::UpdateFloor() {
    float half_w = 0.05f;
    float half_d = 0.4f;

    //half_w = 0.1f;
	//half_d = 0.3f;

    Transform transform;
    transform.position = m_position;
    transform.rotation = m_rotation;

    HousePlaneCreateInfo createInfo;
    createInfo.p0 = glm::vec3(transform.to_mat4() * glm::vec4(-half_w, 0.0f, -half_d, 1.0f));
    createInfo.p1 = glm::vec3(transform.to_mat4() * glm::vec4(-half_w, 0.0f, +half_d, 1.0f));
    createInfo.p2 = glm::vec3(transform.to_mat4() * glm::vec4(+half_w, 0.0f, +half_d, 1.0f));
    createInfo.p3 = glm::vec3(transform.to_mat4() * glm::vec4(+half_w, 0.0f, -half_d, 1.0f));
    createInfo.parentDoorId = GetObjectId();
    createInfo.type = HousePlaneType::FLOOR;

    createInfo.textureScale = 0.4f;
    createInfo.materialName = "FloorBoards";

    World::AddHousePlane(createInfo, SpawnOffset());
}

void Door::CleanUp() {
	m_meshNodes.CleanUp();
}

void Door::Update(float deltaTime) {
    // TODO: cache this at init rather than recompute every frame
    Transform transform;
    transform.position = m_position;
    transform.rotation = m_rotation;

    // Store it, this is used by the deadlocks
    m_doorModelMatrix = transform.to_mat4();

    m_meshNodes.Update(m_doorModelMatrix);

    m_renderItems = m_meshNodes.GetRenderItems();

    for (DeadLock& deadLock : m_deadLocks) {
        deadLock.Update(deltaTime);
        const std::vector<RenderItem>& deadLockRenderItems = deadLock.m_meshNodes.GetRenderItems();
        m_renderItems.insert(m_renderItems.end(), deadLockRenderItems.begin(), deadLockRenderItems.end());
    }

    // Retrieve physics AABB
    bool found = false;
    for (const MeshNode& meshNode : m_meshNodes.GetNodes()) {
        if (RigidDynamic* rigidDynamic = Physics::GetRigidDynamicById(meshNode.rigidDynamicId)) {
            if (found) {
                Logging::Warning() << "There's a door with more than 1 mesh node with a rigidDynamicId\n";
            }
            m_physicsAABB = rigidDynamic->GetAABB();
            found = true;
        }
    }

    // DebugDraw();

    //CreateRaytracingVertices();
}

void Door::CreateRaytracingVertices() {
    // TODO: get this out of here and make a raytracing shadow caster mesh manager thing for anything that casts shadows
    m_raytracingDoorMesh.Reset();

    float w = DOOR_DEPTH;
    float h = DOOR_HEIGHT;
    float d = DOOR_WIDTH;

    std::vector<Vertex> vertices;
    vertices.reserve(24);

    float paddingPosX = 0.01f;
    float paddingPosY = 0.03f;
    float paddingPosZ = 0.02f;
    float paddingNegX = 0.08f;
    float paddingNegY = 0.03f;
    float paddingNegZ = 0.02f;

    // Corners
    glm::vec3 p0 = glm::vec3(0 + paddingPosX, 0 - paddingNegY, 0 + paddingPosZ); // front bottom right
    glm::vec3 p1 = glm::vec3(-w - paddingNegX, 0 - paddingNegY, 0 + paddingPosZ); // front bottom left
    glm::vec3 p2 = glm::vec3(-w - paddingNegX, h + paddingPosY, 0 + paddingPosZ); // front top left
    glm::vec3 p3 = glm::vec3(0 + paddingPosX, h + paddingPosY, 0 + paddingPosZ); // front top right
    glm::vec3 p4 = glm::vec3(0 + paddingPosX, 0 - paddingNegY, -d - paddingNegZ); // back bottom right
    glm::vec3 p5 = glm::vec3(-w - paddingNegX, 0 - paddingNegY, -d - paddingNegZ); // back bottom left
    glm::vec3 p6 = glm::vec3(-w - paddingNegX, h + paddingPosY, -d - paddingNegZ); // back top left
    glm::vec3 p7 = glm::vec3(0 + paddingPosX, h + paddingPosY, -d - paddingNegZ); // back top right

    // front face
    vertices.emplace_back(Vertex(p0, glm::vec3(0, 0, 1)));
    vertices.emplace_back(Vertex(p3, glm::vec3(0, 0, 1)));
    vertices.emplace_back(Vertex(p2, glm::vec3(0, 0, 1)));
    vertices.emplace_back(Vertex(p1, glm::vec3(0, 0, 1)));

    // back face
    vertices.emplace_back(Vertex(p5, glm::vec3(0, 0, -1)));
    vertices.emplace_back(Vertex(p6, glm::vec3(0, 0, -1)));
    vertices.emplace_back(Vertex(p7, glm::vec3(0, 0, -1)));
    vertices.emplace_back(Vertex(p4, glm::vec3(0, 0, -1)));

    // left face
    vertices.emplace_back(Vertex(p1, glm::vec3(-1, 0, 0)));
    vertices.emplace_back(Vertex(p2, glm::vec3(-1, 0, 0)));
    vertices.emplace_back(Vertex(p6, glm::vec3(-1, 0, 0)));
    vertices.emplace_back(Vertex(p5, glm::vec3(-1, 0, 0)));

    // right face
    vertices.emplace_back(Vertex(p4, glm::vec3(1, 0, 0)));
    vertices.emplace_back(Vertex(p7, glm::vec3(1, 0, 0)));
    vertices.emplace_back(Vertex(p3, glm::vec3(1, 0, 0)));
    vertices.emplace_back(Vertex(p0, glm::vec3(1, 0, 0)));

    // top face
    vertices.emplace_back(Vertex(p3, glm::vec3(0, 1, 0)));
    vertices.emplace_back(Vertex(p7, glm::vec3(0, 1, 0)));
    vertices.emplace_back(Vertex(p6, glm::vec3(0, 1, 0)));
    vertices.emplace_back(Vertex(p2, glm::vec3(0, 1, 0)));

    // bottom face
    vertices.emplace_back(Vertex(p1, glm::vec3(0, -1, 0)));
    vertices.emplace_back(Vertex(p5, glm::vec3(0, -1, 0)));
    vertices.emplace_back(Vertex(p4, glm::vec3(0, -1, 0)));
    vertices.emplace_back(Vertex(p0, glm::vec3(0, -1, 0)));

    // Indices
    std::vector<uint32_t> indices = {
        0, 1, 2, 2, 3, 0,       // front
        4, 5, 6, 6, 7, 4,       // back
        8, 9, 10, 10, 11, 8,    // left
        12, 13, 14, 14, 15, 12, // right
        16, 17, 18, 18, 19, 16, // top
        20, 21, 22, 22, 23, 20  // bottom
    };

    m_raytracingDoorMesh.AddMesh(vertices, indices, "RaytracingDoor");
    m_raytracingDoorMesh.UpdateBuffers();
}

void Door::UpdateWorldForward() {
    Transform transform;
    transform.rotation = m_rotation;
    m_worldForward = glm::vec3(transform.to_mat4() * glm::vec4(m_localForward, 1.0f));
}

bool Door::CameraFacingDoorWorldForward(const glm::vec3& cameraPositon, const glm::vec3& cameraForward) {
    glm::vec3 toCamera = cameraPositon - GetPosition();
    glm::vec3 toDoor = GetPosition() - cameraPositon;

    bool cameraOnFrontSide = glm::dot(m_worldForward, toDoor) > 0.0f;
    bool doorInFrontOfCamera = glm::dot(cameraForward, toDoor) > 0.0f;

    return !(cameraOnFrontSide && doorInFrontOfCamera);
}

void Door::DebugDraw() {

    glm::vec4 color = GREEN;

    Player* player = GameOLD::GetLocalPlayerByIndex(0);
    if (!player) return;

    if (CameraFacingDoorWorldForward(player->GetCameraPosition(), player->GetCameraForward())) {
        color = GREEN;
    }
    else {
        color = RED;
    }

    glm::vec3 p1 = GetPosition();
    glm::vec3 p2 = GetPosition() + m_worldForward;
    DebugDraw::DrawLine(p1, p2, color);
    DebugDraw::DrawPoint(p1, color);
    DebugDraw::DrawPoint(p2, color);
}


void Door::SetPosition(const glm::vec3& position) {
    m_createInfo.position = position;
    m_position = position;
}

void Door::SetRotationY(float value) {
    m_createInfo.rotation.y = value;
    m_rotation.y = value;
}

void Door::SetEditorName(const std::string& name) {
    m_createInfo.editorName = name;
    Bible::ConfigureDoorMeshNodes(m_objectId, m_createInfo, &m_meshNodes);
}

void Door::SetType(DoorType type) {
    m_createInfo.type = type;
    Bible::ConfigureDoorMeshNodes(m_objectId, m_createInfo, &m_meshNodes);
}

void Door::SetFrontMaterial(DoorMaterialType type) {
    m_createInfo.materialTypeFront = type;
    Bible::ConfigureDoorMeshNodes(m_objectId, m_createInfo, &m_meshNodes);
}

void Door::SetBackMaterial(DoorMaterialType type) {
    m_createInfo.materialTypeBack = type;
    Bible::ConfigureDoorMeshNodes(m_objectId, m_createInfo, &m_meshNodes);
}

void Door::SetFrameFrontMaterial(DoorMaterialType type) {
    m_createInfo.materialTypeFrameFront = type;
    Bible::ConfigureDoorMeshNodes(m_objectId, m_createInfo, &m_meshNodes);
}

void Door::SetFrameBackMaterial(DoorMaterialType type) {
    m_createInfo.materialTypeFrameBack = type;
    Bible::ConfigureDoorMeshNodes(m_objectId, m_createInfo, &m_meshNodes);
}

void Door::SetDeadLockState(bool value) {
    m_createInfo.hasDeadLock = value;
    Bible::ConfigureDoorMeshNodes(m_objectId, m_createInfo, &m_meshNodes);
}

void Door::SetDeadLockedAtInitState(bool value) {
    m_createInfo.deadLockedAtInit = value;
    Bible::ConfigureDoorMeshNodes(m_objectId, m_createInfo, &m_meshNodes);
}
