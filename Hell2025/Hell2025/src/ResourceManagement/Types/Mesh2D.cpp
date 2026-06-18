#include "Mesh2D.h"

Mesh2D::Mesh2D(const std::string& name) {
    m_name = name;
}

OpenGLMesh2D& Mesh2D::GetGLMesh2D() {
    return glMesh2D;
}
