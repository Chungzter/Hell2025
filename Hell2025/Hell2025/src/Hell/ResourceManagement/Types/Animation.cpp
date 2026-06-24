#include "Animation.h"

#include "Hell/Logging.h"

namespace {
    size_t StringAllocatedByteCount(const std::string& value) {
        return value.capacity() + 1;
    }

    size_t FileInfoAllocatedByteCount(const FileInfo& fileInfo) {
        return StringAllocatedByteCount(fileInfo.path) +
               StringAllocatedByteCount(fileInfo.name) +
               StringAllocatedByteCount(fileInfo.ext) +
               StringAllocatedByteCount(fileInfo.dir);
    }
}

float Animation::GetTicksPerSecond() const {
    return m_ticksPerSecond != 0 ? m_ticksPerSecond : 25.0f;
}

void Animation::SetFileInfo(const FileInfo& fileInfo) {
    m_fileInfo = fileInfo;
}

const FileInfo& Animation::GetFileInfo() const {
    return m_fileInfo;
}

LoadState Animation::GetLoadState() const {
    return m_loadState;
}

void Animation::SetLoadState(LoadState loadState) {
    m_loadState = loadState;
}

const std::string& Animation::GetName() const {
    return m_fileInfo.name;
}

size_t Animation::GetCPUAllocatedByteCount() const {
    size_t byteCount = FileInfoAllocatedByteCount(m_fileInfo);
    byteCount += m_animatedNodes.capacity() * sizeof(AnimatedNode);

    for (const AnimatedNode& node : m_animatedNodes) {
        byteCount += StringAllocatedByteCount(node.m_nodeName);
        byteCount += node.m_nodeKeys.capacity() * sizeof(SQT);
    }

    return byteCount;
}

void Animation::PrintNodeNames() const {
    std::string result = m_fileInfo.name + "\n";

    for (const AnimatedNode& node : m_animatedNodes) {
        result += " - " + node.m_nodeName + "\n";
    }

    Logging::Debug() << result;
}
