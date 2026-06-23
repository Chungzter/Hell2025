#include "DebugDraw.h"

#include "Hell/BVH/Types.h"
#include "Hell/ResourceManagement/ResourceManager.h"

using namespace Hell;

namespace DebugDraw {
    namespace {
        std::vector<DebugVertex2D> g_points2D;
        std::vector<DebugVertex3D> g_points3D;
        std::vector<DebugVertex2D> g_lines2D;
        std::vector<DebugVertex3D> g_lines3D;
        std::vector<DebugVertex3D> g_itemExaminelines;

        void UnpackTriangle(const BVHTriangle& triangle, glm::vec3& p0, glm::vec3& e1, glm::vec3& e2) {
            p0 = glm::vec3(triangle.v0_and_e1x);
            e1 = glm::vec3(triangle.v0_and_e1x.w, triangle.e1yz_and_e2xy.x, triangle.e1yz_and_e2xy.y);
            e2 = glm::vec3(triangle.e1yz_and_e2xy.z, triangle.e1yz_and_e2xy.w, triangle.e2z_and_normal.x);
        }
    }

    void BeginFrame() {
        g_lines3D.clear();
        g_lines2D.clear();
        g_points2D.clear();
        g_points3D.clear();
        g_itemExaminelines.clear();
    }

    void UploadVertexData() {
        GenericMesh& genericMeshLines2D = ResourceManager::GetGenericMesh("DebugLines2D");
        genericMeshLines2D.UpdateVertexData(g_lines2D);

        GenericMesh& genericMeshLines3D = ResourceManager::GetGenericMesh("DebugLines3D");
        genericMeshLines3D.UpdateVertexData(g_lines3D);

        GenericMesh& genericMeshPoints2D = ResourceManager::GetGenericMesh("DebugPoints2D");
        genericMeshPoints2D.UpdateVertexData(g_points2D);

        GenericMesh& genericMeshPoints3D = ResourceManager::GetGenericMesh("DebugPoints3D");
        genericMeshPoints3D.UpdateVertexData(g_points3D);

        GenericMesh& genericMeshExamineLines2D = ResourceManager::GetGenericMesh("DebugMeshItemExamineLines");
        genericMeshExamineLines2D.UpdateVertexData(g_itemExaminelines);
    }

    void DrawLine(const glm::vec3& begin, const glm::vec3& end, const glm::vec4& color, bool depthEnabled, int exclusiveViewportIndex, int ignoredViewportIndex) {
        DebugVertex3D v0 = DebugVertex3D(begin, color, glm::ivec2(0, 0), int(depthEnabled), exclusiveViewportIndex);
        DebugVertex3D v1 = DebugVertex3D(end, color, glm::ivec2(0, 0), int(depthEnabled), exclusiveViewportIndex);
        g_lines3D.push_back(v0);
        g_lines3D.push_back(v1);
    }

    void DrawItemExamineLine(const glm::vec3& begin, const glm::vec3& end, const glm::vec4& color) {
        bool depthEnabled = true;
        int exclusiveViewportIndex = -1;
        DebugVertex3D v0 = DebugVertex3D(begin, color, glm::ivec2(0, 0), int(depthEnabled), exclusiveViewportIndex);
        DebugVertex3D v1 = DebugVertex3D(end, color, glm::ivec2(0, 0), int(depthEnabled), exclusiveViewportIndex);
        g_itemExaminelines.push_back(v0);
        g_itemExaminelines.push_back(v1);
    }


    void DrawLine2D(const glm::ivec2& begin, const glm::ivec2& end, const glm::vec4& color) {
        g_lines2D.emplace_back(DebugVertex2D(begin, color));
        g_lines2D.emplace_back(DebugVertex2D(end, color));
    }

    void DrawPoint(const glm::vec3& position, const glm::vec4& color, bool depthEnabled, int exclusiveViewportIndex) {
        g_points3D.push_back(DebugVertex3D(position, color, glm::ivec2(0, 0), int(depthEnabled), exclusiveViewportIndex));
    }

    void DrawPoint2D(const glm::ivec2& position, const glm::vec4& color) {
        g_points2D.emplace_back(DebugVertex2D(position, color));
    }

    void DrawAABB(const AABB& aabb, const glm::vec4& color) {
        glm::vec3 FrontTopLeft = glm::vec3(aabb.GetBoundsMin().x, aabb.GetBoundsMax().y, aabb.GetBoundsMax().z);
        glm::vec3 FrontTopRight = glm::vec3(aabb.GetBoundsMax().x, aabb.GetBoundsMax().y, aabb.GetBoundsMax().z);
        glm::vec3 FrontBottomLeft = glm::vec3(aabb.GetBoundsMin().x, aabb.GetBoundsMin().y, aabb.GetBoundsMax().z);
        glm::vec3 FrontBottomRight = glm::vec3(aabb.GetBoundsMax().x, aabb.GetBoundsMin().y, aabb.GetBoundsMax().z);
        glm::vec3 BackTopLeft = glm::vec3(aabb.GetBoundsMin().x, aabb.GetBoundsMax().y, aabb.GetBoundsMin().z);
        glm::vec3 BackTopRight = glm::vec3(aabb.GetBoundsMax().x, aabb.GetBoundsMax().y, aabb.GetBoundsMin().z);
        glm::vec3 BackBottomLeft = glm::vec3(aabb.GetBoundsMin().x, aabb.GetBoundsMin().y, aabb.GetBoundsMin().z);
        glm::vec3 BackBottomRight = glm::vec3(aabb.GetBoundsMax().x, aabb.GetBoundsMin().y, aabb.GetBoundsMin().z);
        DrawLine(FrontTopLeft, FrontTopRight, color);
        DrawLine(FrontBottomLeft, FrontBottomRight, color);
        DrawLine(BackTopLeft, BackTopRight, color);
        DrawLine(BackBottomLeft, BackBottomRight, color);
        DrawLine(FrontTopLeft, FrontBottomLeft, color);
        DrawLine(FrontTopRight, FrontBottomRight, color);
        DrawLine(BackTopLeft, BackBottomLeft, color);
        DrawLine(BackTopRight, BackBottomRight, color);
        DrawLine(FrontTopLeft, BackTopLeft, color);
        DrawLine(FrontTopRight, BackTopRight, color);
        DrawLine(FrontBottomLeft, BackBottomLeft, color);
        DrawLine(FrontBottomRight, BackBottomRight, color);
    }

    void DrawItemExamineAABB(const AABB& aabb, const glm::vec4& color) {
        glm::vec3 FrontTopLeft = glm::vec3(aabb.GetBoundsMin().x, aabb.GetBoundsMax().y, aabb.GetBoundsMax().z);
        glm::vec3 FrontTopRight = glm::vec3(aabb.GetBoundsMax().x, aabb.GetBoundsMax().y, aabb.GetBoundsMax().z);
        glm::vec3 FrontBottomLeft = glm::vec3(aabb.GetBoundsMin().x, aabb.GetBoundsMin().y, aabb.GetBoundsMax().z);
        glm::vec3 FrontBottomRight = glm::vec3(aabb.GetBoundsMax().x, aabb.GetBoundsMin().y, aabb.GetBoundsMax().z);
        glm::vec3 BackTopLeft = glm::vec3(aabb.GetBoundsMin().x, aabb.GetBoundsMax().y, aabb.GetBoundsMin().z);
        glm::vec3 BackTopRight = glm::vec3(aabb.GetBoundsMax().x, aabb.GetBoundsMax().y, aabb.GetBoundsMin().z);
        glm::vec3 BackBottomLeft = glm::vec3(aabb.GetBoundsMin().x, aabb.GetBoundsMin().y, aabb.GetBoundsMin().z);
        glm::vec3 BackBottomRight = glm::vec3(aabb.GetBoundsMax().x, aabb.GetBoundsMin().y, aabb.GetBoundsMin().z);
        DrawItemExamineLine(FrontTopLeft, FrontTopRight, color);
        DrawItemExamineLine(FrontBottomLeft, FrontBottomRight, color);
        DrawItemExamineLine(BackTopLeft, BackTopRight, color);
        DrawItemExamineLine(BackBottomLeft, BackBottomRight, color);
        DrawItemExamineLine(FrontTopLeft, FrontBottomLeft, color);
        DrawItemExamineLine(FrontTopRight, FrontBottomRight, color);
        DrawItemExamineLine(BackTopLeft, BackBottomLeft, color);
        DrawItemExamineLine(BackTopRight, BackBottomRight, color);
        DrawItemExamineLine(FrontTopLeft, BackTopLeft, color);
        DrawItemExamineLine(FrontTopRight, BackTopRight, color);
        DrawItemExamineLine(FrontBottomLeft, BackBottomLeft, color);
        DrawItemExamineLine(FrontBottomRight, BackBottomRight, color);
    }

    void DrawAABB(const AABB& aabb, const glm::vec4& color, const glm::mat4& worldTransform) {
        glm::vec3 FrontTopLeft = worldTransform * glm::vec4(aabb.GetBoundsMin().x, aabb.GetBoundsMax().y, aabb.GetBoundsMax().z, 1.0f);
        glm::vec3 FrontTopRight = worldTransform * glm::vec4(aabb.GetBoundsMax().x, aabb.GetBoundsMax().y, aabb.GetBoundsMax().z, 1.0f);
        glm::vec3 FrontBottomLeft = worldTransform * glm::vec4(aabb.GetBoundsMin().x, aabb.GetBoundsMin().y, aabb.GetBoundsMax().z, 1.0f);
        glm::vec3 FrontBottomRight = worldTransform * glm::vec4(aabb.GetBoundsMax().x, aabb.GetBoundsMin().y, aabb.GetBoundsMax().z, 1.0f);
        glm::vec3 BackTopLeft = worldTransform * glm::vec4(aabb.GetBoundsMin().x, aabb.GetBoundsMax().y, aabb.GetBoundsMin().z, 1.0f);
        glm::vec3 BackTopRight = worldTransform * glm::vec4(aabb.GetBoundsMax().x, aabb.GetBoundsMax().y, aabb.GetBoundsMin().z, 1.0f);
        glm::vec3 BackBottomLeft = worldTransform * glm::vec4(aabb.GetBoundsMin().x, aabb.GetBoundsMin().y, aabb.GetBoundsMin().z, 1.0f);
        glm::vec3 BackBottomRight = worldTransform * glm::vec4(aabb.GetBoundsMax().x, aabb.GetBoundsMin().y, aabb.GetBoundsMin().z, 1.0f);
        DrawLine(FrontTopLeft, FrontTopRight, color);
        DrawLine(FrontBottomLeft, FrontBottomRight, color);
        DrawLine(BackTopLeft, BackTopRight, color);
        DrawLine(BackBottomLeft, BackBottomRight, color);
        DrawLine(FrontTopLeft, FrontBottomLeft, color);
        DrawLine(FrontTopRight, FrontBottomRight, color);
        DrawLine(BackTopLeft, BackBottomLeft, color);
        DrawLine(BackTopRight, BackBottomRight, color);
        DrawLine(FrontTopLeft, BackTopLeft, color);
        DrawLine(FrontTopRight, BackTopRight, color);
        DrawLine(FrontBottomLeft, BackBottomLeft, color);
        DrawLine(FrontBottomRight, BackBottomRight, color);
    }

    void DrawOBB(const OBB& obb, const glm::vec4& color) {
        const std::vector<glm::vec3>& corners = obb.GetCorners();
        if (corners.size() < 8) return;

        // Bottom Face
        DrawLine(corners[0], corners[1], color);
        DrawLine(corners[1], corners[5], color);
        DrawLine(corners[5], corners[4], color);
        DrawLine(corners[4], corners[0], color);

        // Top Face
        DrawLine(corners[2], corners[3], color);
        DrawLine(corners[3], corners[7], color);
        DrawLine(corners[7], corners[6], color);
        DrawLine(corners[6], corners[2], color);

        DrawLine(corners[0], corners[2], color); // Front Left
        DrawLine(corners[1], corners[3], color); // Front Right
        DrawLine(corners[4], corners[6], color); // Back Left
        DrawLine(corners[5], corners[7], color); // Back Right
    }

    void DrawFrustum(const Frustum& frustum, const glm::vec4& color) {
        glm::vec3 ntl = frustum.GetCorner(0);
        glm::vec3 ntr = frustum.GetCorner(1);
        glm::vec3 nbl = frustum.GetCorner(2);
        glm::vec3 nbr = frustum.GetCorner(3);

        glm::vec3 ftl = frustum.GetCorner(4);
        glm::vec3 ftr = frustum.GetCorner(5);
        glm::vec3 fbl = frustum.GetCorner(6);
        glm::vec3 fbr = frustum.GetCorner(7);

        // near face
        DrawLine(ntl, ntr, color);
        DrawLine(ntr, nbr, color);
        DrawLine(nbr, nbl, color);
        DrawLine(nbl, ntl, color);

        // far face
        DrawLine(ftl, ftr, color);
        DrawLine(ftr, fbr, color);
        DrawLine(fbr, fbl, color);
        DrawLine(fbl, ftl, color);

        // connect near to far
        DrawLine(ntl, ftl, color);
        DrawLine(ntr, ftr, color);
        DrawLine(nbl, fbl, color);
        DrawLine(nbr, fbr, color);
    }

    void DrawCircle(const glm::vec3& center, float radius, const glm::vec3 axisU, const glm::vec3 axisV, int segments, const glm::vec4& color) {
        const float step = glm::two_pi<float>() / float(segments);
        glm::vec3 prev = center + radius * axisU;
        for (int i = 1; i <= segments; ++i) {
            float a = step * float(i);
            glm::vec3 p = center + radius * (axisU * cos(a) + axisV * sin(a));
            DrawLine(prev, p, color);
            prev = p;
        }
    }

    void DrawSphere(const glm::vec3& position, float radius, const glm::vec4& color) {
        const int numLongitudes = 12;
        const int numLatitudes = 8;
        const int segsPerCircle = 96;

        const glm::vec3 up = glm::vec3(0, 1, 0);
        glm::vec3 right = glm::vec3(1, 0, 0);
        glm::vec3 axisA = glm::normalize(glm::cross(up, right));
        glm::vec3 axisB = glm::normalize(glm::cross(up, axisA));

        // Longitudes
        for (int m = 0; m < numLongitudes; ++m) {
            float theta = glm::two_pi<float>() * float(m) / float(numLongitudes);
            glm::vec3 dir = axisA * cos(theta) + axisB * sin(theta);
            DrawCircle(position, radius, up, dir, segsPerCircle, color);
        }

        // Latitudes
        for (int j = 1; j < numLatitudes; ++j) {
            float t = float(j) / float(numLatitudes);
            float phi = glm::pi<float>() * (t - 0.5f);
            float z = radius * sin(phi);
            float r = radius * cos(phi);
            DrawCircle(position + up * z, r, axisA, axisB, segsPerCircle, color);
        }
    }

    void DrawMeshBvhTriangles(const MeshBvh& meshBvh, const glm::vec4& color, const glm::mat4& worldTransform) {
        for (const BVHTriangle& triangle : meshBvh.m_triangles) {
            glm::vec3 p0, e1, e2;
            UnpackTriangle(triangle, p0, e1, e2);

            glm::vec3 p1 = p0 - e1;
            glm::vec3 p2 = p0 + e2;

            p0 = worldTransform * glm::vec4(p0, 1.0f);
            p1 = worldTransform * glm::vec4(p1, 1.0f);
            p2 = worldTransform * glm::vec4(p2, 1.0f);

            DrawLine(p0, p1, color);
            DrawLine(p1, p2, color);
            DrawLine(p2, p0, color);
        }
    }

    void DrawMeshBvhNodes(const MeshBvh& meshBvh, const glm::vec4& color, const glm::mat4& worldTransform) {
        for (const BvhNode& node : meshBvh.m_nodes) {
            AABB aabb(node.boundsMin, node.boundsMax);
            DrawAABB(aabb, color, worldTransform);
        }
    }

    void DrawSceneBvhNodes(const SceneBvh& sceneBvh, const glm::vec4& color) {
        const glm::mat4 worldTransform = glm::mat4(1.0f);

        for (const BvhNode& node : sceneBvh.m_nodes) {
            AABB aabb(node.boundsMin, node.boundsMax);
            DrawAABB(aabb, color, worldTransform);
        }
    }

    void DrawBvhRayResultTriangle(const BvhRayResult& rayResult, const SceneBvh& sceneBvh, const glm::vec4& color) {
        DrawBvhRayResultTriangle(rayResult, sceneBvh.m_triangles, color);
    }

    void DrawBvhRayResultTriangle(const BvhRayResult& rayResult, const std::vector<BVHTriangle>& triangles, const glm::vec4& color) {
        if (!rayResult.hitFound) return;

        const size_t triangleIndex = rayResult.primtiviveId / 12;
        if (triangleIndex >= triangles.size()) return;

        glm::vec3 p0, e1, e2;
        UnpackTriangle(triangles[triangleIndex], p0, e1, e2);

        glm::vec3 p1 = p0 - e1;
        glm::vec3 p2 = p0 + e2;

        p0 = rayResult.primitiveTransform * glm::vec4(p0, 1.0f);
        p1 = rayResult.primitiveTransform * glm::vec4(p1, 1.0f);
        p2 = rayResult.primitiveTransform * glm::vec4(p2, 1.0f);

        DrawPoint(p0, color);
        DrawPoint(p1, color);
        DrawPoint(p2, color);
        DrawLine(p0, p1, color);
        DrawLine(p2, p1, color);
        DrawLine(p0, p2, color);
    }

    void DrawBvhRayResultNode(const BvhRayResult& rayResult, const glm::vec4& color) {
        if (!rayResult.hitFound) return;

        AABB aabb(rayResult.nodeBoundsMin, rayResult.nodeBoundsMax);
        DrawAABB(aabb, color, rayResult.primitiveTransform);
    }
}
