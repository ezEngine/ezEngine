#pragma once

#include <RendererFoundation/Descriptors/Enumerations.h>
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

/// \brief Used as a key to descriptor allocators. Defines the resource usage of each type inside a bind group layout.
struct ezBindGroupLayoutResourceUsageVulkan
{
  ezUInt8 m_Usage[ezGALShaderResourceType::COUNT] = {0};
};