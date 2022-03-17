#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <vulkan/vulkan.hpp>

/// \brief Simple pool for fences
///
/// Do not call ReclaimFence manually, instead call ezGALDeviceVulkan::ReclaimLater which will make sure to reclaim the fence once it is no longer in use.
/// Fences are reclaimed once the frame in ezGALDeviceVulkan is reused (currently 4 frames are in rotation). Do not call resetFences, this is already done by ReclaimFence.
/// Usage:
/// \code{.cpp}
///   vk::Fence f = ezFencePoolVulkan::RequestFence();
///   <insert fence somewhere>
///   <wait for fence>
///   ezGALDeviceVulkan* pDevice = ...;
///   pDevice->ReclaimLater(f);
/// \endcode
class EZ_RENDERERVULKAN_DLL ezFencePoolVulkan
{
public:
  static void Initialize(vk::Device device);
  static void DeInitialize();

  static vk::Fence RequestFence();
  static void ReclaimFence(vk::Fence& fence);

private:
  static ezHybridArray<vk::Fence, 4> s_Fences;
  static vk::Device s_device;
};
