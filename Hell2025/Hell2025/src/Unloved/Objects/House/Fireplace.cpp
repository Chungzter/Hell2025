#include "Fireplace.h"
#include "Unloved/Bible/Bible.h"
#include "Legacy/Renderer/Renderer.h"
#include "Legacy/Util/Util.h"
#include "Legacy/World/LegacyWorld.h"

namespace Unloved {

Fireplace::Fireplace(uint64_t id, const FireplaceCreateInfo& createInfo, const SpawnOffset& spawnOffset) {
    m_id = id;
    m_createInfo = createInfo;
    m_transform.position = m_createInfo.position + spawnOffset.translation;
    m_transform.rotation = m_createInfo.rotation + glm::vec3(0.0f, spawnOffset.yRotation, 0.0f);

    //m_transform.rotation.y = HELL_PI;

    UpdateWorldMatrix();

    //m_createInfo.type = FireplaceType::WOOD_STOVE;

    std::vector<MeshNodeCreateInfo> meshNodeCreateInfoSet;

    if (m_createInfo.type == FireplaceType::DEFAULT) {

        MeshNodeCreateInfo& walls = meshNodeCreateInfoSet.emplace_back();
        walls.meshName = "Fireplace_Body_Lower.002";
        //walls.rigidDynamic.createObject = true;
        //walls.rigidDynamic.kinematic = true;
        //walls.rigidDynamic.filterData.raycastGroup = RAYCAST_DISABLED;
        //walls.rigidDynamic.filterData.collisionGroup = CollisionGroup::ENVIROMENT_OBSTACLE;
        //walls.rigidDynamic.filterData.collidesWith = (CollisionGroup)(GENERIC_BOUNCEABLE | BULLET_CASING | RAGDOLL_PLAYER | RAGDOLL_ENEMY);
        //walls.addtoNavMesh = true;

        m_meshNodes.Init(id, "Fireplace", meshNodeCreateInfoSet);

        m_meshNodes.SetMeshMaterials("NumGrid");
        m_meshNodes.SetMeshMaterials("Ceiling2");
        m_meshNodes.SetMeshMaterials("WallPaper");
        m_meshNodes.SetMeshMaterials("PlasterRed");

        m_meshNodes.SetMaterialByMeshName("Fireplace_Wall_Damage", "Fireplace");
        m_meshNodes.SetMaterialByMeshName("Fireplace_Wall_Damage.001", "Fireplace");
        m_meshNodes.SetMaterialByMeshName("Fireplace_Bricks_low", "Fireplace");
        m_meshNodes.SetMaterialByMeshName("Fireplace_Cap_low", "Fireplace");
        m_meshNodes.SetMaterialByMeshName("Fireplace_Log_Tray_low", "Fireplace");
        m_meshNodes.SetMaterialByMeshName("Fireplace_Log_Try_Holder_low", "Fireplace");
        m_meshNodes.SetMaterialByMeshName("Fireplace_Floor_low", "Fireplace");

        m_meshNodes.SetMaterialByMeshName("Fireplace_Mantle_low", "Fireplace");
        m_meshNodes.SetMaterialByMeshName("Fireplace_Mantle_Trim_low", "Fireplace");

        m_meshNodes.SetMaterialByMeshName("Fireplace_Wood_Log1_low", "T_Firewood");
        m_meshNodes.SetMaterialByMeshName("Fireplace_Wood_Log2_low", "T_Firewood");
        m_meshNodes.SetMaterialByMeshName("Fireplace_Wood_Log3_low", "T_Firewood");
        m_meshNodes.SetMaterialByMeshName("Fireplace_Wood_Log4_low", "T_Firewood");
        m_meshNodes.SetMaterialByMeshName("Fireplace_Wood_Log5_low", "T_Firewood");
        m_meshNodes.SetMaterialByMeshName("Fireplace_Coal1_low", "T_Firewood");
        m_meshNodes.SetMaterialByMeshName("Fireplace_Coal2_low", "T_Firewood");
        m_meshNodes.SetBlendingModeByMeshName("FireBounds", BlendingMode::DO_NOT_RENDER);

        m_meshNodes.SetMaterialByMeshName("Fireplace_Wall_Damage", "T_WallsChippedEdges");
        m_meshNodes.SetMaterialByMeshName("Fireplace_Wall_Damage.001", "T_WallsChippedEdges");
        m_meshNodes.SetBlendingModeByMeshName("Fireplace_Wall_Damage", BlendingMode::ALPHA_DISCARD);
        m_meshNodes.SetBlendingModeByMeshName("Fireplace_Wall_Damage.001", BlendingMode::ALPHA_DISCARD);
    }

    if (m_createInfo.type == FireplaceType::WOOD_STOVE) {

        MeshNodeCreateInfo& walls = meshNodeCreateInfoSet.emplace_back();
        walls.meshName = "FireplaceBrick_WallMain";
        walls.rigidDynamic.createObject = true;
        walls.rigidDynamic.kinematic = true;
        walls.rigidDynamic.filterData.raycastGroup = RAYCAST_DISABLED;
        walls.rigidDynamic.filterData.collisionGroup = CollisionGroup::ENVIROMENT_OBSTACLE;
        walls.rigidDynamic.filterData.collidesWith = (CollisionGroup)(GENERIC_BOUNCEABLE | BULLET_CASING | RAGDOLL_PLAYER | RAGDOLL_ENEMY);
        walls.addtoNavMesh = true;

        MeshNodeCreateInfo& door = meshNodeCreateInfoSet.emplace_back();
        door.meshName = "FireplaceBrick_StoveDoor";
        door.materialName = "FireplaceB_Stove";
        door.openable.isOpenable = true;
        door.openable.openAxis = OpenAxis::ROTATE_Y_NEG;
        door.openable.initialOpenState = OpenState::CLOSED;
        door.openable.minOpenValue = 0.0f;
        door.openable.maxOpenValue = 1.7;
        door.openable.openSpeed = 7.25f;
        door.openable.closeSpeed = 7.25f;
        //door.openable.openingAudio = "BathroomCabinetOpen.wav";
        //door.openable.closingAudio = "BathroomCabinetClose.wav";
        door.openable.additionalTriggerMeshNames = { "FireplaceBrick_StoveHandle", "FireplaceBrick_StoveGlass" };

        MeshNodeCreateInfo& glass = meshNodeCreateInfoSet.emplace_back();
        glass.meshName = "FireplaceBrick_StoveGlass";
        glass.materialName = "FireplaceB_GlassStove";
        glass.decalType = DecalType::GLASS;

		m_meshNodes.Init(id, "FireplaceBrick", meshNodeCreateInfoSet);

		m_meshNodes.SetMaterialByMeshName("FireplaceBrick_Floor", "FireplaceB_Floor");
		m_meshNodes.SetMaterialByMeshName("FireplaceBrick_Stove", "FireplaceB_Stove");
		m_meshNodes.SetMaterialByMeshName("FireplaceBrick_StoveDoor", "FireplaceB_Stove");
		m_meshNodes.SetMaterialByMeshName("FireplaceBrick_StoveHandle", "FireplaceB_Stove");
		m_meshNodes.SetMaterialByMeshName("FireplaceBrick_StoveWindSlide", "FireplaceB_Stove");
		m_meshNodes.SetMaterialByMeshName("FireplaceBrick_StoveGlass", "FireplaceB_GlassStove");
		m_meshNodes.SetMaterialByMeshName("FireplaceBrick_Tool0", "FireplaceB_Tools");
		m_meshNodes.SetMaterialByMeshName("FireplaceBrick_Tool1", "FireplaceB_Tools");
		m_meshNodes.SetMaterialByMeshName("FireplaceBrick_Tool2", "FireplaceB_Tools");
		m_meshNodes.SetMaterialByMeshName("FireplaceBrick_WallFront", "FireplaceB_BrickWall2");
		m_meshNodes.SetMaterialByMeshName("FireplaceBrick_WallMain", "FireplaceB_BrickWall1");
		m_meshNodes.SetMaterialByMeshName("FireplaceBrick_WallExtended", "FireplaceB_BrickWall1");

		m_meshNodes.SetBlendingModeByMeshName("FireplaceBrick_StoveGlass", BlendingMode::GLASS);

        m_useFireClipHeight = true;
	}

    m_wallWidth = 0.766488f * 2.0f;
    m_wallDepth = 0.425;
    m_wallDepth = 0.450083f;

    glm::vec3 boundsMin = glm::vec3(-0.1f, 0.0f, -m_wallWidth * 0.5f);
    glm::vec3 boundsMax = glm::vec3(m_wallDepth, 2.7f, m_wallWidth * 0.5f);
    m_wallsAabb = AABB(boundsMin, boundsMax);
}

void Fireplace::UpdateWorldMatrix() {
    m_worldMatrix = m_transform.to_mat4();

    glm::vec3 localForward = glm::vec3(1.0, 0.0, 0.0f);
    glm::vec3 localRight = glm::vec3(0.0, 0.0, 1.0f);

    m_worldForward = m_worldMatrix * glm::vec4(localForward, 0.0f);
    m_worldRight = m_worldMatrix * glm::vec4(localRight, 0.0f);

    // Fire
    m_firePosition = m_transform.position;
    m_firePosition.y += 0.3f;
    m_firePosition += m_worldForward * 0.475f;

    ConfigureFire();

    // Remove the old light if there was one
    if (m_lightId != 0) {
        LegacyWorld::RemoveObject(m_lightId);
    }

    LightCreateInfo lightCreateInfo;
    lightCreateInfo.position = m_firePosition;
    lightCreateInfo.type = LightType::FIREPLACE_FIRE;
    lightCreateInfo.saveToFile = false;
    lightCreateInfo.radius = 2.75f;

    m_lightId = LegacyWorld::AddLight(lightCreateInfo, SpawnOffset());

    if (Light* light = LegacyWorld::GetLightByObjectId(m_lightId)) {
        light->m_doFlicker = true;
    }
}

void Fireplace::ConfigureFire() {
    SpriteSheetObjectCreateInfo createInfo;
    createInfo.position = m_firePosition;
    createInfo.rotation = glm::vec3(0.0f, HELL_PI * 0.5f, 0.0f);
    createInfo.scale = glm::vec3(1.0f);
    createInfo.uvOffset = glm::vec2(0.0f, 0.75f);
    createInfo.loop = true;
    createInfo.billboard = true;
    createInfo.renderingEnabled = true;
    createInfo.animationSpeed = 30.0f;
    createInfo.textureName = "FireplaceFire_8x8";

    m_fireSpriteSheetObject = SpriteSheetObject(createInfo);
}

void Fireplace::SetPosition(const glm::vec3& position) {
    m_createInfo.position = position;
    m_transform.position = position;
    UpdateWorldMatrix();
}

void Fireplace::SetPositionX(float x) {
    SetPosition(glm::vec3(x, GetPosition().y, GetPosition().z));
}

void Fireplace::SetPositionY(float y) {
    SetPosition(glm::vec3(GetPosition().x, y, GetPosition().z));
}

void Fireplace::SetPositionZ(float z) {
    SetPosition(glm::vec3(GetPosition().x, GetPosition().y, z));
}

void Fireplace::SetRotation(const glm::vec3& rotation) {
    m_createInfo.rotation = rotation;
    m_transform.rotation = rotation;
    UpdateWorldMatrix();
}

void Fireplace::Update(float deltaTime) {
    m_meshNodes.Update(m_worldMatrix);
    m_fireSpriteSheetObject.Update(deltaTime);

    //DebugDraw::DrawPoint(m_firePosition, RED);

    //if (const AABB* aabb = m_meshNodes.GetWorldSpaceAabbByMeshName("FireBounds")) {
    //    AABB aabbb(aabb->GetBoundsMin(), aabb->GetBoundsMax());
    //    DebugDraw::DrawAABB(aabbb, YELLOW);
    //}
    //
    //if (const AABB* aabb = m_meshNodes.GetWorldSpaceAabbByMeshName("FireBounds")) {
    //    m_fireSpriteSheetObject.SetAABBBounds(*aabb);
    //}

    //DebugDraw::DrawAABB(m_wallsAabb, WHITE, m_worldMatrix);

    //glm::vec3 center = m_worldMatrix[3];
    //glm::vec3 forwardPoint = center + m_worldForward;
    //glm::vec3 rightPoint = center + m_worldRight;
    //
    //DebugDraw::DrawLine(center, forwardPoint, BLUE);
    //DebugDraw::DrawLine(center, rightPoint, YELLOW);

}

void Fireplace::CleanUp() {
    m_meshNodes.CleanUp();

    LegacyWorld::RemoveObject(m_lightId);
}
}
