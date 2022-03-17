#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <vulkan/vulkan.hpp>

/// \brief Simple pool for command buffers
///
/// Do not call ReclaimCommandBuffer manually, instead call ezGALDeviceVulkan::ReclaimLater which will make sure to reclaim the command buffer once it is no longer in use.
/// Usage:
/// \code{.cpp}
///   vk::CommandBuffer c = ezCommandBufferPoolVulkan::RequestCommandBuffer();
///   c.begin();
///   ...
///   c.end();
///   ezGALDeviceVulkan* pDevice = ...;
///   pDevice->ReclaimLater(c);
/// \endcode
class EZ_RENDERERVULKAN_DLL ezCommandBufferPoolVulkan
{
public:
  static void Initialize(vk::Device device, ezUInt32 graphicsQueueIndex);
  static void DeInitialize();

  static vk::CommandBuffer RequestCommandBuffer();
  static void ReclaimCommandBuffer(vk::CommandBuffer& CommandBuffer);

private:
  static vk::Device s_device;
  static vk::CommandPool s_commandPool;
  static ezHybridArray<vk::CommandBuffer, 4> s_CommandBuffers;
};
