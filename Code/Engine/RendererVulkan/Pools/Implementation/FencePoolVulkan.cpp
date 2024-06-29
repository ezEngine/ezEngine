#include <RendererVulkan/RendererVulkanPCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Pools/FencePoolVulkan.h>

vk::Device ezFencePoolVulkan::s_device;
ezHybridArray<vk::Fence, 4> ezFencePoolVulkan::s_Fences;

void ezFencePoolVulkan::Initialize(vk::Device device)
{
  s_device = device;
}

void ezFencePoolVulkan::DeInitialize()
{
  for (vk::Fence& fence : s_Fences)
  {
    s_device.destroyFence(fence, nullptr);
  }
  s_Fences.Clear();
  s_Fences.Compact();

  s_device = nullptr;
}

vk::Fence ezFencePoolVulkan::RequestFence()
{
  EZ_ASSERT_DEBUG(s_device, "ezFencePoolVulkan::Initialize not called");
  if (!s_Fences.IsEmpty())
  {
    vk::Fence Fence = s_Fences.PeekBack();
    s_Fences.PopBack();
    return Fence;
  }
  else
  {
    vk::Fence fence;
    vk::FenceCreateInfo createInfo = {};
    VK_ASSERT_DEV(s_device.createFence(&createInfo, nullptr, &fence));
    return fence;
  }
}

void ezFencePoolVulkan::ReclaimFence(vk::Fence& fence)
{
  vk::Result fenceStatus = s_device.getFenceStatus(fence);
  if (fenceStatus == vk::Result::eNotReady)
  {
    // #TODO_VULKAN Workaround for fences that were waited for (and thus signaled) returning VK_NOT_READY if AMDs profiler is active.
    // The fence will simply take another round through the reclaim process and will eventually turn signaled.
    static_cast<ezGALDeviceVulkan*>(ezGALDevice::GetDefaultDevice())->ReclaimLater(fence);
    return;
  }
  VK_ASSERT_DEV(fenceStatus);
  s_device.resetFences(1, &fence);
  EZ_ASSERT_DEBUG(s_device, "ezFencePoolVulkan::Initialize not called");
  s_Fences.PushBack(fence);
}


ezFenceQueueVulkan::ezFenceQueueVulkan(vk::Device device)
  : m_device(device)
{
}

ezFenceQueueVulkan::~ezFenceQueueVulkan()
{
  while (!m_PendingFences.IsEmpty())
  {
    WaitForNextFence(ezTime::MakeFromHours(1));
  }
}

ezGALFenceHandle ezFenceQueueVulkan::GetCurrentFenceHandle()
{
  return m_uiCurrentFenceCounter;
}

void ezFenceQueueVulkan::FenceSubmitted(vk::Fence vkFence)
{
  m_PendingFences.PushBack({vkFence, m_uiCurrentFenceCounter});
  m_uiCurrentFenceCounter++;
}

void ezFenceQueueVulkan::FlushReadyFences()
{
  while (!m_PendingFences.IsEmpty())
  {
    if (WaitForNextFence() == ezGALAsyncResult::Pending)
      return;
  }
}

ezEnum<ezGALAsyncResult> ezFenceQueueVulkan::GetFenceResult(ezGALFenceHandle hFence, ezTime timeout /*= ezTime::MakeZero()*/)
{
  if (hFence <= m_uiReachedFenceCounter)
    return ezGALAsyncResult::Ready;

  EZ_ASSERT_DEBUG(hFence <= m_uiCurrentFenceCounter, "Invalid fence handle");

  while (!m_PendingFences.IsEmpty() && m_PendingFences[0].m_hFence <= hFence)
  {
    ezTimestamp start = ezTimestamp::CurrentTimestamp();
    ezEnum<ezGALAsyncResult> res = WaitForNextFence(timeout);
    if (res == ezGALAsyncResult::Pending)
      return res;

    ezTimestamp end = ezTimestamp::CurrentTimestamp();
    timeout -= (end - start);
  }

  return hFence <= m_uiReachedFenceCounter ? ezGALAsyncResult::Ready : ezGALAsyncResult::Pending;
}

ezEnum<ezGALAsyncResult> ezFenceQueueVulkan::WaitForNextFence(ezTime timeout /*= ezTime::MakeZero()*/)
{
  vk::Result fenceStatus;
  {
    EZ_PROFILE_SCOPE("getFenceStatus");
    fenceStatus = m_device.getFenceStatus(m_PendingFences[0].m_vkFence);
  }
  if (fenceStatus == vk::Result::eSuccess)
  {
    m_uiReachedFenceCounter = m_PendingFences[0].m_hFence;
    m_PendingFences.PopFront();
    return ezGALAsyncResult::Ready;
  }

  EZ_ASSERT_DEBUG(fenceStatus == vk::Result::eNotReady, "getFenceStatus returned {}", vk::to_string(fenceStatus).c_str());
  if (fenceStatus == vk::Result::eNotReady && timeout > ezTime::MakeZero())
  {
    fenceStatus = m_device.waitForFences(1, &m_PendingFences[0].m_vkFence, true, static_cast<ezUInt64>(timeout.GetNanoseconds()));
    if (fenceStatus == vk::Result::eTimeout)
      return ezGALAsyncResult::Pending;

    EZ_ASSERT_DEBUG(fenceStatus == vk::Result::eSuccess, "waitForFences returned {}", vk::to_string(fenceStatus).c_str());

    m_uiReachedFenceCounter = m_PendingFences[0].m_hFence;
    m_PendingFences.PopFront();
  }
  return ezGALAsyncResult::Pending;
}
