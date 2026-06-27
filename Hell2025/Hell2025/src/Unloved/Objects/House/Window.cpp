#include "Window.h"
#include "Unloved/Editor/Editor.h"
#include "Hell/Physics/Physics.h"
#include "Legacy/Renderer/RenderDataManager.h"
#include "Util.h"

namespace Unloved {

Window::Window(uint64_t id, const WindowCreateInfo& createInfo, const SpawnOffset& spawnOffset) {
    m_createInfo = createInfo;
    m_objectId = id;

    m_transform.position = m_createInfo.position + spawnOffset.translation;
    m_transform.rotation = m_createInfo.rotation + glm::vec3(0.0f, spawnOffset.yRotation, 0.0f);

    std::string interiorMaterial = "T_TrimInteriorRE";
    std::string exteriorMaterial = "T_TrimExteriorWP";
    std::string glassTopMaterial = "T_WindowsGlassTop";
    std::string glassBottomMaterial = "T_WindowsGlassBottom";

    std::vector<MeshNodeCreateInfo> meshNodeCreateInfoSet;

    MeshNodeCreateInfo& trimInterior = meshNodeCreateInfoSet.emplace_back();
    trimInterior.meshName = "TrimInterior";
    trimInterior.materialName = interiorMaterial;

    MeshNodeCreateInfo& trimExterior = meshNodeCreateInfoSet.emplace_back();
    trimExterior.meshName = "TrimExterior";
    trimExterior.materialName = exteriorMaterial;

    MeshNodeCreateInfo& sashTop = meshNodeCreateInfoSet.emplace_back();
    sashTop.meshName = "SashTop";
    sashTop.materialName = interiorMaterial;

    MeshNodeCreateInfo& sashBottom = meshNodeCreateInfoSet.emplace_back();
    sashBottom.meshName = "SashBottom";
    sashBottom.materialName = interiorMaterial;

    MeshNodeCreateInfo& lockTop = meshNodeCreateInfoSet.emplace_back();
    lockTop.meshName = "LockTop";
    lockTop.materialName = exteriorMaterial;

    MeshNodeCreateInfo& lockBottom = meshNodeCreateInfoSet.emplace_back();
    lockBottom.meshName = "LockBottom";
    lockBottom.materialName = exteriorMaterial;

    MeshNodeCreateInfo& handles = meshNodeCreateInfoSet.emplace_back();
    handles.meshName = "Handles";
    handles.materialName = exteriorMaterial;

    MeshNodeCreateInfo& glassTop = meshNodeCreateInfoSet.emplace_back();
    glassTop.meshName = "GlassTop";
    glassTop.materialName = glassTopMaterial;
    glassTop.blendingMode = BlendingMode::GLASS;
    glassTop.decalType = DecalType::GLASS;

    MeshNodeCreateInfo& glassBottom = meshNodeCreateInfoSet.emplace_back();
    glassBottom.meshName = "GlassBottom";
    glassBottom.materialName = glassBottomMaterial;
    glassBottom.blendingMode = BlendingMode::GLASS;
    glassBottom.decalType = DecalType::GLASS;

    m_meshNodes.Init(m_objectId, "Window", meshNodeCreateInfoSet);
    m_meshNodes.EnableCSMShadows();
    m_meshNodes.Update(m_transform.to_mat4());

    // Glass PhysX shapes
    //PhysicsFilterData filterData;
    //filterData.raycastGroup = RaycastGroup::RAYCAST_ENABLED;
    //filterData.collisionGroup = CollisionGroup::NO_COLLISION;
    //filterData.collidesWith = CollisionGroup::NO_COLLISION;
    //
    //filterData.collisionGroup = CollisionGroup::ENVIROMENT_OBSTACLE;
    //filterData.collidesWith = (CollisionGroup)(GENERIC_BOUNCEABLE | BULLET_CASING | RAGDOLL_PLAYER | RAGDOLL_ENEMY);
    //
    //m_physicsId = Hell::Physics::CreateRigidStaticTriangleMeshFromModel(m_transform, "WindowGlassPhysX", filterData);
    //
    //// Set PhysX user data
    //PhysicsUserData userData;
    //userData.physicsId = m_physicsId;
    //userData.objectId = m_objectId;
    //userData.physicsType = PhysicsType::RIGID_STATIC;
    //userData.objectType = ObjectType::WINDOW;
    //Hell::Physics::SetRigidStaticUserData(m_physicsId, userData);
}

void Window::CleanUp() {
    Hell::Physics::MarkRigidStaticForRemoval(m_physicsId);
}

void Window::SetPosition(const glm::vec3& position) {
    m_createInfo.position = position;
    m_transform.position = position;
    m_meshNodes.Update(m_transform.to_mat4());
    Hell::Physics::SetRigidStaticWorldTransform(m_physicsId, m_transform.to_mat4());
}

void Window::SetRotationY(float value) {
    m_createInfo.rotation.y = value;
    m_meshNodes.Update(m_transform.to_mat4());
}

void Window::Update(float deltaTime) {
    // Nothing as of yet
}
}
