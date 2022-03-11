#include <RendererVulkan/RendererVulkanPCH.h>

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
  VK_ASSERT_DEV(fenceStatus);
  s_device.resetFences(1, &fence);
  EZ_ASSERT_DEBUG(s_device, "ezFencePoolVulkan::Initialize not called");
  s_Fences.PushBack(fence);
}
