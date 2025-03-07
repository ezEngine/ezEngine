#pragma once

#include <vulkan/vulkan.hpp>

VK_DEFINE_HANDLE(ezVulkanAllocation)

class ezGALBufferVulkan;
class ezGALTextureVulkan;

struct ezStagingBufferVulkan
{
  vk::Buffer m_buffer;
  vk::DeviceSize m_uiOffset = 0;
  ezVulkanAllocation m_alloc;
  ezByteArrayPtr m_Data;
};

struct ezPendingBufferCopyVulkan
{
  ezStagingBufferVulkan m_SrcBuffer = {};
  const ezGALBufferVulkan* m_pDstBuffer = nullptr;
  vk::BufferCopy m_Region;
};

struct ezPendingTextureCopyVulkan
{
  ezStagingBufferVulkan m_SrcBuffer = {};
  const ezGALTextureVulkan* m_pDstTexture = nullptr;
  vk::BufferImageCopy m_Region;
  vk::DeviceSize m_uiTotalSize;
};
