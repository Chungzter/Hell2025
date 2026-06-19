#include "Resources.h"

#include "Hell/ResourceManagement/ResourceManager.h"

namespace Game {

void InitResources() {
    Hell::ResourceManager::CreateGenericMesh("DebugLines2D");
    Hell::ResourceManager::CreateGenericMesh("DebugLines3D");
    Hell::ResourceManager::CreateGenericMesh("DebugPoints2D");
    Hell::ResourceManager::CreateGenericMesh("DebugPoints3D");
    Hell::ResourceManager::CreateGenericMesh("DebugMeshItemExamineLines");
    Hell::ResourceManager::CreateGenericMesh("UI");
    Hell::ResourceManager::CreateMeshBuffer("Procedural");
}

}
