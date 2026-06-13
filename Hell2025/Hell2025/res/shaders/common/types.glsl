
struct ViewportData {
    mat4 projectionReverseZ;
    mat4 inverseProjectionReverseZ;
    mat4 projectionViewReverseZ;
    mat4 prevProjectionViewReverseZ;
    mat4 inverseProjectionViewReverseZ;
    mat4 projection;
    mat4 inverseProjection;
    mat4 view;
    mat4 inverseView;
    mat4 projectionView;
    mat4 prevProjectionView;
    mat4 inverseProjectionView;
    mat4 skyboxProjectionView;
    mat4 flashlightProjectionView;
    mat4 previousProjectionView;

    mat4 csmLightProjectionView[5];

    int xOffset;
    int yOffset;
    int width;
    int height;

    float posX;  // 0 t0 1 range
    float posY;  // 0 t0 1 range
    float sizeX; // 0 t0 1 range
    float sizeY; // 0 t0 1 range

    vec4 frustumPlane0;
    vec4 frustumPlane1;
    vec4 frustumPlane2;
    vec4 frustumPlane3;
    vec4 frustumPlane4;
    vec4 frustumPlane5;
    vec4 flashlightDir;
    vec4 flashlightPosition;

    float flashlightModifer;
    int isOrtho; //true or false
    float orthoSize;
    float fov;

    vec4 viewPos;
    vec4 cameraForward;
    vec4 cameraUp;
    vec4 cameraRight;

    vec4 colorTint;

    float colorContrast;
    int isInShop; //true or false
    float padding1;
    float intensityScalar;

    vec4 vignetteColor;
};

struct RendererData {
    vec4 moonLightDir;

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
    mat4 modelMatrix;
    mat4 prevModelMatrix;
    mat4 inverseModelMatrix;
    vec4 aabbMin;
    vec4 aabbMax;

    int meshIndex;
    int baseColorTextureIndex;
    int normalMapTextureIndex;
    int rmaTextureIndex;

    int objectType;
    int woundMaskTextureIndex;
    int exclusiveViewportIndex;
    int ignoredViewportIndex;

    uint objectIdLowerBit;
    uint objectIdUpperBit;
    int baseSkinnedVertex;
    int baseSkinningTransformIndex;

    uint openableId;
    uint customId;
    int skinned;                        // True or false
    uint shadowBit;

    float emissiveR;
    float emissiveG;
    float emissiveB;
    int emissiveTextureIndex;           // -1 means nothing, anything else is a texture index

    uint baseVertex;
    uint baseIndex;
    uint baseVertexWeight;
    int blockScreenSpaceBloodDecals;    // True or false

    int additionalTextureIndex0;
    int additionalTextureIndex1;
    int additionalTextureIndex2;
    int additionalTextureIndex3;

    int localMeshNodeIndex;
    int opacityTextureIndex;
    int padding1;
    int padding2;

    float tintColorR;
    float tintColorG;
    float tintColorB;
    int hairMapTextureIndex;
};

struct Light {
    float posX;
    float posY;
    float posZ;
    float colorR;

    float colorG;
    float colorB;
    float strength;
    float radius;

    int lightIndex;
    int shadowMapDirty; // true or false
    int useIes;         // true or false
    int iesIndex;

    float iesVScale;
    float iesVBias;
    float iesHScale;
    float iesHBias;

    vec4 forward_iesMaxIntensity;
    vec4 right_iesExposure;
    vec4 up;

    int iesTextureIndex;
    int isDirtyForRaytracing; // true or false
    int padding1;
    int padding2;
};

struct TileLights {
    uint lightCount;
    uint lightIndices[127];
};

struct TileWorldBounds {
    vec4 boundsMin; // w: count of non-background pixels
    vec4 boundsMax; // w: unused
};

//struct TileBloodDecals {
//    uint decalCount;
//    uint decalOffset;
//};

struct TileInstanceData {
    uint count;
    uint offset;
};

struct BloodDecal {
    mat4 modelMatrix;
    mat4 inverseModelMatrix;
    int type;
    int textureIndex;
    int padding1;
    int padding2;
};

struct ChristmasLight {
    vec4 position;
    vec4 color;
};

struct MetaBall {
    vec4 posAndInvSigma2;
};

struct CloudPoint {
    vec4 position;
    vec4 normal;
    vec4 directLightingRGB_dirty;
    vec4 baseColor;
};

struct CloudPointTextureInfo {
    float u;
    float v;
    int baseColorIndex;
    int rmaIndex;
};

struct BvhNode {
    vec3 boundsMin;
    uint firstChildOrPrimitive;
    vec3 boundsMax;
    uint primitiveCount;
};

struct EntityInstance {
    mat4 worldTransform;
    mat4 inverseWorldTransform;

    int rootNodeIndex;
    int objectIdLowerBit;    // Unused on the GPU currently. CPU bvh stuff uses it.
    int objectIdUpperBit;    // Unused on the GPU currently. CPU bvh stuff uses it.
    int openableId;          // Unused on the GPU currently. CPU bvh stuff uses it.

    uint globalMeshIndex;    // Also unsued by the GPU
    uint customId;           // Also unsued by the GPU
    uint localMeshNodeIndex; // Also unsued by the GPU
    uint padding2;           // Also unsued by the GPU
};

struct Triangle {
    vec4 v0_and_e1x;     // p0.xyz, e1.x
    vec4 e1yz_and_e2xy;  // e1.yz,  e2.xy
    vec4 e2z_and_normal; // e2.z,   normal.xyz
};

struct DispatchIndirectArgs {
    uint num_groups_x;
    uint num_groups_y;
    uint num_groups_z;
};

const int PROBE_DISTANCE_OCTA_SIZE = 16;
const int INTERIOR_SIZE = 14;
//const int PROBE_NUM_IRRADIANCE_INTERIOR_TEXELS = 14;
const int PROBE_NUM_DISTANCE_INTERIOR_TEXELS = 14;

//#define PROBE_DISTANCE_OCTA_SIZE 16
#define PROBE_DISTANCE_TEXEL_COUNT (PROBE_DISTANCE_OCTA_SIZE * PROBE_DISTANCE_OCTA_SIZE)
//#define RAYS_PER_PROBE 256

const int PROBE_MAX_DISTANCE_COOLDOWN = 20;
const int PROBE_MAX_IRRADIANCE_COOLDOWN = 35;
const float PROBE_MAX_RAY_DISTANCE = 1.5;
const float PROBE_NORMAL_BIAS = 0.075;
const float PROBE_VIEW_BIAS = 0.1;
const float IRRADIANCE_DAMPENING = 0.0325;

struct ProbeColor {
    vec4 sh[9];
};

struct ProbeState {
    vec3 relocationOffset;
    uint padding;

    bool isRelevant;
    bool isActive;
    uint distanceCooldown;
    uint irradianceCooldown;
};

struct DDGIVolume {
    vec3 origin;
    float probeSpacing;
    ivec3 probeCounts; // number of probes on each axis
    int totalProbes;
    vec3 worldBoundsMin;
    float padding0;
    vec3 worldBoundsMax;
    float padding1;
};

struct AABB {
    vec4 boundsMin;
    vec4 boundsMax;
};

struct Particle {
    vec4 position;
    vec4 velocity;
    float rotation;
    float rotationalVelocity;
    float lifeTime;
    uint exists;
};

struct Material {
    int basecolor;
    int normal;
    int rma;
    int emissive;

    int opacity;
    int hairMaps;
    int padding;
    int padding2;
};