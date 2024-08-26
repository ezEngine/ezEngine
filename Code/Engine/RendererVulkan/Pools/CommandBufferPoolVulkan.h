#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <vulkan/vulkan.hpp>

/// \brief Simple pool for command buffers
///
/// Do not call ReclaimCommandBuffer manually, instead call ezGALDeviceVulkan::ReclaimLater which will make sure to reclaim the command buffer once it is no longer in use.
/// Usage:
/// \code{.cpp}
///   vk::CommandBuffer c = pPool->RequestCommandBuffer();
///   c.begin();
///   ...
///   c.end();
///   ezGALDeviceVulkan* pDevice = ...;
///   pDevice->ReclaimLater(c);
/// \endcode
class EZ_RENDERERVULKAN_DLL ezCommandBufferPoolVulkan
{
public:
  ezCommandBufferPoolVulkan(ezAllocator* pAllocator);
  ~ezCommandBufferPoolVulkan();

  void Initialize(vk::Device device, ezUInt32 graphicsFamilyIndex);
  void DeInitialize();

  vk::CommandBuffer RequestCommandBuffer();
  void ReclaimCommandBuffer(vk::CommandBuffer& CommandBuffer);

private:
  vk::Device m_device;
  vk::CommandPool m_commandPool;
  ezHybridArray<vk::CommandBuffer, 4> m_CommandBuffers;
};
