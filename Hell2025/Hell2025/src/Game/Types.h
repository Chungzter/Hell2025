#pragma once

#include <Game/Constants.h>
#include <Game/Enums.h>
#include <Hell/Types.h>
#include "Input/keycodes.h"

#include <limits>
#include <string>
#include <vector>

struct RenderItem {
    glm::mat4 modelMatrix = glm::mat4(1);
    glm::mat4 prevModelMatrix = glm::mat4(1);
    glm::mat4 inverseModelMatrix = glm::mat4(1);
    glm::vec4 aabbMin = glm::vec4(0);
    glm::vec4 aabbMax = glm::vec4(0);

    int32_t meshIndex = 0;
    int32_t baseColorTextureIndex = 0;
    int32_t normalMapTextureIndex = 0;
    int32_t rmaTextureIndex = 0;

    int32_t objectType = 0;
    int32_t woundMaskTexutreIndex = -1;
    int32_t exclusiveViewportIndex = -1;
    int32_t ignoredViewportIndex = -1;

    uint32_t objectIdUpperBit = 0;
    uint32_t objectIdLowerBit = 0;
    int32_t baseSkinnedVertex = 0;
    int32_t baseSkinningTransformIndex = 0;

    uint32_t openableId = 0;
    uint32_t customId = 0;
    int32_t skinned = 0;
    uint32_t shadowBit = 0;

    float emissiveR = 0.0f;
    float emissiveG = 0.0f;
    float emissiveB = 0.0f;
    int32_t emissiveTextureIndex = -1;

    uint32_t baseVertex = 0;
    uint32_t baseIndex = 0;
    uint32_t baseVertexWeight = 0;
    int blockScreenSpaceBloodDecals = 0;

    int32_t additionalTextureIndex0 = 0;
    int32_t additionalTextureIndex1 = 0;
    int32_t additionalTextureIndex2 = 0;
    int32_t additionalTextureIndex3 = 0;

    int32_t localMeshNodeIndex = 0;
    int32_t opacityTextureIndex = 0;
    int32_t meshId = 0;
    int32_t blendingMode = static_cast<int32_t>(BlendingMode::DEFAULT);

    float tintColorR = 1.0f;
    float tintColorG = 1.0f;
    float tintColorB = 1.0f;
    int32_t hairMapTextureIndex = -1;
};

struct RenderItem2D {
    glm::mat4 modelMatrix = glm::mat4(1);
    float colorTintR = 1.0f;
    float colorTintG = 1.0f;
    float colorTintB = 1.0f;
    int textureIndex = -1;
    int baseVertex = 0;
    int baseIndex = 0;
};

struct SpriteSheetRenderItem {
    glm::vec4 position;
    glm::vec4 rotation;
    glm::vec4 scale;
    glm::vec4 aabbMin;
    glm::vec4 aabbMax;

    float uOffset;
    float vOffset;
    int textureIndex;
    int frameIndex;

    int frameIndexNext;
    int rowCount;
    int columnCount;
    int isBillboard;

    float mixFactor;
    float padding0;
    float padding1;
    float padding2;
};

struct MeshRenderingInfo {
    uint32_t meshIndex;
    uint32_t materialIndex;
    BlendingMode blendingMode;
};

struct ViewportData {
    glm::mat4 projectionReverseZ;
    glm::mat4 inverseProjectionReverseZ;
    glm::mat4 projectionViewReverseZ;
    glm::mat4 prevProjectionViewReverseZ;
    glm::mat4 inverseProjectionViewReverseZ;
    glm::mat4 projection;
    glm::mat4 inverseProjection;
    glm::mat4 view;
    glm::mat4 inverseView;
    glm::mat4 projectionView;
    glm::mat4 prevProjectionView;
    glm::mat4 inverseProjectionView;
    glm::mat4 skyboxProjectionView;
    glm::mat4 flashlightProjectionView;
    glm::mat4 previousProjectionView = glm::mat4(1.0f);

    glm::mat4 csmLightProjectionView[SHADOW_CASCADE_COUNT];

    int xOffset;
    int yOffset;
    int width;
    int height;

    float posX;
    float posY;
    float sizeX;
    float sizeY;

    glm::vec4 frustumPlane0;
    glm::vec4 frustumPlane1;
    glm::vec4 frustumPlane2;
    glm::vec4 frustumPlane3;
    glm::vec4 frustumPlane4;
    glm::vec4 frustumPlane5;
    glm::vec4 flashlightDir;
    glm::vec4 flashlightPosition;

    float flashlightModifer;
    bool isOrtho;
    float orthoSize;
    float fov;

    glm::vec4 viewPos;
    glm::vec4 cameraForward;
    glm::vec4 cameraUp;
    glm::vec4 cameraRight;
    glm::vec4 colorTint;

    float colorContrast;
    int isInShop;
    float padding1;
    float vignetteIntensityScalar;

    glm::vec4 vignetteColor;
};

struct RendererData {
    glm::vec4 moonLightDir = glm::vec4(0.0f);
    float nearPlane;
    float farPlane;
    float gBufferWidth;
    float gBufferHeight;
    float hairBufferWidth;
    float hairBufferHeight;
    float time;
    int splitscreenMode;
    int rendererOverrideState;
    float normalizedMouseX;
    float normalizedMouseY;
    int tileCountX;
    int tileCountY;
};

struct DrawCommandsSet {
    std::vector<RenderItem> glass[4];

    std::vector<DrawIndexedIndirectCommand> alphaDiscard[4];
    std::vector<DrawIndexedIndirectCommand> blended[4];
    std::vector<DrawIndexedIndirectCommand> hair[4];
    std::vector<DrawIndexedIndirectCommand> standard[4];
    std::vector<DrawIndexedIndirectCommand> procedural[4];
    std::vector<DrawIndexedIndirectCommand> mirrorRenderItems[4];
    std::vector<DrawIndexedIndirectCommand> plastic[4];
    std::vector<DrawIndexedIndirectCommand> emissive[4];

    std::vector<DrawIndexedIndirectCommand> skinnedAlphaDiscard[4];
    std::vector<DrawIndexedIndirectCommand> skinnedBlended[4];
    std::vector<DrawIndexedIndirectCommand> skinnedHair[4];
    std::vector<DrawIndexedIndirectCommand> skinnedStandard[4];

    std::vector<DrawIndexedIndirectCommand> skinnedNonDeformingAlphaDiscard[4];
    std::vector<DrawIndexedIndirectCommand> skinnedNonDeformingBlended[4];
    std::vector<DrawIndexedIndirectCommand> skinnedNonDeformingHair[4];
    std::vector<DrawIndexedIndirectCommand> skinnedNonDeformingStandard[4];

    std::vector<DrawIndexedIndirectCommand> shadowMapHiRes[SHADOWMAP_HI_RES_COUNT][6];
    std::vector<DrawIndexedIndirectCommand> moonLightCascades[4][SHADOW_CASCADE_COUNT];
};

struct FlashLightShadowMapDrawInfo {
    std::vector<DrawIndexedIndirectCommand> flashlightShadowMapGeometry[4];
    std::vector<uint32_t> heightMapChunkIndices[4];
};

struct WaterState {
    float heightBeneathWater = 0;
    float heightAboveWater = 0;

    bool cameraUnderWater = false;
    bool feetUnderWater = false;
    bool wading = false;
    bool swimming = false;

    bool cameraUnderWaterPrevious = false;
    bool feetUnderWaterPrevious = false;
    bool wadingPrevious = true;
    bool swimmingPrevious = true;
};

struct PlayerControls {
    unsigned int WALK_FORWARD = HELL_KEY_W;
    unsigned int WALK_BACKWARD = HELL_KEY_S;
    unsigned int WALK_LEFT = HELL_KEY_A;
    unsigned int WALK_RIGHT = HELL_KEY_D;
    unsigned int INTERACT = HELL_KEY_E;
    unsigned int RELOAD = HELL_KEY_R;
    unsigned int FIRE = HELL_MOUSE_LEFT;
    unsigned int ADS = HELL_MOUSE_RIGHT;
    unsigned int JUMP = HELL_KEY_SPACE;
    unsigned int CROUCH = HELL_KEY_WIN_CONTROL;
    unsigned int NEXT_WEAPON = HELL_KEY_Q;
    unsigned int ESCAPE = HELL_KEY_WIN_ESCAPE;
    unsigned int DEBUG_FULLSCREEN = HELL_KEY_G;
    unsigned int DEBUG_ONE = HELL_KEY_1;
    unsigned int DEBUG_TWO = HELL_KEY_2;
    unsigned int DEBUG_THREE = HELL_KEY_3;
    unsigned int DEBUG_FOUR = HELL_KEY_4;
    unsigned int MELEE = HELL_KEY_V;
    unsigned int FLASHLIGHT = HELL_KEY_F;
    unsigned int MISC_WEAPON_FUNCTION = HELL_KEY_T;
    unsigned int RUN = HELL_KEY_LEFT_SHIFT;
    unsigned int TOGGLE_INVENTORY = HELL_KEY_WIN_TAB;
};

struct PhysicsFilterData {
    RaycastGroup raycastGroup = RaycastGroup::RAYCAST_DISABLED;
    CollisionGroup collisionGroup = CollisionGroup::NO_COLLISION;
    CollisionGroup collidesWith = CollisionGroup::ENVIROMENT_OBSTACLE;
};

struct SelectionRectangleState {
    int beginX = 0;
    int beginY = 0;
    int currentX = 0;
    int currentY = 0;
};

struct RendererSettings {
    int depthPeelCount = 3;
    bool drawGrass = true;
    bool screenspaceReflections = true;
    bool debugDrawPointCloud = false;
    bool debugDrawPointCloudGrid = false;
    bool debugDrawIrradianceProbes = false;
    bool enableIrradianceProbeSampling = true;
    bool enableLighting = true;
    bool irradianceUsesSH = false;
    RendererOverrideState rendererOverrideState = RendererOverrideState::NONE;
    ProbeDebugState probeDebugState = ProbeDebugState::HIDDEN;
};

struct SpawnOffset {
    glm::vec3 translation = glm::vec3(0.0);
    float yRotation = 0;
};

struct HeightMapChunk {
    ivecXZ coord;
    int baseIndex = 0;
    int baseVertex = 0;
    glm::vec3 aabbMin;
    glm::vec3 aabbMax;
};

struct PhysicsUserData {
    uint64_t physicsId = 0;
    uint64_t objectId = 0;
    PhysicsType physicsType = PhysicsType::NONE;
    ObjectType objectType = ObjectType::NO_TYPE;
};

struct PhysXRayResult {
    PhysicsUserData userData;
    std::string hitObjectName = UNDEFINED_STRING;
    glm::vec3 hitPosition = glm::vec3(0.0f);
    glm::vec3 hitNormal = glm::vec3(0.0f);
    glm::vec3 rayDirection = glm::vec3(0.0f);
    bool hitFound = false;
    float distanceToHit = std::numeric_limits<float>::max();
};

struct PhysXOverlapResult {
    PhysicsUserData userData;
    glm::vec3 objectPosition;
};

struct PhysXOverlapReport {
    std::vector<PhysXOverlapResult> hits;

    bool HitsFound() {
        return hits.size();
    }
};

struct OceanReadbackData {
    float heightPlayer0 = 0.0f;
    float heightPlayer1 = 0.0f;
    float heightPlayer2 = 0.0f;
    float heightPlayer3 = 0.0f;
};

struct BloodDecalInstanceData {
    glm::mat4 modelMatrix;
    glm::mat4 inverseModelMatrix;
    int type;
    int textureIndex;
    int padding1;
    int padding2;
};

struct HouseLocation {
    HouseType type = HouseType::UNDEFINED;
    std::string houseName = "RANDOM";
    glm::vec3 position = glm::vec3(0.0f);
    float rotation = 0.0f;
};
