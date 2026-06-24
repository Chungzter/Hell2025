#pragma once
#include <Game/Types.h>
#include "Types/Renderer/Model.h"

#include "Hell/ResourceManagement/Types/Material.h"

struct Trim {
    void Init(Transform transform, const std::string& modelName, const std::string& materialName);
    void SubmitRenderItem();

private:
    Transform m_transform;
    int32_t m_materialIndex = -1;
    Model* m_model;
    RenderItem m_renderItem;
    uint64_t m_objectId = 0;

    void UpdateRenderItem();
};
