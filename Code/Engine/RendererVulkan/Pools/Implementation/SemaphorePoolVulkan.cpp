#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Pools/SemaphorePoolVulkan.h>

vk::Device ezSemaphorePoolVulkan::s_Device;
ezHybridArray<vk::Semaphore, 4> ezSemaphorePoolVulkan::s_Semaphores;

void ezSemaphorePoolVulkan::Initialize(vk::Device device)
{
  s_Device = device;
}

void ezSemaphorePoolVulkan::DeInitialize()
{
  for (vk::Semaphore& semaphore : s_Semaphores)
  {
    s_Device.destroySemaphore(semaphore, nullptr);
  }
  s_Semaphores.Clear();
  s_Semaphores.Compact();

  s_Device = nullptr;
}

vk::Semaphore ezSemaphorePoolVulkan::RequestSemaphore()
{
  EZ_ASSERT_DEBUG(s_Device, "ezSemaphorePoolVulkan::Initialize not called");
  if (!s_Semaphores.IsEmpty())
  {
    vk::Semaphore semaphore = s_Semaphores.PeekBack();
    s_Semaphores.PopBack();
    return semaphore;
  }
  else
  {
    vk::Semaphore semaphore;
    vk::SemaphoreCreateInfo semaphoreCreateInfo;
    VK_ASSERT_DEV(s_Device.createSemaphore(&semaphoreCreateInfo, nullptr, &semaphore));
    return semaphore;
  }
}

void ezSemaphorePoolVulkan::ReclaimSemaphore(vk::Semaphore& ref_semaphore)
{
  EZ_ASSERT_DEBUG(s_Device, "ezSemaphorePoolVulkan::Initialize not called");
  s_Semaphores.PushBack(ref_semaphore);
}
