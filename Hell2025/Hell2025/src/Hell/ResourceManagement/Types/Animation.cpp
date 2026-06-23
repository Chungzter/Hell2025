#include "Animation.h"

#include "Hell/Logging.h"

float Animation::GetTicksPerSecond() const {
    return m_ticksPerSecond != 0 ? m_ticksPerSecond : 25.0f;
}

void Animation::SetFileInfo(const FileInfo& fileInfo) {
    m_fileInfo = fileInfo;
}

const FileInfo& Animation::GetFileInfo() const {
    return m_fileInfo;
}

LoadingState Animation::GetLoadingState() const {
    return m_loadingState.GetLoadingState();
}

void Animation::SetLoadingState(LoadingState loadingState) {
    m_loadingState = loadingState;
}

const std::string& Animation::GetName() const {
    return m_fileInfo.name;
}

void Animation::PrintNodeNames() const {
    std::string result = m_fileInfo.name + "\n";

    for (const AnimatedNode& node : m_animatedNodes) {
        result += " - " + node.m_nodeName + "\n";
    }

    Logging::Debug() << result;
}
