#pragma once
#include "API/OpenGL/Types/GL_mesh2D.h"
#include "Hell/Types.h"

struct Mesh2D { 
    Mesh2D() = default;
    Mesh2D(const std::string& name);
    Mesh2D(const Mesh2D&) = delete;
    Mesh2D& operator=(const Mesh2D&) = delete;
    Mesh2D(Mesh2D&&) noexcept = default;
    Mesh2D& operator=(Mesh2D&&) noexcept = default;
    ~Mesh2D() = default;

    OpenGLMesh2D& GetGLMesh2D();

    const std::string& GetName() const { return m_name; }

private:
    std::string m_name = UNDEFINED_STRING;

    OpenGLMesh2D glMesh2D;
};