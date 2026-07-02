#include "WorldPlane.h"
#include "Hell/Geometry/Geometry.h"
#include "Unloved/Debug/DebugDraw.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/Physics/Physics.h"
#include "Hell/Transform.h"
#include "Legacy/Renderer/RenderDataManager.h"
#include "Legacy/World/LegacyWorld.h"
#include "Unloved/Systems/House/HouseBuilder.h"

namespace Unloved {

WorldPlane::WorldPlane(uint64_t id, const WorldPlaneCreateInfo& createInfo, const SpawnOffset& spawnOffset) {
    m_objectId = id;

    m_createInfo = createInfo;

    m_createInfo.p0 += spawnOffset.translation; // is this correct/safe?
    m_createInfo.p1 += spawnOffset.translation; // is this correct/safe?
    m_createInfo.p2 += spawnOffset.translation; // is this correct/safe?
    m_createInfo.p3 += spawnOffset.translation; // is this correct/safe?

    m_materialIndex = Hell::ResourceManager::GetMaterialIndexByName(m_createInfo.materialName);

    UpdateVertexDataFromCreateInfo();
}

void WorldPlane::UpdateVertexDataFromCreateInfo() {
    m_p0 = m_createInfo.p0;
    m_p1 = m_createInfo.p1;
    m_p2 = m_createInfo.p2;
    m_p3 = m_createInfo.p3;

    // Vertices
    m_vertices.resize(4);
    m_vertices[0].position = m_p0;
    m_vertices[1].position = m_p1;
    m_vertices[2].position = m_p2;
    m_vertices[3].position = m_p3;

    // Indices
    m_indices = { 0, 1, 2, 2, 3, 0 };

    // Update UVs
    for (Vertex& vertex : m_vertices) {
        glm::vec3 origin = glm::vec3(0, 0, 0);
        origin = glm::vec3(0);
        vertex.uv = Hell::Geometry::CalculateUV(vertex.position, glm::vec3(0.0f, 1.0f, 0.0f));
        vertex.uv *= m_createInfo.textureScale;
        vertex.uv.x += m_createInfo.textureOffsetU;
        vertex.uv.y += m_createInfo.textureOffsetV;
    }

    // Update normals and tangents
    for (int i = 0; i < m_indices.size(); i += 3) {
        Vertex& v0 = m_vertices[m_indices[i + 0]];
        Vertex& v1 = m_vertices[m_indices[i + 1]];
        Vertex& v2 = m_vertices[m_indices[i + 2]];
        Hell::Geometry::SetNormalsAndTangentsFromVertices(v0, v1, v2);
    }

    Hell::Physics::MarkRigidStaticForRemoval(m_physicsId);
    CreatePhysicsObject();

    // Calculate worldspace center
    m_worldSpaceCenter = (m_p0 + m_p1 + m_p2 + m_p3) / 4.0f;

    // Nav mesh poly
    m_navMeshPoly.clear();
    m_navMeshPoly.reserve(4);
    m_navMeshPoly.emplace_back(m_p0.x, m_p0.z);
    m_navMeshPoly.emplace_back(m_p1.x, m_p1.z);
    m_navMeshPoly.emplace_back(m_p2.x, m_p2.z);
    m_navMeshPoly.emplace_back(m_p3.x, m_p3.z);

    HouseBuilder::MarkDirty();
}

void WorldPlane::CleanUp() {
    Hell::Physics::MarkRigidStaticForRemoval(m_physicsId);
    m_vertices.clear();
    m_indices.clear();
    m_objectId = 0;
    m_physicsId = 0;
    m_p0 = glm::vec3(0.0f);
    m_p1 = glm::vec3(0.0f);
    m_p2 = glm::vec3(0.0f);
    m_p3 = glm::vec3(0.0f);
    m_materialIndex = -1;

    Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("Procedural");
    meshBuffer.RemoveMesh(m_meshId);
}

void WorldPlane::SetPosition(const glm::vec3& position) {
    UpdateWorldSpaceCenter(position);
}

void WorldPlane::UpdateWorldSpaceCenter(glm::vec3 worldSpaceCenter) {
    glm::vec3 offset = worldSpaceCenter - m_worldSpaceCenter;
    m_createInfo.p0 += offset;
    m_createInfo.p1 += offset;
    m_createInfo.p2 += offset;
    m_createInfo.p3 += offset;
    UpdateVertexDataFromCreateInfo();
}

void WorldPlane::SetMaterial(const std::string& materialName) {
    m_createInfo.materialName = materialName;
    m_materialIndex = Hell::ResourceManager::GetMaterialIndexByName(materialName);
}

Material* WorldPlane::GetMaterial() {
    return Hell::ResourceManager::GetMaterialByIndex(m_materialIndex);
}

void WorldPlane::SetMeshId(uint32_t meshId) {
    m_meshId = meshId;
}

void WorldPlane::SetTextureScale(float value) {
    m_createInfo.textureScale = value;
    UpdateVertexDataFromCreateInfo();
}

void WorldPlane::SetTextureOffsetU(float value) {
    m_createInfo.textureOffsetU = value;
    UpdateVertexDataFromCreateInfo();
}

void WorldPlane::SetTextureOffsetV(float value) {
    m_createInfo.textureOffsetV = value;
    UpdateVertexDataFromCreateInfo();
}

void WorldPlane::CreatePhysicsObject() {
    Hell::Physics::MarkRigidStaticForRemoval(m_physicsId);

    PhysicsFilterData filterData;
    filterData.raycastGroup = RAYCAST_ENABLED;
    filterData.collisionGroup = CollisionGroup::ENVIROMENT_OBSTACLE;
    filterData.collidesWith = (CollisionGroup)(GENERIC_BOUNCEABLE | BULLET_CASING | RAGDOLL_PLAYER | RAGDOLL_ENEMY | CHARACTER_CONTROLLER | ITEM_PICK_UP);

    m_physicsId = Hell::Physics::CreateRigidStaticTriangleMeshFromVertexData(Hell::Transform(), m_vertices, m_indices, filterData);

    // Set PhysX user data
    PhysicsUserData userData;
    userData.physicsId = m_physicsId;
    userData.objectId = m_objectId;
    userData.physicsType = PhysicsType::RIGID_STATIC;
    //userData.objectType = ObjectType::WORLD_PLANE;
    Hell::Physics::SetRigidStaticUserData(m_physicsId, userData);
}

void WorldPlane::SubmitRenderItem() {
    Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("Procedural");

    Mesh* mesh = meshBuffer.GetMeshById(m_meshId);
    if (!mesh) return;

    Material* material = Hell::ResourceManager::GetMaterialByIndex(m_materialIndex);
    if (!material) return;

	RenderItem renderItem;
	renderItem.baseColorTextureIndex = material->m_basecolor;
	renderItem.normalMapTextureIndex = material->m_normal;
	renderItem.rmaTextureIndex = material->m_rma;
	renderItem.modelMatrix = glm::mat4(1.0f);
	renderItem.inverseModelMatrix = glm::mat4(1.0f);
	renderItem.aabbMin = glm::vec4(mesh->aabbMin, 0.0f);
	renderItem.aabbMax = glm::vec4(mesh->aabbMax, 0.0f);
    renderItem.meshId = m_meshId;
    renderItem.baseVertex = mesh->baseVertex;
    renderItem.baseIndex = mesh->baseIndex;

	RenderDataManager::SubmitRenderItemProcedural(renderItem);
}

void WorldPlane::DrawVertices(glm::vec4 color) {
    DebugDraw::DrawPoint(m_p0, color);
    DebugDraw::DrawPoint(m_p1, color);
    DebugDraw::DrawPoint(m_p2, color);
    DebugDraw::DrawPoint(m_p3, color);
}

void WorldPlane::DrawEdges(glm::vec4 color) {
    DebugDraw::DrawLine(m_p0, m_p1, color);
    DebugDraw::DrawLine(m_p1, m_p2, color);
    DebugDraw::DrawLine(m_p2, m_p3, color);
    DebugDraw::DrawLine(m_p3, m_p0, color);
}

void WorldPlane::HideInEditor() {
    m_hiddenInEditor = true;
}

void WorldPlane::UnhideInEditor() {
	m_hiddenInEditor = false;
}
}
