#pragma once
#include <cstdint>

enum class LoadState : uint8_t {
    NOT_REQUESTED,
    QUEUED,
    LOADING,
    LOADED,
    FAILED
};

enum class UploadState : uint8_t {
    NOT_REQUESTED,
    QUEUED,
    UPLOADING,
    UPLOADED,
    FAILED
};
