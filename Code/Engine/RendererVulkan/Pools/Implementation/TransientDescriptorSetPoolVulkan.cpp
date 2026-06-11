#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Pools/TransientDescriptorSetPoolVulkan.h>

vk::DescriptorPool ezTransientDescriptorSetPoolVulkan::s_CurrentTransientPool;
ezHybridArray<vk::DescriptorPool, 4> ezTransientDescriptorSetPoolVulkan::s_FreeTransientPools;
vk::Device ezTransientDescriptorSetPoolVulkan::s_Device;
ezHashTable<vk::DescriptorType, float> ezTransientDescriptorSetPoolVulkan::s_DescriptorWeights;

void ezTransientDescriptorSetPoolVulkan::Initialize(vk::Device device)
{
  s_Device = device;
  s_DescriptorWeights[vk::DescriptorType::eSampler] = 0.5f;              // Image sampler.
  s_DescriptorWeights[vk::DescriptorType::eSampledImage] = 2.0f;         // Read-only image view.
  s_DescriptorWeights[vk::DescriptorType::eStorageImage] = 1.0f;         // Read / write image view.
  s_DescriptorWeights[vk::DescriptorType::eUniformBuffer] = 1.0f;        // Read-only struct (constant buffer)
  s_DescriptorWeights[vk::DescriptorType::eStorageBuffer] = 1.0f;        // Read / write struct (UAV).
  s_DescriptorWeights[vk::DescriptorType::eUniformTexelBuffer] = 0.5f;   // Read-only linear texel buffer with view.
  s_DescriptorWeights[vk::DescriptorType::eCombinedImageSampler] = 2.0f; // Read-only image view with image sampler.

  // Not used by EZ so far.
  s_DescriptorWeights[vk::DescriptorType::eStorageTexelBuffer] = 0.5f;   // Read / write linear texel buffer with view.
  s_DescriptorWeights[vk::DescriptorType::eUniformBufferDynamic] = 1.0f; // Same as eUniformBuffer but allows updating the memory offset into the buffer dynamically.
  s_DescriptorWeights[vk::DescriptorType::eStorageBufferDynamic] = 0.0f; // Same as eStorageBuffer but allows updating the memory offset into the buffer dynamically.

  // Not supported by EZ so far.
  s_DescriptorWeights[vk::DescriptorType::eInputAttachment] = 0.0f; // frame-buffer local read-only image view.

  s_DescriptorWeights[vk::DescriptorType::eInlineUniformBlock] = 0.0f;
  s_DescriptorWeights[vk::DescriptorType::eAccelerationStructureKHR] = 0.0f;
  s_DescriptorWeights[vk::DescriptorType::eAccelerationStructureNV] = 0.0f;
  s_DescriptorWeights[vk::DescriptorType::eMutableVALVE] = 0.0f;
}

void ezTransientDescriptorSetPoolVulkan::DeInitialize()
{
  s_DescriptorWeights.Clear();
  s_DescriptorWeights.Compact();

  for (vk::DescriptorPool& pool : s_FreeTransientPools)
  {
    s_Device.destroyDescriptorPool(pool, nullptr);
  }
  s_FreeTransientPools.Clear();
  s_FreeTransientPools.Compact();
  if (s_CurrentTransientPool)
  {
    s_Device.resetDescriptorPool(s_CurrentTransientPool);
    s_Device.destroyDescriptorPool(s_CurrentTransientPool, nullptr);
    s_CurrentTransientPool = nullptr;
  }

  s_Device = nullptr;
}

ezHashTable<vk::DescriptorType, float>& ezTransientDescriptorSetPoolVulkan::AccessDescriptorPoolWeights()
{
  return s_DescriptorWeights;
}

vk::DescriptorSet ezTransientDescriptorSetPoolVulkan::CreateTransientDescriptorSet(vk::DescriptorSetLayout layout)
{
  vk::DescriptorSet set;
  if (!s_CurrentTransientPool)
  {
    s_CurrentTransientPool = GetNewTransientPool();
  }

  vk::DescriptorSetAllocateInfo allocateInfo;
  allocateInfo.pSetLayouts = &layout;
  allocateInfo.descriptorPool = s_CurrentTransientPool;
  allocateInfo.descriptorSetCount = 1;

  vk::Result res = s_Device.allocateDescriptorSets(&allocateInfo, &set);
  bool bPoolExhausted = false;

  switch (res)
  {
    case vk::Result::eSuccess:
      break;
    case vk::Result::eErrorFragmentedPool:
    case vk::Result::eErrorOutOfPoolMemory:
      bPoolExhausted = true;
      break;
    default:
      VK_ASSERT_DEV(res);
      break;
  }

  if (bPoolExhausted)
  {
    ezGALDeviceVulkan* pDevice = static_cast<ezGALDeviceVulkan*>(ezGALDevice::GetDefaultDevice());
    pDevice->ReclaimLater(s_CurrentTransientPool);
    s_CurrentTransientPool = GetNewTransientPool();
    allocateInfo.descriptorPool = s_CurrentTransientPool;
    VK_ASSERT_DEV(s_Device.allocateDescriptorSets(&allocateInfo, &set));
  }

  return set;
}

void ezTransientDescriptorSetPoolVulkan::UpdateDescriptorSet(vk::DescriptorSet descriptorSet, ezArrayPtr<vk::WriteDescriptorSet> update)
{
  s_Device.updateDescriptorSets(update.GetCount(), update.GetPtr(), 0, nullptr);
}

void ezTransientDescriptorSetPoolVulkan::ReclaimPool(vk::DescriptorPool& ref_descriptorPool)
{
  s_Device.resetDescriptorPool(ref_descriptorPool);
  s_FreeTransientPools.PushBack(ref_descriptorPool);
}

vk::DescriptorPool ezTransientDescriptorSetPoolVulkan::GetNewTransientPool()
{
  if (s_FreeTransientPools.IsEmpty())
  {
    ezHybridArray<vk::DescriptorPoolSize, 20> poolSizes;
    for (auto weight : s_DescriptorWeights)
    {
      if (static_cast<ezUInt32>(weight.Value() * s_uiPoolBaseSize) > 0)
        poolSizes.PushBack(vk::DescriptorPoolSize(weight.Key(), static_cast<ezUInt32>(weight.Value() * s_uiPoolBaseSize)));
    }
    vk::DescriptorPoolCreateInfo poolCreateInfo;
    poolCreateInfo.flags = {};
    poolCreateInfo.maxSets = s_uiPoolBaseSize;
    poolCreateInfo.poolSizeCount = poolSizes.GetCount();
    poolCreateInfo.pPoolSizes = poolSizes.GetData();

    vk::DescriptorPool descriptorPool;
    VK_ASSERT_DEV(s_Device.createDescriptorPool(&poolCreateInfo, nullptr, &descriptorPool));
    return descriptorPool;
  }
  else
  {
    vk::DescriptorPool pool = s_FreeTransientPools.PeekBack();
    s_FreeTransientPools.PopBack();
    return pool;
  }
}