#include "RigidStatic.h"
#include "Physics/Physics.h"

void RigidStatic::Update(float deltaTime) {
    
}

void RigidStatic::MarkForRemoval() {
    m_markedForRemoval = true;
}

void RigidStatic::SetPxRigidStatic(PxRigidStatic* rigidStatic) {
    m_pxRigidStatic = rigidStatic;
}

void RigidStatic::AddPxShape(PxShape* shape) {
    m_pxShapes.push_back(shape);
}