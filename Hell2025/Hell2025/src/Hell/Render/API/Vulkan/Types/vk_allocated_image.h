#pragma once

#include "Hell/Render/API/Vulkan/vk_common.h"
#include <cstddef>
#include <string>

struct AllocatedImage {
    AllocatedImage() = default;
    AllocatedImage(VkFormat imageFormat, VkExtent3D imageExtent, VkSampleCountFlagBits sampleCount, VkImageUsageFlags usage, std::string debugName);
    AllocatedImage(const AllocatedImage&) = delete;
    AllocatedImage& operator=(const AllocatedImage&) = delete;
    AllocatedImage(AllocatedImage&& other) noexcept;
    AllocatedImage& operator=(AllocatedImage&& other) noexcept;

    void Sync(VkCommandBuffer cmd, VkAccessFlags2 dstAccess, VkPipelineStageFlags2 dstStage);
    void Cleanup();

    int32_t GetWidth() const            { return m_extent.width; }
    int32_t GetHeight() const           { return m_extent.height; }
    int32_t GetDepth() const            { return m_extent.depth; }
    VkExtent3D GetExtent() const        { return m_extent; }
    VkExtent2D GetExtent2D() const      { return { m_extent.width, m_extent.height }; }
    VkFormat GetFormat() const          { return m_format; }
    VkSampleCountFlagBits GetSampleCount() const { return m_sampleCount; }
    VkImage GetImage() const            { return m_image; }
    VkImageView GetImageView() const    { return m_imageView; }
    VkImageView GetDepthOnlyImageView() const { return m_depthOnlyImageView != VK_NULL_HANDLE ? m_depthOnlyImageView : m_imageView; }
    VmaAllocation GetAllocation() const { return m_allocation; }
    size_t GetCPUAllocatedByteCount() const;
    size_t GetGPUAllocatedByteCount() const;

private:
    VkImage m_image = VK_NULL_HANDLE;
    VkImageView m_imageView = VK_NULL_HANDLE;
    VkImageView m_depthOnlyImageView = VK_NULL_HANDLE;
    VmaAllocation m_allocation = VK_NULL_HANDLE;
    VkExtent3D m_extent = {};
    VkFormat m_format = VK_FORMAT_UNDEFINED;
    VkSampleCountFlagBits m_sampleCount = VK_SAMPLE_COUNT_1_BIT;

    VkImageLayout m_currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkAccessFlags2 m_currentAccessMask = 0;
    VkPipelineStageFlags2 m_currentStageFlags = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
};
