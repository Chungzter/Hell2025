#include "Wall.h"
#include "Hell/Common/Bit.h"
#include "Hell/Common/Random.h"
#include "Hell/Math/Math.h"
#include "Unloved/Debug/DebugDraw.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Unloved/Systems/House/HouseBuilder.h"
#include "Legacy/Renderer/RenderDataManager.h"
#include "Legacy/Util/Util.h"
#include "Legacy/World/LegacyWorld.h"

#include "Hell/Logging.h"
#include "Unloved/Render/RendererConstants.h"

#include "Unloved/Common/CreateInfo.h"

namespace Unloved {

Wall::Wall(uint64_t id, const WallCreateInfo& createInfo, const SpawnOffset& spawnOffset) {
    m_objectId = id;
    m_createInfo = createInfo;
    m_spawnOffset = spawnOffset;

    for (glm::vec3& point : m_createInfo.points) {
        point += spawnOffset.translation;
    }

    UpdateSegmentsTrimsAndVertexData();
}

void Wall::UpdateSegmentsTrimsAndVertexData() {
    CleanUp();

    for (WallSegment& wallSegment : m_wallSegments) {
        wallSegment.CleanUp();
    }
    m_wallSegments.clear();

    //m_points = m_createInfo.points;
    //m_height = m_createInfo.height;
    //m_textureOffsetU = m_createInfo.textureOffsetU;
    //m_textureOffsetV = m_createInfo.textureOffsetV;
    //m_textureScale = m_createInfo.textureScale;
    m_materialIndex = Hell::ResourceManager::GetMaterialIndexByName(m_createInfo.materialName);
    m_ceilingTrimType = m_createInfo.ceilingTrimType;
    m_floorTrimType = m_createInfo.floorTrimType;

    if (m_createInfo.useReversePointOrder) {
        std::reverse(m_createInfo.points.begin(), m_createInfo.points.end());
    }

    for (int i = 0; i < GetPointCount() - 1; i++) {
        const glm::vec3& start = m_createInfo.points[i];
        const glm::vec3& end = m_createInfo.points[i + 1];
        WallSegment& wallSegment = m_wallSegments.emplace_back();
        wallSegment.Init(start, end, m_createInfo.height, m_objectId, m_spawnOffset);
    }

    // Calculate worldspace center
    m_worldSpaceCenter = glm::vec3(0.0f);
    if (!m_createInfo.points.empty()) {
        for (glm::vec3& point : m_createInfo.points) {
            m_worldSpaceCenter += point;
        }
        m_worldSpaceCenter /= m_createInfo.points.size();
    }

    // Create weather boards
    if (m_createInfo.wallType == WallType::WEATHER_BOARDS) {
        RecreateWeatherBoardMesh();
        CreateCSGVertexData();
    }
    // Create CSG geometry and trims
    else {
        CreateCSGVertexData();
        CreateTrims();
    }
}

void Wall::FlipFaces() {
    m_createInfo.useReversePointOrder = !m_createInfo.useReversePointOrder;
    UpdateSegmentsTrimsAndVertexData();
}

void Wall::UpdateWorldSpaceCenter(glm::vec3 worldSpaceCenter) {
    glm::vec3 offset = worldSpaceCenter - m_worldSpaceCenter;
    for (glm::vec3& point : m_createInfo.points) {
        point += offset;
    }
    UpdateSegmentsTrimsAndVertexData();
}

bool Wall::AddPointToEnd(glm::vec3 point, bool supressWarning) {
    glm::vec3& previousPoint = m_createInfo.points.back();
    float threshold = 0.05f;
    if (glm::distance(point, previousPoint) < threshold) {
        Logging::Debug() << "Wall::AddPoint() failed: new point " << point << " is too close to previous point " << previousPoint << "\n";
        return false;
    }

    m_createInfo.points.push_back(point);
    UpdateSegmentsTrimsAndVertexData();
    return true;
}

bool Wall::UpdatePointPosition(int pointIndex, glm::vec3 position, bool supressWarning) {
    if (pointIndex < 0 || pointIndex >= m_createInfo.points.size()) {
        Logging::Debug() << "Wall::UpdatePointPosition() failed: point index " << pointIndex << " out of range of size " << m_createInfo.points.size() << "\n";
    }

    // Threshold check
    float threshold = 0.05f;
    if (pointIndex > 0) {
        glm::vec3& previousPoint = m_createInfo.points[pointIndex - 1];
        if (glm::distance(position, previousPoint) < threshold) {
            Logging::Debug() << "Wall::UpdatePointPosition() failed: new point " << position << " is too close to previous point " << previousPoint << "\n";
            return false;
        }
    }
    if (pointIndex < m_createInfo.points.size() - 1) {
        glm::vec3& nextPoint = m_createInfo.points[pointIndex + 1];
        if (glm::distance(position, nextPoint) < threshold) {
            Logging::Debug() << "Wall::UpdatePointPosition() failed: new point " << position << " is too close to next point " << nextPoint << "\n";
            return false;
        }
    }

    m_createInfo.points[pointIndex] = position;
    UpdateSegmentsTrimsAndVertexData();
    return true;
}

void Wall::SetMaterial(const std::string& materialName) {
    const int32_t materialIndex = Hell::ResourceManager::GetMaterialIndexByName(materialName);
    if (materialIndex != -1) {
        m_createInfo.materialName = materialName;
        m_materialIndex = materialIndex;
        UpdateSegmentsTrimsAndVertexData();
    }
}

Material* Wall::GetMaterial() {
    return Hell::ResourceManager::GetMaterialByIndex(m_materialIndex);
}

void Wall::SetHeight(float value) {
    m_createInfo.height = value;
    UpdateSegmentsTrimsAndVertexData();
}

void Wall::SetTextureScale(float value) {
    m_createInfo.textureScale = value;
    UpdateSegmentsTrimsAndVertexData();
}

void Wall::SetTextureOffsetU(float value) {
    m_createInfo.textureOffsetU = value;
    UpdateSegmentsTrimsAndVertexData();
}

void Wall::SetTextureOffsetV(float value) {
    m_createInfo.textureOffsetV = value;
    UpdateSegmentsTrimsAndVertexData();
}

void Wall::SetFloorTrimType(TrimType trimType) {
    m_createInfo.floorTrimType = trimType;
    UpdateSegmentsTrimsAndVertexData();
}
void Wall::SetCeilingTrimType(TrimType trimType) {
    m_createInfo.ceilingTrimType = trimType;
    UpdateSegmentsTrimsAndVertexData();
}

const glm::vec3& Wall::GetPointByIndex(int pointIndex) {
    static glm::vec3 invalid = glm::vec3(0.0f);

    if (pointIndex < 0 || pointIndex >= m_createInfo.points.size()) {
        Logging::Error() << "Wall::GetPointByIndex() failed: point index " << pointIndex << " out of range of size " << m_createInfo.points.size() << "\n";
        return invalid;
    }
    return m_createInfo.points[pointIndex];
}

void Wall::CleanUp() {
    CleanUpWeatherBoardMesh();

    for (WallSegment& wallSegment : m_wallSegments) {
        wallSegment.CleanUp();
    }
}

void Wall::CreateTrims() {
    m_trims.clear();
	return;

    // Ceiling
    if (m_ceilingTrimType != TrimType::NONE) {
        for (int i = 0; i < (int)m_createInfo.points.size() - 1; i++) {
            const glm::vec3& start = m_createInfo.points[i];
            const glm::vec3& end = m_createInfo.points[i + 1];

            Hell::Transform t;
            t.position = start;
            t.position.y += m_createInfo.height;
            t.rotation.y = Hell::Math::YawBetweenPoints(start, end);
            t.scale.x = glm::distance(start, end);

            Trim& trim = m_trims.emplace_back();
            trim.Init(t, "TrimCeiling", "Trims");
        }
    }

    // Floor
    if (m_floorTrimType != TrimType::NONE) {
        for (int i = 0; i < (int)m_createInfo.points.size() - 1; i++) {
            const glm::vec3& start = m_createInfo.points[i];
            const glm::vec3& end = m_createInfo.points[i + 1];

            glm::vec3 rayOrigin = start;
            glm::vec3 rayDir = glm::normalize(end - start);
            const float segmentLength = glm::distance(start, end);
            float remaining = segmentLength;
            const float eps = 1e-3f;

            while (remaining > eps) {
                HouseBuilder::ClipRayResult r = HouseBuilder::RaycastClippingVolumes(rayOrigin, rayDir, remaining);
                if (!r.hitFound) break;

                // Only add a trim up to a NEAR face (entering the cube)
                if (glm::dot(r.hitNormal, rayDir) < 0.0f) {
                    Hell::Transform t;
                    t.position = rayOrigin;
                    t.rotation.y = Hell::Math::YawBetweenPoints(start, end);
                    t.scale.x = r.distanceToHit;
                    if (t.scale.x > eps) {
                        Trim& trim = m_trims.emplace_back();
                        trim.Init(t, "TrimFloor", "Trims");
                    }
                }

                float advance = r.distanceToHit + eps; // step through face
                rayOrigin += rayDir * advance;
                remaining -= advance;
            }

            if (remaining > eps) {
                Hell::Transform t;
                t.position = rayOrigin;
                t.rotation.y = Hell::Math::YawBetweenPoints(rayOrigin, end);
                t.scale.x = remaining;
                Trim& trim = m_trims.emplace_back();
                trim.Init(t, "TrimFloor", "Trims");
            }
        }
    }
}

void Wall::CreateCSGVertexData() {
    const std::vector<const HouseBuilder::ClippingVolume*> clippingVolumes = HouseBuilder::GetClippingVolumes();

    for (WallSegment& wallSegment : m_wallSegments) {
        wallSegment.CreateVertexData(clippingVolumes, m_createInfo.textureOffsetU, m_createInfo.textureOffsetV, m_createInfo.textureScale);
    }
}

void Wall::SubmitRenderItems() {
    Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("Procedural");

    // If this wall is exterior, then don't render the CSG geometry, or any trims if you accidentally set it to have trims
    if (m_createInfo.wallType == WallType::WEATHER_BOARDS) {


        for (uint32_t meshId : m_weatherBoardSegmentMeshIds) {
            Mesh* mesh = meshBuffer.GetMeshById(meshId);
            if (!mesh) continue;

            Material* material = Hell::ResourceManager::GetMaterialByName("WeatherBoards0");
            if (!material) continue;

            RenderItem renderItem;
            renderItem.baseColorTextureIndex = material->m_basecolor;
            renderItem.normalMapTextureIndex = material->m_normal;
            renderItem.rmaTextureIndex = material->m_rma;
            renderItem.modelMatrix = glm::mat4(1.0f);
            renderItem.inverseModelMatrix = glm::mat4(1.0f);
            renderItem.aabbMin = glm::vec4(mesh->aabbMin, 0.0f);
            renderItem.aabbMax = glm::vec4(mesh->aabbMax, 0.0f);
            renderItem.meshId = meshId;
            renderItem.baseVertex = mesh->baseVertex;
            renderItem.baseIndex = mesh->baseIndex;

            RenderDataManager::SubmitRenderItemProcedural(renderItem);
        }

        return;
    }

    for (WallSegment& wallSegment : m_wallSegments) {
        Mesh* mesh = meshBuffer.GetMeshById(wallSegment.GetMeshId());
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
        renderItem.meshId = wallSegment.GetMeshId();
        renderItem.baseVertex = mesh->baseVertex;
        renderItem.baseIndex = mesh->baseIndex;

		RenderDataManager::SubmitRenderItemProcedural(renderItem);
    }

    for (Trim& trim : m_trims) {
        trim.SubmitRenderItem();
    }
}

void Wall::DrawSegmentVertices(glm::vec4 color) {
    for (WallSegment& wallSegment : m_wallSegments) {
        const glm::vec3& p1 = wallSegment.GetStart();
        const glm::vec3& p2 = wallSegment.GetEnd();
        glm::vec3 p3 = wallSegment.GetStart() + glm::vec3(0.0f, wallSegment.GetHeight(), 0.0f);
        glm::vec3 p4 = wallSegment.GetEnd() + glm::vec3(0.0f, wallSegment.GetHeight(), 0.0f);
        DebugDraw::DrawPoint(p1, color);
        DebugDraw::DrawPoint(p2, color);
        DebugDraw::DrawPoint(p3, color);
        DebugDraw::DrawPoint(p4, color);
    }
}

void Wall::DrawSegmentLines(glm::vec4 color) {
    for (WallSegment& wallSegment : m_wallSegments) {
        const glm::vec3& p1 = wallSegment.GetStart();
        const glm::vec3& p2 = wallSegment.GetEnd();
        glm::vec3 p3 = wallSegment.GetStart() + glm::vec3(0.0f, wallSegment.GetHeight(), 0.0f);
        glm::vec3 p4 = wallSegment.GetEnd() + glm::vec3(0.0f, wallSegment.GetHeight(), 0.0f);
        DebugDraw::DrawLine(p1, p2, color);
        DebugDraw::DrawLine(p3, p4, color);
        DebugDraw::DrawLine(p1, p3, color);
        DebugDraw::DrawLine(p2, p4, color);

        glm::vec3 midPoint = Hell::Math::MidPoint(wallSegment.GetStart(), wallSegment.GetEnd());
        glm::vec3 normal = wallSegment.GetNormal();
        glm::vec3 projectedMidPoint = midPoint + (normal * 0.2f);
        DebugDraw::DrawLine(midPoint, projectedMidPoint, color);
    }
}

#define WEATHERBOARD_STOP_MESH_HEGIHT 2.6f


void AddBoard(const glm::vec3& origin, const glm::vec3& boardDir, int boardY, float boardWidth, std::vector<Vertex>& verticesOut, std::vector<uint32_t>& indicesOut) {
    Model* weatherBoardModel = Hell::ResourceManager::GetModelByName("WeatherBoard");
    if (!weatherBoardModel || weatherBoardModel->GetMeshIndices().empty()) return;
    if (weatherBoardModel->GetMeshCount() == 0) return;

    uint32_t meshId = weatherBoardModel->GetMeshIndices()[0];
    Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(meshId);
    if (!mesh) return;

    Hell::MeshBuffer& assetGeometry = Hell::ResourceManager::GetMeshBuffer("AssetGeometry");
    std::span<Vertex> verticesSpan(assetGeometry.GetVertices().data() + mesh->baseVertex, mesh->vertexCount);
    std::span<uint32_t> indicesSpan(assetGeometry.GetIndices().data() + mesh->baseIndex, mesh->indexCount);

    uint32_t baseVertex = verticesOut.size();

    // Calculate rotation matrix
    glm::vec3 zAxis = glm::normalize(boardDir);
    glm::vec3 xAxis = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), zAxis));
    glm::vec3 yAxis = glm::cross(zAxis, xAxis);
    glm::mat3 rotationMatrix = glm::mat3(xAxis, yAxis, zAxis);

    // If the board is above 15 then make it use board 12 and have a random x uv
    float randomOffsetX = 0.0f;
    if (boardY > 15) {
        boardY = 12 + Hell::Random::Int(0, 3);
        randomOffsetX = Hell::Random::Float(0.0f, 1.0f);
    }

    // This is the vertical uv distance between boards in the texture
    float uvVerticalOffset = 1.0f / 16.0f;

    for (Vertex vertex : verticesSpan) {

        // If this vertex is on the right of the board then shift it to the desired width and update uvs
        bool isRightEdge = vertex.uv.x > 0.5f;
        if (isRightEdge) {
            vertex.position.z *= boardWidth;
            vertex.uv.x = boardWidth * 0.25f; // 0.25 because the texture is 4 meter wide in world space
        }

        vertex.position = rotationMatrix * vertex.position;
        vertex.position += origin;
        
        vertex.normal = glm::normalize(rotationMatrix * vertex.normal);

        vertex.uv.x += randomOffsetX;
        vertex.uv.y -= uvVerticalOffset * boardY;

        verticesOut.push_back(vertex);
    }

    for (uint32_t index : indicesSpan) {
        indicesOut.push_back(index + baseVertex);
    }
}

void Wall::CleanUpWeatherBoardMesh() {
    Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("Procedural");

    // Clear any old mesh segments
    for (uint32_t meshId : m_weatherBoardSegmentMeshIds) {
        meshBuffer.RemoveMesh(meshId);
    }

    m_weatherBoardSegmentMeshIds.clear();
}

void Wall::RecreateWeatherBoardMesh() {
    Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("Procedural");

    CleanUpWeatherBoardMesh();
    m_weatherBoardstopRenderItems.clear();

    if (m_createInfo.wallType != WallType::WEATHER_BOARDS) return;

    Material* material = Hell::ResourceManager::GetMaterialByName("WeatherBoards0");
    Model* model = Hell::ResourceManager::GetModelByName("WeatherBoard_Stop");

    if (!model) {
        Logging::Error() << "Wall::CreateWeatherBoards() failed to load model 'WeatherBoard_Stop'";
        return;
    }

    float individialBoardHeight = 0.13f;
    float desiredTotalWallHeight = 5.6f;
    int weatherBoardCount = (int)(desiredTotalWallHeight / individialBoardHeight);
    float actualFinalWallHeight = weatherBoardCount * individialBoardHeight;


    for (WallSegment& wallSegemet : m_wallSegments) {
        glm::vec3 start = wallSegemet.GetStart();
        glm::vec3 end = wallSegemet.GetEnd();

        Hell::Transform transform;
        transform.position = start;
        transform.scale.y = actualFinalWallHeight;
        transform.rotation.y = Hell::Math::YawBetweenPoints(start, end);

        RenderItem& renderItem = m_weatherBoardstopRenderItems.emplace_back();
        renderItem.modelMatrix = transform.to_mat4();
        renderItem.inverseModelMatrix = glm::inverse(renderItem.modelMatrix);
        renderItem.meshId = model->GetMeshIndices()[0];
        renderItem.baseColorTextureIndex = material->m_basecolor;
        renderItem.rmaTextureIndex = material->m_rma;
        renderItem.normalMapTextureIndex = material->m_normal;
        Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("AssetGeometry");
        if (Mesh* mesh = meshBuffer.GetMeshById(renderItem.meshId)) {
            renderItem.baseIndex = mesh->baseIndex;
            renderItem.baseVertex = mesh->baseVertex;
        }
        renderItem.shadowBit |= (SHADOW_BIT_CAST_SHADOW | SHADOW_BIT_CAST_CSM_SHADOW | SHADOW_BIT_STATIC);

        Util::UpdateRenderItemAABB(renderItem);
        Hell::Bit::PackUint64(m_objectId, renderItem.objectIdLowerBit, renderItem.objectIdUpperBit);
    }

    // Create new mesh segments
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    for (WallSegment& wallSegemet : m_wallSegments) {

        for (int i = 0; i < weatherBoardCount; i++) {

            glm::vec3 start = wallSegemet.GetStart();
            glm::vec3 end = wallSegemet.GetEnd();

            start.y += individialBoardHeight * i;
            end.y += individialBoardHeight * i;

            glm::vec3 rayOrigin = start;
            glm::vec3 rayDir = glm::normalize(end - start);
            const float segLen = glm::distance(start, end);
            float remaining = segLen;
            const float eps = 1e-3f;

            while (remaining > eps) {
                HouseBuilder::ClipRayResult rayResult = HouseBuilder::RaycastClippingVolumes(rayOrigin, rayDir, remaining);
                if (!rayResult.hitFound) break;

                if (glm::dot(rayResult.hitNormal, rayDir) < 0.0f && rayResult.distanceToHit > eps) {
                    glm::vec3 localStart = rayOrigin;
                    glm::vec3 localEnd = rayOrigin + (rayDir * rayResult.distanceToHit);
                    float boardWidth = glm::distance(localStart, localEnd);

                    AddBoard(rayOrigin, rayDir, i, boardWidth, vertices, indices);
                }

                float advance = rayResult.distanceToHit + eps;
                rayOrigin += rayDir * advance;
                remaining -= advance;
            }

            if (remaining > eps) {
                float boardWidth = glm::distance(rayOrigin, end);
                AddBoard(rayOrigin, rayDir, i, boardWidth, vertices, indices);
            }
        }

        uint32_t meshId = meshBuffer.AddMesh(vertices, indices, "Weatherboards");
        m_weatherBoardSegmentMeshIds.emplace_back(meshId);
    }
}
}
