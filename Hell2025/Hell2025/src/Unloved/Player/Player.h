#pragma once
#include "PlayerTypes.h"

#include "Hell/BVH/Types.h"
#include "Hell/Math/AABB.h"
#include "Hell/Math/Transform.h"
#include "Hell/Math/VecXZ.h"
#include "Hell/Physics/Physics.h"

#include "Unloved/Common/Types.h"

#include "Unloved/Camera/Camera.h"
#include "Unloved/Camera/Frustum.h"
#include "Unloved/Inventory/Inventory.h"
#include "Unloved/Objects/Renderables/AnimatedGameObject.h"
#include "Unloved/Objects/Renderables/SpriteSheetObject.h"
#include "Unloved/Systems/TypeWriter/TypeWriter.h"

namespace Unloved {

struct Player {

public:
    void SubmitP90MagsRenderItems();

	void TriggerHealVignette();
	void TriggerDamageVignette();
	void TriggerVignette(const glm::vec3& color, float duration);
	void UpdateVignette(float deltaTime);

	const glm::vec3& GetVignetteColor() const { return m_vignetteColor; }
	float GetVignetteDuration() const         { return m_vignetteDuration; }
	float GetVignetteTimer() const            { return m_vignetteTimer; }
	float GetVignettIntensityScalar() const   { return m_vignetteIntensityScalar; }

private:
	glm::vec3 m_vignetteColor = glm::vec3(0);
	float m_vignetteDuration = 0.0f;
	float m_vignetteTimer = 0.0f;
    float m_vignetteIntensityScalar = 0.0f;


public:

    void Init(uint64_t playerId, const glm::vec3& position, const glm::vec3& rotation, int32_t viewportIndex);
    void BeginFrame();

    void Update(float deltaTime);
    void UpdateCharacterModelHacks();
    void PostWorldUpdate();

    void Respawn();
    void EnableControl();
    void DisableControl();
    void InitRagdoll();
    void InitCharacterModel();
    void GiveDamage(int damage, uint64_t enemyId);
    void CreateCharacterController(const glm::vec3& position);
    //void MoveCharacterController(glm::vec3 displacement);
    const bool IsAwaitingSpawn();
    const bool HasControl();
    const bool IsLocal() const;
    const bool IsOnline() const;
    const glm::vec3 GetFootPosition() const;
    const glm::mat4& GetViewMatrix() const;
    const glm::mat4& GetInverseViewMatrix() const;
    const glm::vec3& GetCameraPosition() const;
    const glm::vec3& GetCameraRotation() const;
    const glm::vec3& GetCameraForward() const;
    const glm::vec3& GetCameraRight() const;
    const glm::vec3& GetCameraUp() const;
    const int32_t GetViewportIndex() const;
    AnimatedGameObject* GetCharacterModelAnimatedGameObject();
    AnimatedGameObject* GetViewWeaponAnimatedGameObject();
    Unloved::Camera& GetCamera();


public:
    glm::vec3 GetViewportColorTint();
    float GetViewportContrast();
    float m_timeSinceDeath = 0.0f;
    bool RespawnAllowed();

    int m_killCount = 0;

    void UpdateWeaponAttachments();

    void UpdateViewWeaponVisibility();
    void UpdateCursorRays();
    void UpdateInteract();
    void UpdateCamera(float deltaTime);
    void UpdateViewWeapon(float deltaTime);
    void UpdateMovement(float deltaTime);
    void UpdateUI(float deltaTime);
    void UpdateSpriteSheets(float deltaTime);
    void UpdateHeadBob(float deltaTime);
    void UpdateBreatheBob(float deltaTime);
    void UpdateAudio(float deltaTime);
    void UpdateFlashlight(float deltaTime);
    void UpdateFlashlightFrustum();
    void UpdateAnimatedGameObjects(float deltaTime);
    void UpdatePlayingPiano(float deltaTime);
    void UpdateWeaponSlide();

    // Remove me when you can
    void HideKnifeMesh();
    void HideGlockMesh();
    void HideShotgunMesh();
    void HideAKS74UMesh();
    // Remove me when you can

    bool IsFacingClosestMermaid();
    float DotToClosestToMermaid();
    void GiveCash(int amount);
    void SubtractCash(int amount);


    void Kill(bool wasHeadShot);
    glm::mat4 m_deathCamViewMatrix = glm::mat4(1.0f);

    // Ragdoll
    Ragdoll* GetRagdoll();
    uint64_t GetRagdollId();

    // Ladder
    uint64_t m_ladderIdOverlapIndexFeet = 0;
    uint64_t m_ladderIdOverlapIndexEyes = 0;
    float m_overlappedLadderHeight = 0.0f;
    float m_ladderFootstepAudioTimer = 0;
    float m_ladderFootstepAudioLoopLength = 0.5;
    void UpdateLadderIds();
    void UpdateLadderMovement(float deltaTime);
    bool IsOverlappingLadder();
    int GetCash() { return m_cash; }

    MeshNodes m_supressor;
    MeshNodes m_redDot;
    MeshNodes m_p90MagMeshNodes;

    // Weapon shit
    int GetCurrentWeaponMagAmmo();
    int GetCurrentWeaponTotalAmmo();
    bool IsShellInShotgunChamber();
    void UpdateWeaponLogic(float deltaTime);
    void GiveDefaultLoadout();
    void GiveWeapon(const std::string& name);
    void GiveAmmo(const std::string& name, int amount);
    void GiveSight(const std::string& weaponName);
    void GiveSilencer(const std::string& weaponName);
    void SwitchWeapon(const std::string& name, WeaponAction weaponAction);
    void DropWeapons();
    void DropItems();

    const std::string& GetSelectedWeaponName();

    WeaponAction& GetWeaponAction();
    WeaponInfo* GetCurrentWeaponInfo();
    WeaponState* GetWeaponStateByName(const std::string &name);
    WeaponState* GetCurrentWeaponState();
    AmmoState* GetAmmoStateByName(const std::string& name);
    AmmoState* GetCurrentAmmoState();
    AmmoInfo* GetCurrentAmmoInfo();

    void AddHorizontalImpulse(glm::vec3 direction, float force);
    void AddVerticalImpulse(float force);
    void SimulateVelocityMovement(float deltaTime, const glm::vec3& inputDirection, float maxSpeed, float accelerationStrength, float damping, bool applyGravity, float gravity);
    //glm::vec3 m_velocity = glm::vec3(0.0f);
    glm::vec3 m_movementDirection = glm::vec3(0.0f); // can be zero
    float m_acceleration = 0.0f;

    glm::ivec2 GetViewportCenter();
    void CheckForMeleeHit();
    float GetTargetWalkingSpeed();

    TypeWriter m_typeWriter;

    // Water
    WaterState m_waterState;
    bool FeetEnteredUnderwater();
    bool FeetExitedUnderwater();
    bool CameraEnteredUnderwater();
    bool CameraExitedUnderwater();
    bool IsSwimming();
    bool IsWading();
    bool CameraIsUnderwater();
    bool FeetBelowWater();
    bool StartedSwimming();
    bool StoppedSwimming();
    bool StartingWading();
    bool StoppedWading();
    float GetFeetDistanceBeneathWater();

    // Piano
    uint64_t m_pianoId = 0;
    bool m_isPlayingPiano = false;
    void SitAtPiano(uint64_t pianoId);

    // State queries
    bool IsMoving();
    bool IsGrounded();
    bool IsCrouching();
    //bool IsAlive();
    //bool IsOverlappingLadder();
    //bool IsAtShop();
    bool IsPlayingPiano();
    //bool IsUnderWater();

    // Input
    int32_t GetKeyboardIndex();
    int32_t GetMouseIndex();
    void SetKeyboardIndex(int32_t index);
    void SetMouseIndex(int32_t index);
    void ConsumeInteract();
    bool PressingWalkForward();
    bool PressingWalkBackward();
    bool PressingWalkLeft();
    bool PressingWalkRight();
    bool PressingCrouch();
    bool PressedUp(bool allowKeyRepeat = false);
    bool PressedDown(bool allowKeyRepeat = false);
    bool PressedLeft(bool allowKeyRepeat = false);
    bool PressedRight(bool allowKeyRepeat = false);
    bool PressedWalkForward(bool allowKeyRepeat = false);
    bool PressedWalkBackward(bool allowKeyRepeat = false);
    bool PressedWalkLeft(bool allowKeyRepeat = false);
    bool PressedWalkRight(bool allowKeyRepeat = false);
    bool PressedInteract();
    bool PressingInteract();
    bool PressedReload();
    bool PressedFire();
    bool PressingFire();
    bool PressingJump();
    bool PressingRun();
    bool PressedRun();
    bool PressedCrouch();
    bool PressedWeaponMiscFunction();
    bool PressedNextWeapon();
    bool PressingADS();
    bool PressedADS();
    bool PressedEscape();
    bool PressedMelee();
    bool PressedFlashlight();
    bool PressedToggleInventory();
	bool PressedInventoryExamine();
	bool PressedInventoryDiscard();
    bool HasWeapon(const std::string& weaponName);

    bool PurchaseItem(const std::string& itemName);
    void DiscardItem(const std::string& itemName);

    const InventoryState& GetInvetoryState() { return m_inventory.GetInventoryState(); }
    Unloved::Inventory& GetInventory() { return m_inventory; }

    void DisplayInfoText(const std::string& text);
    std::string m_infoText = "";
    float m_infoTextTimer = 0;

    int m_health = 100;

    // Dev keys
    bool PressedFullscreen();
    bool PressedOne();
    bool PressedTwo();
    bool PressedThree();
    bool PressedFour();

    const float GetFov();
    const void SetName(const std::string& name);

    glm::mat4 GetViewWeaponBoneWorldMatrix(const std::string& boneName);

    Hell::ivecXZ GetChunkPos() { return m_chunkPos; }

    bool InventoryIsOpen();
    bool InventoryIsClosed();
    bool ShopInventoryIsOpen();
    bool ShopInventoryIsClosed();

    uint64_t GetClosestMirrorId();

    bool InteractFound()                        { return m_interactFound; }
    uint64_t GetInteractObjectId()              { return m_interactObjectId; }
    Unloved::Frustum& GetFlashlightFrustum()             { return m_flashlightFrustum; }
    const glm::vec3& GetInteractHitPosition()   { return m_interactHitPosition; }
    const std::string& GetName() const          { return m_name; }


    Unloved::Inventory m_inventory;
    Unloved::Inventory m_shopInventory;

private:
    std::string m_name = "PLAYER_NAME";

    // Interact
    PhysXRayResult m_physXRayResult;
    BvhRayResult m_bvhRayResult;
    uint64_t m_interactObjectId = 0;
    uint32_t m_interactOpenableId = 0;
    uint32_t m_interactCustomId = 0;
    glm::vec3 m_interactHitPosition = glm::vec3(0.0f);
    bool m_rayHitFound = false;
    bool m_interactFound = false;
    int32_t m_deathCount = 0;
    int32_t m_respawnCount = 0;

    //glm::vec3 m_position = glm::vec3(0.0f);
    Hell::ivecXZ m_chunkPos;

    // Interact

    Unloved::Frustum m_flashlightFrustum;


    // Flashlight
    glm::vec3 m_flashlightPosition;
    glm::vec3 m_flashlightDirection;
    glm::mat4 m_flashlightProjectionView;
    bool m_flashlightOn = false;
    float m_flashLightModifier = 0.0f;

    // Headbob
    float m_bobOffsetX = 0.0f;
    float m_bobOffsetY = 0.0f;
    float m_headBobTime = 0.0f;
    float m_breatheBobTime = 0.0f;
    glm::vec3 m_headBob = glm::vec3(0.0f);
    glm::vec3 m_breatheBob = glm::vec3(0.0f);

    // Audio
    bool m_footstepPlayed = false;

    // States
    bool m_moving = false;
    bool m_crouching = false;
    bool m_grounded = true;
    bool m_groundedLastFrame;
    bool m_feetAboveHeightField = false;
    bool m_running = false;

    // Speed
    float m_currentSpeed = 0.0f;
    float m_walkingSpeed = 4.25f;
    float m_runningSpeed = 2.5f;
    float m_crouchingSpeed = 2.325f;
    float m_swimmingSpeed = 3.25f;
    float m_crouchDownSpeed = 17.5f;

    // Heights
    float m_realViewHeightStanding = 1.65f;  // are these used?
    float m_realViewHeightCrouching = 1.15f; // are these used?
    float m_viewHeightStanding = m_realViewHeightStanding;
    float m_viewHeightCrouching = m_realViewHeightCrouching;
    float m_currentViewHeight = m_viewHeightStanding;

    // Misc
    int m_revolverReloadIterations = 0;
    int m_currentWeaponIndex = 0;
    bool m_controlEnabled = true;
    bool m_awaitingSpawn = true;
    bool m_firedThisFrame = false;
    bool _needsAmmoReloaded = false;
    bool m_revolverNeedsCocking = false;
    bool _glockSlideNeedsToBeOut = false;
    bool _needsShotgunFirstShellAdded = false;
    bool _needsShotgunSecondShellAdded = false;
    float m_mouseSensitivity = 0.002f;
    float m_cameraZoom = 1.0f;
    float m_accuracyModifer = 0;
    int32_t m_viewportIndex = 0;
    Unloved::Camera m_camera;
    InputType m_inputType = KEYBOARD_AND_MOUSE;
    PlayerControls m_controls;
    ShellEjectionState m_shellEjectionState;
    WeaponAction m_weaponAction = DRAW_BEGIN;

    //std::vector<WeaponState> m_weaponStates;
    //std::vector<AmmoState> m_ammoStates;

    float m_waterImpactVelocity = 0;


    float _muzzleFlashRotation = 0;
    glm::vec2 _weaponSwayFactor = glm::vec2(0);
    glm::vec3 _weaponSwayTargetPos = glm::vec3(0);
    float _muzzleFlashCounter = 0;

    //uint32_t _interactFlags;
    //uint32_t _bulletFlags;
    int m_playerIndex;


    public:
        void UseItem(const std::string& itemName);
        bool CanUseItem(const std::string& itemName);

        float _muzzleFlashTimer = 0;

        Hell::Transform m_weaponSwayTransform;

        glm::mat4 m_weaponSwayMatrix = glm::mat4(1);
        int m_mouseIndex = -1;
        int m_keyboardIndex = -1;

        glm::mat4 GetItemExamineModelMatrix() { return m_inventory.GetItemExamineModelMatrix(); }


        float m_weaponAudioFrequency = 1.0f;

        float GetWeaponAudioFrequency();

        void NextWeapon();
        void UpdateMeleeLogic(float deltaTime);
        void UpdateGunLogic(float deltaTime);
        void UpdateShotgunGunLogic(float deltaTime);

        WeaponType GetCurrentWeaponType();
        WeaponAction GetCurrentWeaponAction();

        // Melee
        void FireMelee();
        bool CanFireMelee();

        // Shop
        bool m_isInShop = false;
        void EnterShop();
        void LeaveShop();
        void UpdateShop(float deltaTime);
        bool IsInShop() { return m_isInShop; }

        // Gun
        void FireGun();
        void ReloadGun();
        void EnterADS();
        void LeaveADS();
        void UpdateGunReloadLogic();
        void UpdateSlideLogic();
        void UpdateADSLogic(float deltaTime);
        bool CanFireGun();
        bool CanReloadGun();
        bool CanEnterADS();
        bool CanLeaveADS();
        void SpawnBullet(float variance);
        void SpawnUnderWaterBullet(float variance);
        bool IsInADS();

        // Shotgun
        void FireShotgun();
        void ShotgunMelee();
        void DryFireShotgun();
        void ReloadShotgun();
        void UnloadShotgun();
        void ToggleAutoShotgun();
        void UpdatePumpAudio();
        void UpdateShotgunReloadLogic();
        void UpdateShotgunUnloadLogic();
        bool CanToggleShotgunAuto();
        bool CanFireShotgun();
        bool CanDryFireShotgun();
        bool CanReloadShotgun();
        bool CanUnloadShotgun();
        bool ShotgunRequiresPump();

        // Casings and Muzzle flash
        void SpawnMuzzleFlash(float speed, float scale);
        void SpawnCasing();

        bool ViewModelAnimationsCompleted();
        bool ViewportIsVisible();

        // Physics
        void SetFootPosition(glm::vec3 position);
        PxShape* GetCharacterControllerShape();
        PxRigidDynamic* GetCharacterControllerActor();

        glm::mat4& GetAnimatedCameraMatrix();
        glm::mat4& GetCSMViewMatrix();

        glm::mat4 m_animatedCameraMatrix;
        glm::mat4 m_csmViewMatrix;

        float m_walkTiltTimer = 0.0f;

        float m_yVelocity = 0;
        int m_cash = 0;

        bool m_pistolAwaitingFireReleased = false;

        float m_weaponSwayX = 0;
        float m_weaponSwayY = 0;

        // Swimming
        float m_swimVerticalAcceleration = 0.0f;
        float m_smoothedWaterY;
        //bool m_underWater = false;

        struct MeleeBulletWaveState {
            bool active = false;
            float startTime = 0.0f;
            float maxTime = 0.0f;
            float time = 0.0f;
            float intervalDuration = 0.0f;
            float intervalCounter = 0.0f;
            int spawnCountThisWave = 0;
        } m_meleeBulletWaveState;

        void UpdateMelleBulletWave(float deltaTime);
		void BeginMeleeBulletWave();
		void CalculateMuzzleFlashSpawnPosition();

        glm::vec3& GetMuzzleFlashSpawnPosition() { return m_muzzleFlashSpawnPosition; }

    private:
        bool m_alive = true;

        glm::vec3 m_muzzleFlashSpawnPosition;

        uint64_t m_viewWeaponAnimatedGameObjectId;
        uint64_t m_characterModelAnimatedGameObjectId;
        SpriteSheetObject m_muzzleFlash;

      //  CharacterController

        //PxController* m_characterController = NULL;

        std::vector<SpriteSheetRenderItem> m_spriteSheetRenderItems;
        AABB m_characterControllerAABB;

        void UpdateWalkingMovement(float deltaTime);
        void UpdateSwimmingMovement(float deltaTime);

        uint64_t m_playerId = 0;
        uint64_t m_characterControllerId = 0;

    public:
        const uint64_t GetPlayerId()                                            { return m_playerId; }
        const uint64_t GetcharacterControllerId()                               { return m_characterControllerId; }
        const bool IsAlive()                                                    { return m_alive; }
        const bool IsDead()                                                     { return !m_alive; }
        const bool IsRunning()                                                  { return m_running; }
        const bool AreFeetAboveHeightField()                                    { return m_feetAboveHeightField; }
        const std::vector<SpriteSheetRenderItem>& GetSpriteSheetRenderItems()   { return m_spriteSheetRenderItems; }
        const glm::vec3 GetFlashlightPosition()                                 { return m_flashlightPosition; }
        const glm::vec3 GetFlashlightDirection()                                { return m_flashlightDirection; };
        const glm::mat4 GetFlashlightProjectionView()                           { return m_flashlightProjectionView; };
        const float GetFlashLightModifer()                                      { return m_flashLightModifier; }
        const AABB& GetCharacterControllerAABB()                                { return m_characterControllerAABB; }
};

} // namespace Unloved
