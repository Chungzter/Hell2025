#include "Mesh.h"

void Mesh::SetName(const std::string& name_) {
    name = name_;
}

int32_t Mesh::GetIndexCount() {
    return indexCount;
}

const std::string& Mesh::GetName() {
    return name;
}
