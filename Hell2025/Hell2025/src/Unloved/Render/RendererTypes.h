#pragma once

#include "Unloved/Render/RendererEnums.h"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <cstdint>
#include <vector>

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

    glm::mat4 jitteredProjectionViewReverseZ;
    glm::mat4 inverseJitteredProjectionViewReverseZ;

    glm::mat4 csmLightProjectionView[5]; // Is this right?

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
    uint32_t lightCount; // Boolean
    uint32_t enableDDGI; // Boolean
    uint32_t enableIndirectSpecular; // Boolean

    uint32_t enableTAA;  // Boolean
    float indirectSpecularFactor = 1.0f;
    float indirectSpecularRoughnessDampening = 1.0;
    uint32_t directPointShadowMode = 0;

    glm::vec2 taaJitterPx = glm::vec2(0.0f);
    uint32_t padding0 = 0;
    uint32_t padding1 = 0;

    glm::vec4 flashlightColor = glm::vec4(0.780f, 0.778f, 0.797f, 1.0f);

    float flashlightRange = 19.1f;
    float flashlightFalloffExponent = 4.29f;
    float flashlightBrightness = 1.0f;
    float flashlightIESConeScale = 1.0f;

    float flashlightIESInnerAngle = 14.0f;
    float flashlightIESOuterAngle = 40.0f;
    float flashlightIESContrast = 0.2f;
    float flashlightIESVerticalScale = 0.0f;

    float flashlightIESVerticalBias = 0.0f;
    float flashlightIESHorizontalBias = 0.0f;
    int32_t flashlightIESTextureIndex = -1;
    uint32_t flashlightIESEnabled = 1;

    float flashlightCenterSpotRange = 15.0f;
    float flashlightCenterSpotFalloffExponent = 4.0f;
    float flashlightCenterSpotBrightness = 1.0f;
    float flashlightCenterSpotInnerAngle = 1.5f;

    float flashlightCenterSpotOuterAngle = 5.0f;
    uint32_t flashlightCenterSpotEnabled = 1;
    uint32_t padding3 = 0;
    uint32_t enableDDGIReflections = 0;
};

struct RenderItem {
    glm::mat4 modelMatrix = glm::mat4(1);
    glm::mat4 prevModelMatrix = glm::mat4(1);
    glm::mat4 inverseModelMatrix = glm::mat4(1);

    glm::vec4 aabbMin = glm::vec4(0);
    glm::vec4 aabbMax = glm::vec4(0);

    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
    uint32_t baseVertex = 0;
    uint32_t baseIndex = 0;

    uint32_t baseVertexWeight = 0;
    uint32_t baseSkinningTransformIndex = 0;
    uint32_t objectIdLowerBit = 0;
    uint32_t objectIdUpperBit = 0;

    int32_t materialIndex = -1;
    int32_t woundMaskTextureIndex = -1;
    int32_t exclusiveViewportIndex = -1;
    int32_t ignoredViewportIndex = -1;

    uint32_t meshId = 0;
    uint32_t miscFlags = 0;
    uint32_t shadowFlags = 0;
    uint32_t vulkanFlags = 0;

    int32_t localMeshNodeIndex = 0;
    float emissiveR = 0.0f;
    float emissiveG = 0.0f;
    float emissiveB = 0.0f;

    uint32_t blendingMode = static_cast<uint32_t>(BlendingMode::DEFAULT);
    float tintColorR = 1.0f;
    float tintColorG = 1.0f;
    float tintColorB = 1.0f;

    uint32_t customId = 0;
    uint32_t openableId = 0;
    int32_t woundMaterialIndex = -1;
    int padding = 0;
};

struct GlassLightRange {
    uint32_t offset;
    uint32_t count;
};

struct SpriteSheetRenderItem {
    glm::mat4 modelMatrix = glm::mat4(1);
    glm::vec4 uvFrame = glm::vec4(0);
    glm::vec4 uvFrameNext = glm::vec4(0);
    glm::vec4 localOffset = glm::vec4(0);

    int textureIndex = -1;
    int isBillboard = 0;
    float mixFactor = 0.0f;
    int32_t exclusiveViewportIndex = -1;
};

struct GPULight {
    float posX;
    float posY;
    float posZ;
    float colorR;

    float colorG;
    float colorB;
    float strength;
    float radius;

    float iesVScale;
    float iesVBias;
    float iesHScale;
    float iesHBias;

    glm::vec3 forward;
    float iesMaxIntensity;

    glm::vec3 right;
    float iesExposure;

    glm::vec3 up;
    int padding0;

    int iesTextureIndex;
    int isDirtyForRaytracing = 0; // true or false
    int hiResShadowMapIndex;
    int lowResShadowMapIndex;

    glm::vec4 worldBoundsMin = glm::vec4(0.0f);
    glm::vec4 worldBoundsMax = glm::vec4(0.0f);
};

struct GPUAABB {
    glm::vec4 boundsMin{};
    glm::vec4 boundsMax{};
};

struct GPUChristmasLight {
    glm::vec4 position;
    glm::vec4 color;
};

struct TileLights {
    uint32_t lightCount;
    uint32_t lightIndices[127];
};

struct TileWorldBounds {
    glm::vec4 boundsMin; // w: count of non-background pixels
    glm::vec4 boundsMax; // w: unused
};

struct TileInstanceData {
    unsigned int count;
    unsigned int offset;
};

struct GpuParticle {
    glm::vec4 position;
    glm::vec4 velocity;

    float rotation;
    float rotationalVelocity;
    float lifeTime = 0.0f;
    uint32_t exists = 0;
};

// Skinning

struct SkinningJob {
    uint32_t vertexCount;
    uint32_t indexCount;
    uint32_t sourceBaseVertex;
    uint32_t sourceBaseIndex;

    uint32_t sourceVertexWeightOffset;
    uint32_t skinnedBaseVertex;
    uint32_t skinningTransformOffset;
    uint32_t padding;
};

struct SkinningDispatchGroup {
    uint32_t jobIndex;
    uint32_t vertexOffset;
    uint32_t padding0;
    uint32_t padding1;
};

// Vulkan ray queries

struct RayQueryMesh {
    uint32_t baseVertex;
    uint32_t baseIndex;
    uint32_t vertexCount;
    uint32_t indexCount;
};

struct RayQueryMaterial {
    uint32_t blendingMode;
    int32_t materialIndex;
    uint32_t shadowBit = 0;
    uint32_t padding0 = 0;
};

struct RayQueryMeshInstance {
    RayQueryMesh mesh;
    RayQueryMaterial material;
};

struct TransientRayQueryBLASInstance {
    std::vector<RayQueryMeshInstance> meshInstances;
    glm::mat4 modelMatrix = glm::mat4(1.0f);
};

struct RayQueryMultiMeshBLAS {
    std::vector<RayQueryMeshInstance> meshInstances;
    uint64_t vulkanBlasId = 0;
    uint64_t vertexBufferDeviceAddress = 0;
    uint64_t indexBufferDeviceAddress = 0;
    uint64_t vertexBufferByteSize = 0;
    uint64_t indexBufferByteSize = 0;
    uint64_t sourceGeometryVersion = 0;
    glm::mat4 modelMatrix = glm::mat4(1.0f);
};

struct RayQueryBLASInstance {
    RayQueryMeshInstance meshInstance;
    uint64_t vulkanBlasId = 0;
    uint64_t vertexBufferDeviceAddress = 0;
    uint64_t indexBufferDeviceAddress = 0;
    glm::mat4 modelMatrix = glm::mat4(1.0f);
};

