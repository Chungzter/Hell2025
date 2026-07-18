#include "PictureFrame.h"
#include "Hell/Logging.h"

#include "Unloved/Render/Renderer.h"

namespace Unloved {

PictureFrame::PictureFrame(uint64_t id, PictureFrameCreateInfo& createInfo, SpawnOffset& spawnOffset) {
    m_objectId = id;
    m_createInfo = createInfo;

    m_createInfo.position += spawnOffset.translation;
    m_createInfo.rotation += glm::vec3(0.0f, spawnOffset.yRotation, 0.0f);

    SelectRandomPicture();
}

void PictureFrame::CleanUp() {
    m_meshNodes.CleanUp();
}

void PictureFrame::Update() {
    Transform transform;
    transform.position = m_createInfo.position;
    transform.rotation = m_createInfo.rotation;
    transform.scale = m_createInfo.scale;

    m_meshNodes.Update(transform.to_mat4());
}

void PictureFrame::SelectRandomPicture() {
    const std::vector<const char*> bigLandscapeImages = {
        //"Picture_RainbowMage_ALB",
        "Picture_SHNakedLady",
        "Picture_Raptors",
        "Picture_SamNeil",
        "Picture_Minotaur"
    };

    std::string materialName = "CheckerBoard";

    if (m_createInfo.type == PictureFrameType::BIG_LANDSCAPE) {
        int random = rand() % bigLandscapeImages.size();
        materialName = bigLandscapeImages[random];
    }
    else {
        // TODO
    }

    std::vector<MeshNodeCreateInfo> meshNodeCreateInfoSet;

    MeshNodeCreateInfo& picture = meshNodeCreateInfoSet.emplace_back();
    picture.meshName = "picture_low.003";
    picture.materialName = materialName;

    MeshNodeCreateInfo& frame = meshNodeCreateInfoSet.emplace_back();
    frame.meshName = "frame_side.L_low.022";
    frame.materialName = "PictureFrame0";

    m_meshNodes.Init(m_objectId, "PictureFrame_BigLandscape", meshNodeCreateInfoSet);
}

void PictureFrame::SetPosition(const glm::vec3& position) {
    m_createInfo.position = position;
}

void PictureFrame::SetRotation(const glm::vec3& rotation) {
    m_createInfo.rotation = rotation;
}

void PictureFrame::SetScale(const glm::vec3& scale) {
    m_createInfo.scale = scale;
}
}
