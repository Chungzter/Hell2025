#include "Player.h"

#include "Hell/Audio.h"
#include "Hell/Backend/BackEnd.h"
#include "Hell/Common/Constants.h"
#include "Hell/Common/Random.h"
#include "Hell/Logging.h"

#include "Unloved/Common/Constants.h"
#include "Unloved/Render/Renderer.h"
#include "Legacy/World/LegacyWorld.h"

#include "Unloved/Bible/Bible.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Editor/Editor.h"
#include "Unloved/Systems/Ocean/Ocean.h"
#include "Unloved/Viewport/ViewportManager.h"
#include "Unloved/World/World.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <vector>

namespace Audio = Hell::Audio;

namespace Unloved {

void Player::Init(uint64_t playerId, const glm::vec3& position, const glm::vec3& rotation, int32_t viewportIndex) {
    m_playerId = playerId;
    m_flashlightSpotLightId = World::AddSpotLight(playerId, viewportIndex);

    m_camera.SetPosition(position + glm::vec3(0.0f, m_viewHeightStanding, 0.0f));
    m_camera.SetEulerRotation(rotation);
    m_viewportIndex = viewportIndex;

    m_inventory.SetLocalPlayerIndex(m_viewportIndex);
    m_shopInventory.SetLocalPlayerIndex(m_viewportIndex);

    m_characterModelAnimatedGameObjectId= LegacyWorld::CreateAnimatedGameObject();
    m_viewWeaponAnimatedGameObjectId = LegacyWorld::CreateAnimatedGameObject();

    AnimatedGameObject* viewWeapon = GetViewWeaponAnimatedGameObject();
    viewWeapon->SetExclusiveViewportIndex(viewportIndex);
    viewWeapon->DisableShadows();
    viewWeapon->SetExcludeFromVulkanTLAS(true);

    AnimatedGameObject* characterModel = GetCharacterModelAnimatedGameObject();

    SpriteSheetObjectCreateInfo createInfo;
    createInfo.textureName = "MuzzleFlash_4x5";
    createInfo.loop = false;
    createInfo.billboard = true;
    createInfo.renderingEnabled = false;
    m_muzzleFlash.Init(createInfo);

    CreateCharacterController(position);
    InitCharacterModel();
    InitRagdoll();
}

void Player::BeginFrame() {
    m_interactFound = false;
    m_interactObjectId = 0;
    m_interactOpenableId = 0;
}

void Player::EnterShop() {
    m_isInShop = true;
    m_shopInventory.OpenAsShop();
    m_inventory.CloseInventory();

    const std::string& text = Bible::MermaidShopGreeting();
    m_typeWriter.DisplayText(text);

    m_flashlightOn = true;
    Audio::PlayAudio(AUDIO_SELECT, 1.00f);

    ConsumeInteract();

    //InputMulti::ClearKeyStates();
}

void Player::LeaveShop() {
    m_isInShop = false;
    m_inventory.CloseInventory();
    m_shopInventory.CloseInventory();
}

static float WrapPi(float a) {
    const float twoPi = 6.28318530718f;
    a = std::fmod(a + 3.14159265359f, twoPi);
    if (a < 0.0f) a += twoPi;
    return a - 3.14159265359f;
}

static float LerpAngle(float a, float b, float t) {
    float delta = WrapPi(b - a);
    return a + delta * t;
}

void Player::UpdateShop(float deltaTime) {
    glm::vec3 targetPosition = glm::vec3(13.06f, 28.68f, 36.78);
    glm::vec3 targetCamEuler = glm::vec3(-0.08f, -1.65f, 0.0f);

    if (Unloved::Session::GetSplitscreenMode() == SplitscreenMode::TWO_PLAYER) {
        targetPosition = glm::vec3(12.93f, 28.61f, 36.80);
        targetCamEuler = glm::vec3(-0.00f, -2.05f, 0.0f);
    }


    glm::vec3 currentPosition = GetFootPosition();
    glm::vec3 currentCamEuler = m_camera.GetEulerRotation();

    float positionInterpolationSpeed = 25.0f;
    float rotationInterpolationSpeed = 50.0f;

    float positionT = 1.0f - std::exp(-positionInterpolationSpeed * deltaTime);
    float rotationT = 1.0f - std::exp(-rotationInterpolationSpeed * deltaTime);

    glm::vec3 newPosition = glm::mix(currentPosition, targetPosition, positionT);

    glm::vec3 newCamEuler = currentCamEuler;
    newCamEuler.x = LerpAngle(currentCamEuler.x, targetCamEuler.x, rotationT); // pitch
    newCamEuler.y = LerpAngle(currentCamEuler.y, targetCamEuler.y, rotationT); // yaw
    newCamEuler.z = 0.0f;

    SetFootPosition(newPosition);
    m_camera.SetEulerRotation(newCamEuler);
}

void Player::DiscardItem(const std::string& itemName) {
    ItemInfo* itemInfo = Bible::GetItemInfoByName(itemName);
    if (!itemInfo) {
        Logging::Error() << "Player::DiscardItem(..) failed to drop item '" << itemName << "'\n";
        return;
    }

    glm::vec3 spawnPosition = GetCameraPosition() + (GetCameraForward() * 0.5f);

	PickUpCreateInfo createInfo;
	createInfo.position = spawnPosition;
	createInfo.rotation.x = Hell::Random::Float(-HELL_PI, HELL_PI);
	createInfo.rotation.y = Hell::Random::Float(-HELL_PI, HELL_PI);
	createInfo.rotation.z = Hell::Random::Float(-HELL_PI, HELL_PI);
	createInfo.name = itemName;
	createInfo.saveToFile = false;
	createInfo.disablePhysicsAtSpawn = false;
	createInfo.respawn = false;
	createInfo.type = Bible::GetItemType(itemName);

	Unloved::World::AddPickUp(createInfo);
}

bool Player::PurchaseItem(const std::string& itemName) {
    ItemInfo* itemInfo = Bible::GetItemInfoByName(itemName);
    if (!itemInfo) return false;

    const ItemType& itemType = itemInfo->GetType();
    const int itemCost = itemInfo->GetCost();

    // Is it a weapon that you already have
    if (itemInfo->GetType() == ItemType::WEAPON && HasWeapon(itemName)) {
		m_typeWriter.DisplayText("You already got one Darlin'.");
		Audio::PlayAudio("ShopDenied.wav", 1.0f);
        return false;
    }

    // Can you afford it?
	if (m_cash >= itemCost) {
		if (itemType == ItemType::WEAPON) {
			m_inventory.GiveWeapon(itemName);
			m_inventory.GiveAmmo(itemName, itemCost);
			SwitchWeapon(itemName, DRAW_BEGIN);
			SubtractCash(itemCost);
		}
		if (itemType == ItemType::HEAL) {
			m_inventory.AddInventoryItem(itemName);
			SubtractCash(itemCost);
		}
        else {
            Logging::ToDo() << "Bro, Player::PurchaseItem(...) is missing this item type's implementation";
        }

        m_typeWriter.DisplayText(Bible::MermaidShopWeaponPurchaseConfirmationText());
        Audio::PlayAudio("ShopPurchase2.wav", 1.0f);
        LeaveShop();

        return true;
    }

    // Denied coz you couldn't afford it
	m_typeWriter.DisplayText(Bible::MermaidShopFailedPurchaseText());
	Audio::PlayAudio("ShopDenied.wav", 1.0f);
	return false;
}

void Player::Respawn() {
    m_inventory.Init();
    m_shopInventory.Init();
    m_health = 100;
    m_isInShop = false;
    m_alive = true;
    m_flashlightOn = false;
    m_awaitingSpawn = false;

    // Get random spawn point
    const SpawnPoint& spawnPoint = Session::GetGameMode() == GameMode::DEATH_MATCH ? Session::GetRandomDeathmatchSpawnPoint() : Session::GetRandomCampaignSpawnPoint();
    glm::vec3 spawnPosition = spawnPoint.GetPosition() - glm::vec3(0.0f, 1.60, 0.0f);

    // Set position and camera rotation to spawn point
    SetFootPosition(spawnPosition);
    m_camera.SetEulerRotation(spawnPoint.GetCameraEuler());

    //if (m_viewportIndex == 0) {
    //    SetFootPosition(glm::vec3(36.18, 31, 37.26));
    //    m_camera.SetEulerRotation(glm::vec3(-0.15, -0.02, 0));
    //}

   // else {
   //     if (m_viewportIndex == 1) {
   //         SetFootPosition(glm::vec3(12.5f, 30.6f, 45.5f));
   //         m_camera.SetEulerRotation(glm::vec3(0, 0, 0));
   //     }
   //     if (m_viewportIndex == 2) {
   //         SetFootPosition(glm::vec3(12.5f, 30.6f, 55.5f));
   //         m_camera.SetEulerRotation(glm::vec3(0, 0, 0));
   //     }
   //     if (m_viewportIndex == 3) {
   //         SetFootPosition(glm::vec3(12.5f, 30.6f, 605.5f));
   //         m_camera.SetEulerRotation(glm::vec3(0, 0, 0));
   //     }
   // }

    GiveDefaultLoadout();
    SwitchWeapon("Glock", WeaponAction::DRAW_BEGIN);

    m_camera.Update();
    m_flashlightDirection = m_camera.GetForward();

    if (IsInShop()) {
        m_flashlightDirection += glm::vec3(0.0f, -0.1f, 0.0f);
        m_flashlightDirection = glm::normalize(m_flashlightDirection);
    }

    // Are you inside? Turn flash light on
    float maxRayDistance = 2000;
    glm::vec3 rayOrigin = GetFootPosition() + glm::vec3(0, 2, 0);
    glm::vec3 rayDir = glm::vec3(0, 1, 0);
    PhysXRayResult physxRayResult = Hell::Physics::CastPhysXRay(rayOrigin, rayDir, maxRayDistance, true, std::vector<physx::PxRigidActor*>());
    if (!physxRayResult.hitFound) {
        m_flashlightOn = true;
    }

    // Make character model animated again (aka not ragdoll)
    AnimatedGameObject* characterModel = GetCharacterModelAnimatedGameObject();
    if (characterModel) {
        characterModel->SetAnimationModeToAnimated();
    }

    if (Ragdoll* Ragdoll = GetRagdoll()) {
        Ragdoll->DisableSimulation();
    }

    m_respawnCount++;

    //Logging::Debug() << "Spawned player " << m_viewportIndex << " at " << spawnPosition;
}


void Player::EnableControl() {
    m_controlEnabled = true;
}
void Player::DisableControl() {
    m_controlEnabled = false;
}

const bool Player::IsAwaitingSpawn() {
    return m_awaitingSpawn;
}

const bool Player::HasControl() {
    return m_controlEnabled;
}

const bool Player::IsLocal() const {
    return m_viewportIndex != -1;
}

const bool Player::IsOnline() const {
    return m_viewportIndex == -1;
}

const glm::mat4& Player::GetViewMatrix() const {
    return m_camera.GetViewMatrix();
}

const glm::mat4& Player::GetInverseViewMatrix() const {
    return m_camera.GetInverseViewMatrix();
}

const glm::vec3& Player::GetCameraPosition() const {
    return m_camera.GetPosition();
}

const glm::vec3& Player::GetCameraRotation() const {
    return m_camera.GetEulerRotation();
}

const glm::vec3& Player::GetCameraForward() const {
    return m_camera.GetForward();
}

const glm::vec3& Player::GetCameraRight() const {
    return m_camera.GetRight();
}

const glm::vec3& Player::GetCameraUp() const {
    return m_camera.GetUp();
}

const int32_t Player::GetViewportIndex() const {
    return m_viewportIndex;
}

const glm::vec3 Player::GetFootPosition() const {
    // FIND ME
    PxController* m_characterController = nullptr;
    CharacterController* characterControler = Hell::Physics::GetCharacterControllerById(m_characterControllerId);
    if (characterControler) {
        m_characterController = characterControler->GetPxController();
        PxExtendedVec3 pxPos = m_characterController->getFootPosition();
        return glm::vec3(
            static_cast<float>(pxPos.x),
            static_cast<float>(pxPos.y),
            static_cast<float>(pxPos.z)
        );
    }
    else {
        return glm::vec3(0.0f, 0.0f, 0.0f);
    }
}

Unloved::Camera& Player::GetCamera() {
    return m_camera;
}

AnimatedGameObject* Player::GetCharacterModelAnimatedGameObject() {
    return Unloved::World::GetAnimatedGameObjectByObjectId(m_characterModelAnimatedGameObjectId);
}

AnimatedGameObject* Player::GetViewWeaponAnimatedGameObject() {
    return Unloved::World::GetAnimatedGameObjectByObjectId(m_viewWeaponAnimatedGameObjectId);
}

bool Player::ViewportIsVisible() {
    Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(m_viewportIndex);
    if (!viewport) {
        return false;
    }
    else {
        return viewport->IsVisible();
    }
}

bool Player::ViewModelAnimationsCompleted() {
    AnimatedGameObject* viewWeapon = GetViewWeaponAnimatedGameObject();
    if (!viewWeapon) {
        std::cout << "WARNING!!! Player::ViewModelAnimationsCompleted() failed coz viewWeapon was nullptr\n";
        return true;
    }
    return viewWeapon->IsAllAnimationsComplete();
}

float Player::GetWeaponAudioFrequency() {
    return m_weaponAudioFrequency;
}

glm::mat4& Player::GetAnimatedCameraMatrix() {
    return m_animatedCameraMatrix;
}

glm::mat4& Player::GetCSMViewMatrix() {
    return m_csmViewMatrix;
}

void Player::DisplayInfoText(const std::string& text) {
    m_infoTextTimer = 2.0f;
    m_infoText = text;
}

const float Player::GetFov() {
    return m_cameraZoom;
}

void Player::GiveDamage(int damage, uint64_t enemyId) {
    m_health -= damage;
    if (m_health <= 0) {
        m_health = 0;
        Kill(false);
    }
    else {
        TriggerDamageVignette();
    }
}

float Player::DotToClosestToMermaid() {
    if (Unloved::World::GetMermaids().empty()) return 0;

    Mermaid& mermaid = Unloved::World::GetMermaids()[0];
    return glm::dot(mermaid.GetWorldForward(), GetCameraForward());
}

void Player::GiveCash(int amount) {
    m_cash += amount;
}

void Player::SubtractCash(int amount) {
    m_cash -= amount;
}

bool Player::IsFacingClosestMermaid() {
    if (Unloved::World::GetMermaids().empty()) return false;

    Mermaid& mermaid = Unloved::World::GetMermaids()[0];

    const glm::vec3& cameraPositon = GetCameraPosition();
    const glm::vec3& cameraForward = GetCameraForward();

    glm::vec3 toCamera = cameraPositon - mermaid.GetPosition();
    glm::vec3 toMermaid = mermaid.GetPosition() - cameraPositon;

    bool cameraOnFrontSide = glm::dot(mermaid.GetWorldForward(), toMermaid) > 0.0f;
    bool mermaidInFrontOfCamera = glm::dot(cameraForward, toMermaid) > 0.0f;

    // HACCCCCCCCCK because cameraOnFrontSide doesn't evaluate to what you think it does
    float distanceToMermaid = glm::distance(cameraPositon, mermaid.GetPosition());
    if (distanceToMermaid > 2.0f) {
        return false;
    }

    return !(cameraOnFrontSide && mermaidInFrontOfCamera);
}

void Player::Kill(bool wasHeadShot) {
    if (m_alive) {
        m_flashlightOn = false;
        m_deathCount++;
        m_alive = false;
        m_inventory.CloseInventory();
        m_shopInventory.CloseInventory();

        if (AnimatedGameObject* characterModel = GetCharacterModelAnimatedGameObject()) {
            if (GetRagdoll()) {
                characterModel->SetAnimationModeToRagdoll();
            }
        }

        Audio::PlayAudio("Death0.wav", 1.0f);
        DropWeapons();
        DropItems();
        m_cash /= 2;

        // HACK
        for (int i = 0; i < Unloved::Session::GetLocalPlayerCount(); i++) {
            if (i != m_viewportIndex) {
                if (Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(i)) {
                    player->m_killCount++;
                    if (wasHeadShot) {
                        player->GiveCash(Bible::GetPlayerHeadShotCashReward());
                    }
                    else {
                        player->GiveCash(Bible::GetPlayerKillCashReward());
                    }
                }
            }
        }
    }
}

glm::vec3 Player::GetViewportColorTint() {
    glm::vec3 colorTint = glm::vec3(1, 1, 1);

    if (InventoryIsOpen() && m_inventory.GetInventoryState() == InventoryState::EXAMINE_ITEM) {
        colorTint = glm::vec3(0.325);
    }

    if (IsDead()) {
        colorTint.r = 2.0;
        colorTint.g = 0.2f;
        colorTint.b = 0.2f;

        float waitTime = 3;
        if (m_timeSinceDeath > waitTime) {
            float val = (m_timeSinceDeath - waitTime) * 10;
            colorTint.r -= val;
        }
    }

    //if (m_viewportIndex == 0) {
    //    std::cout << colorTint << "\n";
    //}

    return colorTint;
}

bool Player::HasWeapon(const std::string& weaponName) {
    return m_inventory.HasItem(weaponName);
}

const void Player::SetName(const std::string& name) {
    m_name = name;
}

bool Player::RespawnAllowed() {
    return IsDead() && m_timeSinceDeath > 3.25f;
}


float Player::GetViewportContrast() {
    if (IsAlive()) {
        return 1.0f;
    }
    else {
        return 1.1f;
    }
}

uint64_t Player::GetRagdollId() {
    AnimatedGameObject* characterModel = GetCharacterModelAnimatedGameObject();
    if (!characterModel) return 0;

    return characterModel->GetRagdollId();
}

Ragdoll* Player::GetRagdoll() {
    const uint64_t RagdollId = GetRagdollId();
    if (RagdollId == 0) return nullptr;

    return Hell::Physics::GetRagdollById(RagdollId);
}

bool Player::InventoryIsOpen() {
    return m_inventory.IsOpen();
}

bool Player::InventoryIsClosed() {
    return m_inventory.IsClosed();
}

bool Player::ShopInventoryIsOpen() {
    return m_shopInventory.IsOpen();
}

bool Player::ShopInventoryIsClosed() {
    return m_shopInventory.IsClosed();
}

void Player::TriggerHealVignette() {
	TriggerVignette(glm::vec3(0.0f, 0.2f, 0.0f), 0.4f);
}

void Player::TriggerDamageVignette() {
	TriggerVignette(glm::vec3(0.6f, 0.0f, 0.0f), 0.4f);
}

void Player::TriggerVignette(const glm::vec3& color, float duration) {
	m_vignetteColor = color;
	m_vignetteTimer = duration;
	m_vignetteDuration = duration;
}

} // namespace Unloved
