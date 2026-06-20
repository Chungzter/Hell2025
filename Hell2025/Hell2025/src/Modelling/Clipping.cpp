#include "Clipping.h"
#include "AssetManagement/AssetManager.h"
#include "clipper2/clipper.h"
#include "Hell/Math/GLM.h"
#include "earcut/earcut.hpp"
#include <glm/gtx/intersect.hpp>
#include "Util.h"

namespace mapbox {
    namespace util {
        template <std::size_t I>
        struct nth<I, glm::vec2> {
            static_assert(I < 2, "Index out of range for glm::vec2");

            inline static float get(const glm::vec2& t) {
                return (I == 0) ? t.x : t.y;
            }
        };
    }
}

namespace Clipping {

    double ComputeSignedArea(const std::vector<glm::vec2>& points);
    std::vector<glm::vec2> ProjectWallSegmentTo2D(WallSegment& wallSegment);
    std::vector<glm::vec2> ProjectCubeSliceTo2D(const ClippingCube& cube, const WallSegment& refWallSegment);
    std::vector<glm::vec2> FlattenEarcutInput(const std::vector<std::vector<glm::vec2>>& earcutInput);
    std::vector<std::vector<glm::vec2>> ConvertClipperToEarcut(const Clipper2Lib::PathsD& solution);
    std::vector<Vertex> ProjectBackTo3D(const std::vector<glm::vec2>& vertices2D, const WallSegment& refWallSegment);
    Clipper2Lib::PathD ConvertToClipperPath(const std::vector<glm::vec2>& points);
    
    void SubtractCubesFromWallSegment(WallSegment& wallSegment, std::vector<ClippingCube>& clippingCubes, std::vector<Vertex>& verticesOut, std::vector<uint32_t>& indicesOut) {
        const AABB& wallAABB = wallSegment.GetAABB();
        const glm::vec3 wallNormal = wallSegment.GetNormal();
        const std::vector<glm::vec3>& corners = wallSegment.GetCorners();
        
        // Create wall path
        std::vector<glm::vec2> projectedWall = ProjectWallSegmentTo2D(wallSegment);
        Clipper2Lib::PathsD wallPath = { ConvertToClipperPath(projectedWall) };

        // Create cube paths
        Clipper2Lib::PathsD clippingPaths;
        for (ClippingCube& clippingCube : clippingCubes) {
            const AABB& cubeAABB = clippingCube.GetAABB();
            if (!wallAABB.IntersectsAABB(cubeAABB)) continue;

            std::vector<glm::vec2> projectedCube = ProjectCubeSliceTo2D(clippingCube, wallSegment);
            Clipper2Lib::PathD clipPath = ConvertToClipperPath(projectedCube);
            clippingPaths.push_back(clipPath);
        }

        // Perform Boolean Subtraction
        Clipper2Lib::PathsD solution = Clipper2Lib::Difference(wallPath, clippingPaths, Clipper2Lib::FillRule::NonZero);

        // Triangulate the result
        std::vector<std::vector<glm::vec2>> earcutInput = ConvertClipperToEarcut(solution);
        std::vector<glm::vec2> vertices2D = FlattenEarcutInput(earcutInput);
        verticesOut = ProjectBackTo3D(vertices2D, wallSegment);
        indicesOut = mapbox::earcut<uint32_t>(earcutInput);
    }

    std::vector<glm::vec2> ProjectWallSegmentTo2D(WallSegment& wallSegment) {
        glm::vec3 uDir = glm::normalize(wallSegment.GetEnd() - wallSegment.GetStart());
        glm::vec3 vDir = glm::vec3(0, 1, 0);
        glm::vec3 origin = wallSegment.GetStart();
        std::vector<glm::vec2> projectedPoints;
        for (const glm::vec3& corner : wallSegment.GetCorners()) {
            glm::vec3 local = corner - origin;
            projectedPoints.emplace_back(glm::dot(local, uDir), glm::dot(local, vDir));
        }
        return projectedPoints;
    }

    std::vector<glm::vec2> ProjectCubeSliceTo2D(const ClippingCube& cube, const WallSegment& refWallSegment) {
        glm::mat4 model = cube.GetModelMatrix();
        glm::vec3 localHalfExtents(0.5f, 0.5f, 0.5f);
        std::vector<glm::vec3> cubeVertices;
        cubeVertices.reserve(8);
        for (int i = 0; i < 8; ++i) {
            float x = (i & 1) ? localHalfExtents.x : -localHalfExtents.x;
            float y = (i & 2) ? localHalfExtents.y : -localHalfExtents.y;
            float z = (i & 4) ? localHalfExtents.z : -localHalfExtents.z;
            glm::vec4 localPos(x, y, z, 1.0f);
            cubeVertices.push_back(glm::vec3(model * localPos));
        }

        const int edgeIndices[12][2] = {
            {0, 1}, {1, 3}, {3, 2}, {2, 0},
            {4, 5}, {5, 7}, {7, 6}, {6, 4},
            {0, 4}, {1, 5}, {2, 6}, {3, 7}
        };

        glm::vec3 uDir = glm::normalize(refWallSegment.GetEnd() - refWallSegment.GetStart());
        glm::vec3 vDir = glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 wallNormal = glm::normalize(glm::cross(uDir, vDir));
        glm::vec3 planeOrigin = refWallSegment.GetStart();

        std::vector<glm::vec3> intersectionPoints;
        const float tolerance = 0.0001f;
        for (int i = 0; i < 12; ++i) {
            int idx0 = edgeIndices[i][0], idx1 = edgeIndices[i][1];
            const glm::vec3& p0 = cubeVertices[idx0], & p1 = cubeVertices[idx1];

            float d0 = glm::dot(p0 - planeOrigin, wallNormal);
            float d1 = glm::dot(p1 - planeOrigin, wallNormal);

            if (fabs(d0) < tolerance) intersectionPoints.push_back(p0);
            if (fabs(d1) < tolerance) intersectionPoints.push_back(p1);

            if (d0 * d1 < 0.0f) {
                float t = d0 / (d0 - d1);
                intersectionPoints.push_back(p0 + t * (p1 - p0));
            }
        }

        const float dupEpsilon = 0.001f;
        std::vector<glm::vec3> uniquePoints;
        for (const auto& pt : intersectionPoints) {
            if (std::none_of(uniquePoints.begin(), uniquePoints.end(),
                [&](const glm::vec3& upt) { return glm::length(pt - upt) < dupEpsilon; })) {
                uniquePoints.push_back(pt);
            }
        }

        if (uniquePoints.empty()) return {};

        std::vector<glm::vec2> projectedPoints;
        projectedPoints.reserve(uniquePoints.size());
        for (const glm::vec3& pt : uniquePoints) {
            glm::vec3 offset = pt - refWallSegment.GetStart();
            projectedPoints.emplace_back(glm::dot(offset, uDir), glm::dot(offset, vDir));
        }

        std::vector<glm::vec2> convexHull = Util::ComputeConvexHull2D(projectedPoints);
        return Util::SortConvexHullPoints2D(convexHull);
    }

    std::vector<Vertex> ProjectBackTo3D(const std::vector<glm::vec2>& vertices2D, const WallSegment& refWallSegment) {
        glm::vec3 uDir = glm::normalize(refWallSegment.GetEnd() - refWallSegment.GetStart());
        glm::vec3 vDir = glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 wDir = glm::normalize(glm::cross(uDir, vDir));
        glm::vec3 origin = refWallSegment.GetStart();

        std::vector<Vertex> vertices;
        vertices.reserve(vertices2D.size());

        for (const glm::vec2& pt : vertices2D) {
            glm::vec3 pos = origin + (pt.x * uDir) + (pt.y * vDir);
            pos -= glm::dot(pos - origin, wDir) * wDir;

            Vertex vertex;
            vertex.position = pos;
            vertices.push_back(vertex);
        }
        return vertices;
    }

    double ComputeSignedArea(const std::vector<glm::vec2>& points) {
        double area = 0.0;
        size_t n = points.size();
        for (size_t i = 0; i < n; ++i) {
            size_t j = (i + 1) % n;
            area += (points[j].x - points[i].x) * (points[j].y + points[i].y);
        }
        return area * 0.5;
    }

    std::vector<glm::vec2> FlattenEarcutInput(const std::vector<std::vector<glm::vec2>>& earcutInput) {
        std::vector<glm::vec2> finalVertices;
        for (const auto& polygon : earcutInput) {
            finalVertices.insert(finalVertices.end(), polygon.begin(), polygon.end());
        }
        return finalVertices;
    }

    std::vector<std::vector<glm::vec2>> ConvertClipperToEarcut(const Clipper2Lib::PathsD& solution) {
        std::vector<std::vector<glm::vec2>> earcutInput;
        std::vector<uint32_t> holeIndices;

        size_t totalVertices = 0;
        for (size_t i = 0; i < solution.size(); ++i) {
            std::vector<glm::vec2> polygon;
            for (const auto& p : solution[i]) {
                polygon.emplace_back(p.x, p.y);
            }

            if (!polygon.empty()) {
                if (ComputeSignedArea(polygon) < 0) {
                    holeIndices.push_back(totalVertices);
                }
                totalVertices += polygon.size();
                earcutInput.push_back(std::move(polygon));
            }
        }

        return earcutInput;
    }

    Clipper2Lib::PathD ConvertToClipperPath(const std::vector<glm::vec2>& points) {
        Clipper2Lib::PathD path;
        for (const auto& pt : points) {
            path.push_back(Clipper2Lib::PointD(pt.x, pt.y));
        }
        return path;
    }

    ClippingCubeRayResult CastClippingCubeRay(const glm::vec3& rayOrigin, const glm::vec3 rayDir, std::vector<ClippingCube>& clippingCubes) {
        ClippingCubeRayResult rayResult;
        rayResult.distanceToHit = std::numeric_limits<float>::max();

        Mesh* mesh = AssetManager::GetMeshByModelNameMeshName("Primitives", "Cube");
        if (!mesh) return rayResult;

        std::vector<Vertex>& vertices = AssetManager::GetVertices();
        std::vector<uint32_t>& indices = AssetManager::GetIndices();

        for (ClippingCube& clippingCube : clippingCubes) {
            const glm::mat4& modelMatrix = clippingCube.GetModelMatrix();

            for (int i = mesh->baseIndex; i < mesh->baseIndex + mesh->indexCount; i+=3) {
                uint32_t idx0 = indices[i + 0];
                uint32_t idx1 = indices[i + 1];
                uint32_t idx2 = indices[i + 2];
                const glm::vec3& vert0 = modelMatrix * glm::vec4(vertices[idx0 + mesh->baseVertex].position, 1.0f);
                const glm::vec3& vert1 = modelMatrix * glm::vec4(vertices[idx1 + mesh->baseVertex].position, 1.0f);
                const glm::vec3& vert2 = modelMatrix * glm::vec4(vertices[idx2 + mesh->baseVertex].position, 1.0f);
                float t = 0;

                if (Util::RayIntersectsTriangle(rayOrigin, rayDir, vert0, vert1, vert2, t) && t < rayResult.distanceToHit) {
                    rayResult.distanceToHit = t;
                    rayResult.hitFound = true;
                    rayResult.hitPosition = rayOrigin + (rayDir * t);
                    rayResult.hitClippingCube = &clippingCube;
                }
            }
        }

        return rayResult;
    }
}