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

struct GPULight {
    float posX;
    float posY;
    float posZ;
    float colorR;

    float colorG;
    float colorB;
    float strength;
    float radius;

    int lightIndex; // Legacy. Replacing soon
    int shadowMapDirty = 1; // true or false
    int useIes = 0;         // true or false
    int iesIndex;

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

    uint32_t lightIdUpperBit = 0;
    uint32_t lightIdLowerBit = 0;
    uint32_t padding1;
    uint32_t padding2;
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
    int lightCount;
    int lightIndices[127];
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

struct SkinningJob {
    uint32_t vertexCount;
    uint32_t indexCount;
    uint32_t baseVertex;
    uint32_t baseIndex;

    uint32_t baseVertexWeight;
    uint32_t baseSkinningVertex;
    uint32_t baseSkinningTransformIndex;
    uint32_t padding1;
};

struct RayTracingGeometryRange {
    uint32_t baseVertex;
    uint32_t baseIndex;
    uint32_t vertexCount;
    uint32_t indexCount;
    uint32_t blendingMode;
    int32_t materialIndex;
    uint32_t shadowBit = 0;
    uint32_t padding0 = 0;
};

struct SkinnedRayTracingGroup {
    std::vector<RayTracingGeometryRange> ranges;
    glm::mat4 modelMatrix;
};

struct StaticRayTracingInstance {
    RayTracingGeometryRange range;
    uint64_t vulkanBlasId = 0;
    uint64_t vertexBufferDeviceAddress = 0;
    uint64_t indexBufferDeviceAddress = 0;
    glm::mat4 modelMatrix = glm::mat4(1.0f);
};

