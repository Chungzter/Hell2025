#include "GenericMesh.h"

#include "API/OpenGL/GL_resource_manager.h"
#include "Hell/Backend/BackEnd.h"
#include "Hell/Logging.h"

#include <algorithm>

namespace Hell {

GenericMesh::GenericMesh(const std::string& name) {
    m_name = name;
}

void GenericMesh::UpdateVertexData(const void* vertices, size_t vertexCount, const VertexLayoutDescription& layout) {
    if (m_vertexStride == 0) {
        m_vertexStride = layout.stride;
    }

    m_vertexCount = vertexCount;
    m_vertexCapacity = std::max(m_vertexCapacity, vertexCount);

    if (Hell::BackEnd::GetAPI() == API::OPENGL) {
        if (m_openGLId == 0) {
            m_openGLId = OpenGLResourceManager::CreateGenericMesh();
        }

        OpenGLResourceManager::GetGenericMesh(m_openGLId).UpdateVertexData(vertices, vertexCount, layout);
    }
}

void GenericMesh::UpdateIndexData(const std::vector<uint32_t>& indices) {
    m_indexCount = indices.size();
    m_indexCapacity = std::max(m_indexCapacity, indices.size());

    if (Hell::BackEnd::GetAPI() == API::OPENGL) {
        if (m_openGLId == 0) {
            m_openGLId = OpenGLResourceManager::CreateGenericMesh();
        }

        OpenGLResourceManager::GetGenericMesh(m_openGLId).UpdateIndexData(indices);
    }
}

void GenericMesh::CleanUp() {
    if (m_openGLId != 0) {
        OpenGLResourceManager::RemoveGenericMesh(m_openGLId);
        m_openGLId = 0;
    }

    m_vertexStride = 0;
    m_vertexCount = 0;
    m_indexCount = 0;
    m_vertexCapacity = 0;
    m_indexCapacity = 0;
}

size_t GenericMesh::GetCPUAllocatedByteCount() const {
    return 0;
}

size_t GenericMesh::GetGPUAllocatedByteCount() const {
    return (m_vertexCapacity * m_vertexStride) +
           (m_indexCapacity * sizeof(uint32_t));
}

uint32_t GenericMesh::GetVAO() const {
    if (Hell::BackEnd::GetAPI() == API::OPENGL) {
        OpenGLGenericMesh& mesh = OpenGLResourceManager::GetGenericMesh(m_openGLId);
        return mesh.GetVAO();
    }
    else {
        Logging::Error() << "GenericMesh::GetVAO() was called but API is Vulkan\n";
        return 0;
    }
}

} // namespace
