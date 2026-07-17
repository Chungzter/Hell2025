#include "VATInstance.h"

#include "Hell/Logging.h"
#include "Hell/ResourceManagement/ResourceManager.h"

#include <algorithm>
#include <cmath>

void VATInstance::Init(const VATInstanceCreateInfo& createInfo) {
    m_createInfo = createInfo;
    m_currentTime = 0.0f;
    m_currentFrameIdx = 0;
    m_fps = 0.0f;
    m_frameCount = 0;
    m_positionTextureIndex = -1;
    m_rotationTextureIndex = -1;
    m_lookupTextureIndex = -1;

    Hell::Vat* vat = Hell::ResourceManager::GetVATPtr(m_createInfo.resourceName);
    if (!vat) return;

    m_positionTextureIndex = Hell::ResourceManager::GetTextureBindlessIndexByName(m_createInfo.resourceName + "_pos", true);
    m_rotationTextureIndex = Hell::ResourceManager::GetTextureBindlessIndexByName(m_createInfo.resourceName + "_rot", true);
    m_lookupTextureIndex = Hell::ResourceManager::GetTextureBindlessIndexByName(m_createInfo.resourceName + "_lookup", true);

    const Hell::VATMetadata& metadata = vat->GetMetadata();
    m_frameCount = std::max(metadata.frameCount, 1);

    const float fps = metadata.fps > 0.0f ? metadata.fps : 24.0f;
    m_fps = std::max(fps * m_createInfo.playbackSpeed, 0.01f);

    if (!HasValidTextureIndices()) {
        Logging::Error() << "VATInstance::Init() failed to resolve VAT textures for '" << m_createInfo.resourceName << "'\n";
    }
}

void VATInstance::Update(float deltaTime, const glm::mat4& /*modelMatrix*/) {
    m_currentTime += deltaTime;

    Hell::Vat* vat = Hell::ResourceManager::GetVATPtr(m_createInfo.resourceName);
    if (!vat) return;

    const float loopDuration = static_cast<float>(m_frameCount) / m_fps;
    const float stopTime = static_cast<float>(m_frameCount - 1) / m_fps;

    if (m_createInfo.loop) {
        m_currentTime = std::fmod(m_currentTime, loopDuration);
    }
    else {
        m_currentTime = std::min(m_currentTime, stopTime);
    }

    m_currentFrameIdx = std::min(static_cast<int32_t>(m_currentTime * m_fps), m_frameCount - 1);
}
