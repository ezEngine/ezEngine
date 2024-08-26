#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <Foundation/Time/Timestamp.h>

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

// #TODO_VULKAN extend to support multiple queues.
class EZ_RENDERERVULKAN_DLL ezFenceQueueVulkan
{
public:
  ezFenceQueueVulkan(ezGALDeviceVulkan* pDevice);
  ~ezFenceQueueVulkan();

  ezGALFenceHandle GetCurrentFenceHandle();
  void FenceSubmitted(vk::Fence vkFence);
  void FlushReadyFences();
  ezEnum<ezGALAsyncResult> GetFenceResult(ezGALFenceHandle hFence, ezTime timeout = ezTime::MakeZero());

private:
  ezEnum<ezGALAsyncResult> WaitForNextFence(ezTime timeout = ezTime::MakeZero());

private:
  struct PendingFence
  {
    vk::Fence m_vkFence;
    ezGALFenceHandle m_hFence;
  };
  ezDeque<PendingFence> m_PendingFences;
  ezUInt64 m_uiCurrentFenceCounter = 1;
  ezUInt64 m_uiReachedFenceCounter = 0;
  vk::Device m_device;
};
