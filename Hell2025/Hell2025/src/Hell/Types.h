#pragma once
#include <vector>
#include <iostream>
#include "Hell/Common.h"
#include <Hell/GLM.h>
#include <Hell/Render/TextureTypes.h>
#include <unordered_map>

struct BlitRegion {
    int32_t originX = 0;
    int32_t originY = 0;
    int32_t width = 0;
    int32_t height = 0;
};

struct BlitRect {
    int32_t x0 = 0;
    int32_t y0 = 0;
    int32_t x1 = 0;
    int32_t y1 = 0;
};

struct BindlessMeshInstance {
    glm::mat4 modelMatrix;
    uint32_t meshletOffset;
    uint32_t primitiveOffset;
    uint32_t vertexOffset;
    uint32_t flags;
};

struct GLSLMeshlet {
    uint32_t vertexOffset;
    uint32_t primitiveOffset;
    uint32_t vertexCount;
    uint32_t primitiveCount;
};

struct AABBRayResult {
	bool hitFound = false;
	glm::vec3 hitPositionWorld = glm::vec3(0.0f);
	glm::vec3 hitPositionLocal = glm::vec3(0.0f);
	glm::vec3 hitNormalWorld = glm::vec3(0.0f);
	glm::vec3 hitNormalLocal = glm::vec3(0.0f);
};

struct FileInfo {
    std::string path;
    std::string name;
    std::string ext;
    std::string dir;
};

struct Transform {
	glm::vec3 position = glm::vec3(0);
	glm::vec3 rotation = glm::vec3(0);
	glm::vec3 scale = glm::vec3(1);

    Transform() = default;

    explicit Transform(const glm::vec3& position, const glm::vec3& rotation = glm::vec3(0.0f), const glm::vec3& scale = glm::vec3(1.0f)) {
        this->position = position;
        this->rotation = rotation;
        this->scale = scale;
    }

	glm::mat4 to_mat4() const {
		glm::mat4 m = glm::translate(glm::mat4(1), position);
		m *= glm::mat4_cast(glm::quat(rotation));
		m = glm::scale(m, scale);
		return m;
	};
};

struct AnimatedTransform {
    AnimatedTransform() = default;
    AnimatedTransform(glm::mat4 matrix) {
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(matrix, scale, rotation, translation, skew, perspective);
    }
    glm::mat4 to_mat4() const {
        glm::mat4 m = glm::translate(glm::mat4(1), translation);
        m *= glm::mat4_cast(rotation);
        m = glm::scale(m, scale);
        return m;
    };
    glm::vec3 to_forward_vector() const {
        glm::quat q = glm::quat(rotation);
        return glm::normalize(q * glm::vec3(0.0f, 0.0f, 1.0f));
    }
    glm::vec3 translation = glm::vec3(0);
    glm::quat rotation = glm::quat(1, 0, 0, 0);
    glm::vec3 scale = glm::vec3(1);
};

struct Material {
    Material() {}
    std::string m_name = UNDEFINED_STRING;
    int m_basecolor = 0;
	int m_normal = 0;
	int m_rma = 0;
	int m_emissive = 0;
	//int m_hairFlowMap = 0;
    //int m_hairIdMap = 0;
    //int m_hairRootMap = 0;
    //int m_hairBlendMap = 0;
    int m_opacity = 0;
    int m_hairMaps = 0;
};

struct QueuedTextureBake {
    void* texture = nullptr;
    int jobID = 0;
    int width = 0;
    int height = 0;
    ImageFormat imageFormat = ImageFormat::UNDEFINED;
    int mipmapLevel = 0;
    int dataSize = 0;
    const void* data = nullptr;
    bool inProgress = false;
};

struct Resolutions {
    glm::ivec2 gBuffer;
    glm::ivec2 finalImage;
    glm::ivec2 ui;
    glm::ivec2 hair;
};

struct DrawIndexedIndirectCommand {
    uint32_t indexCount = 0;
    uint32_t instanceCount = 0;
    uint32_t firstIndex = 0;
    int32_t  baseVertex = 0;
    uint32_t baseInstance = 0;
};

struct DrawArraysIndirectCommand {
    uint32_t vertexCount = 0;
    uint32_t instanceCount = 0;
    uint32_t firstVertex = 0;
    uint32_t baseInstance = 0;
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

    int lightIndex;
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
    int padding1;
    int padding2;
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

struct Node {
    std::string name;
    int parentIndex;
    glm::mat4 inverseBindTransform;
};

struct vecXZ {
    float x = 0.0f;
    float z = 0.0f;

    vecXZ() : x(0.0f), z(0.0f) {}
    vecXZ(float x, float z) : x(x), z(z) {}

    bool operator==(const vecXZ& other) const {
        return x == other.x && z == other.z;
    }

    bool operator!=(const vecXZ& other) const {
        return !(*this == other);
    }

    bool operator<(const vecXZ& other) const {
        return (x < other.x) || (x == other.x && z < other.z);
    }

    bool operator>(const vecXZ& other) const {
        return other < *this;
    }

    bool operator<=(const vecXZ& other) const {
        return !(other < *this);
    }

    bool operator>=(const vecXZ& other) const {
        return !(*this < other);
    }
};

struct ivecXZ {
    int x = 0;
    int z = 0;

    ivecXZ() : x(0), z(0) {}
    ivecXZ(int x, int z) : x(x), z(z) {}

    bool operator==(const ivecXZ& other) const {
        return x == other.x && z == other.z;
    }

    bool operator!=(const ivecXZ& other) const {
        return !(*this == other);
    }

    bool operator<(const ivecXZ& other) const {
        return (x < other.x) || (x == other.x && z < other.z);
    }

    bool operator>(const ivecXZ& other) const {
        return other < *this;
    }

    bool operator<=(const ivecXZ& other) const {
        return !(other < *this);
    }

    bool operator>=(const ivecXZ& other) const {
        return !(*this < other);
    }
};

struct CubeRayResult {
    Transform cubeTransform = Transform();
    glm::vec3 hitPosition = glm::vec3(0.0f);
    glm::vec3 hitNormal = glm::vec3(0.0f);
    float distanceToHit = 0;
    bool hitFound = false;
};

#pragma pack(push, 1)
struct BvhNode {
    glm::vec3 boundsMin;
    uint32_t firstChildOrPrimitive;
    glm::vec3 boundsMax;
    uint32_t primitiveCount;
};
#pragma pack(pop)

struct RayData {
    float origin[3];
    float dir[3];
    float invDir[3];
    float paddedInvDir[3];
    float minDistance = 0;
    float maxDistance = 0;
    int octant[3];
};

struct PrimitiveInstance {
    uint64_t objectId;
    uint64_t meshBvhId;
    glm::vec3 worldAabbBoundsMin;
    glm::vec3 worldAabbBoundsMax;
	glm::vec3 worldAabbCenter;
	glm::mat4 worldTransform;
	glm::mat4 inverseWorldTransform;
    uint32_t openableId;
    uint32_t customId;
    uint32_t globalMeshIndex;
    uint32_t localMeshNodeIndex;
};

struct GpuPrimitiveInstance {
    glm::mat4 worldTransform;
    glm::mat4 inverseWorldTransform;

    int32_t rootNodeIndex;
    uint32_t objectIdLowerBit;
    uint32_t objectIdUpperBit;
    uint32_t openableId;

    uint32_t globalMeshIndex;
    uint32_t customId;
    uint32_t localMeshNodeIndex;
    uint32_t padding2;
};

struct SceneBvh {
	std::vector<BvhNode> m_nodes;
	std::vector<PrimitiveInstance> m_instances;
	std::vector<GpuPrimitiveInstance> m_gpuInstances;
};

struct MeshBvh {
    std::vector<BvhNode> m_nodes;
    std::vector<float> m_triangleData;
};

struct BvhRayResult {
    bool hitFound = false;
    size_t primtiviveId = 0;
    uint64_t objectId = 0;
    uint32_t openableId = 0;
    uint32_t customId = 0;
    uint32_t globalMeshIndex = 0;
    uint32_t localMeshNodeIndex = 0;
    float distanceToHit = std::numeric_limits<float>::max();
    glm::vec3 hitPosition = glm::vec3(0);
    glm::vec3 hitNormal = glm::vec3(0.0f);
    glm::mat4 primitiveTransform = glm::mat4(1.0f);
    glm::vec3 nodeBoundsMin = glm::vec3(0.0f);
    glm::vec3 nodeBoundsMax = glm::vec3(0.0f);
};

struct Bone {
    char name[64];
    glm::mat4 localRestPose = glm::mat4(1.0f);
    glm::mat4 inverseBindPose = glm::mat4(1.0f);
    int32_t parentIndex = -1;
    int32_t deformFlag = 0; // 0: non-deforming 1: deforming
};

struct DispatchIndirectCommand{
	uint32_t num_groups_x;
	uint32_t num_groups_y;
	uint32_t num_groups_z;
};

#define PROBE_DISTANCE_OCTA_SIZE 16
#define PROBE_DISTANCE_TEXEL_COUNT (PROBE_DISTANCE_OCTA_SIZE * PROBE_DISTANCE_OCTA_SIZE)

struct ProbeColor {
    glm::vec4 sh[9];
};

struct ProbeState {
    glm::vec3 relocationOffset = glm::vec3(0.0f);
    uint32_t padding;

    uint32_t isRelevant; // bool in GLSL
    uint32_t isActive;  // bool in GLSL
    uint32_t distanceCooldown;
    uint32_t irradianceCooldown;
};

struct DDGIVolumeGPU {
    glm::vec3 origin{};
    float probeSpacing{};

    glm::ivec3 probeCounts{};
    int32_t numProbes{};

    glm::vec3 worldBoundsMin{};
    float padding0{};

    glm::vec3 worldBoundsMax{};
    float padding1{};
};

struct GPUAABB {
    glm::vec4 boundsMin{};
    glm::vec4 boundsMax{};
};
