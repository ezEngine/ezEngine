#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <vulkan/vulkan.hpp>

/// \brief Simple pool for semaphores
///
/// Do not call ReclaimSemaphore manually, instead call ezGALDeviceVulkan::ReclaimLater which will make sure to reclaim the semaphore once it is no longer in use.
/// Usage:
/// \code{.cpp}
///   vk::Semaphore s = ezSemaphorePoolVulkan::RequestSemaphore();
///   ...
///   ezGALDeviceVulkan* pDevice = ...;
///   pDevice->ReclaimLater(s);
/// \endcode
class EZ_RENDERERVULKAN_DLL ezSemaphorePoolVulkan
{
public:
  static void Initialize(vk::Device device);
  static void DeInitialize();

  static vk::Semaphore RequestSemaphore();
  static void ReclaimSemaphore(vk::Semaphore& semaphore);

private:
  static ezHybridArray<vk::Semaphore, 4> s_semaphores;
  static vk::Device s_device;
};
