

#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererFoundation/Device/ImmutableSamplers.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Pools/DescriptorSetPoolVulkan.h>
#include <RendererVulkan/Shader/BindGroupLayoutVulkan.h>
#include <RendererVulkan/State/StateVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>

ezGALBindGroupLayoutVulkan::ezGALBindGroupLayoutVulkan(const ezGALBindGroupLayoutCreationDescription& Description)
  : ezGALBindGroupLayout(Description)
{
}

ezGALBindGroupLayoutVulkan::~ezGALBindGroupLayoutVulkan() = default;

ezResult ezGALBindGroupLayoutVulkan::InitPlatform(ezGALDevice* pDevice)
{
  ezGALDeviceVulkan* pVulkanDevice = static_cast<ezGALDeviceVulkan*>(pDevice);

  ezHybridArray<vk::DescriptorSetLayoutBinding, 6> bindings;
  bindings.Reserve(m_Description.m_ResourceBindings.GetCount() + m_Description.m_ImmutableSamplers.GetCount());

  auto ConvertBinding = [](const ezShaderResourceBinding& ezBinding, vk::DescriptorSetLayoutBinding& out_Binding)
  {
    out_Binding.binding = ezBinding.m_iSlot;
    out_Binding.descriptorType = ezConversionUtilsVulkan::GetDescriptorType(ezBinding.m_ResourceType);
    out_Binding.descriptorCount = ezBinding.m_uiArraySize;
    out_Binding.stageFlags = ezConversionUtilsVulkan::GetShaderStages(ezBinding.m_Stages);
  };

  // Build Vulkan descriptor set layout
  for (ezUInt32 i = 0; i < m_Description.m_ResourceBindings.GetCount(); i++)
  {
    const ezShaderResourceBinding& ezBinding = m_Description.m_ResourceBindings[i];
    vk::DescriptorSetLayoutBinding& binding = bindings.ExpandAndGetRef();
    ConvertBinding(ezBinding, binding);
  }

  // Build m_ResourceUsage
  for (ezUInt32 i = 0; i < m_Description.m_ResourceBindings.GetCount(); i++)
  {
    const ezShaderResourceBinding& ezBinding = m_Description.m_ResourceBindings[i];
    m_ResourceUsage.m_Usage[ezBinding.m_ResourceType.GetValue()]++;
  }

  const ezGALImmutableSamplers::ImmutableSamplers& immutableSamplers = ezGALImmutableSamplers::GetImmutableSamplers();

  for (ezUInt32 i = 0; i < m_Description.m_ImmutableSamplers.GetCount(); i++)
  {
    const ezShaderResourceBinding& ezBinding = m_Description.m_ImmutableSamplers[i];
    vk::DescriptorSetLayoutBinding& binding = bindings.ExpandAndGetRef();
    ConvertBinding(ezBinding, binding);
    if (const ezGALSamplerStateHandle* hSampler = immutableSamplers.GetValue(ezBinding.m_sName))
    {
      const auto* pSampler = static_cast<const ezGALSamplerStateVulkan*>(pVulkanDevice->GetSamplerState(*hSampler));
      binding.pImmutableSamplers = &pSampler->GetImageInfo().sampler;
    }
    else
    {
      ezLog::Error("Immutable sampler '{}' not found, failed to create bind group layout", ezBinding.m_sName.GetData());
      return EZ_FAILURE;
    }
  }

  vk::DescriptorSetLayoutCreateInfo descriptorSetLayout;
  descriptorSetLayout.bindingCount = bindings.GetCount();
  descriptorSetLayout.pBindings = bindings.GetData();
  VK_ASSERT_DEBUG(pVulkanDevice->GetVulkanDevice().createDescriptorSetLayout(&descriptorSetLayout, nullptr, &m_DescriptorSetLayout));

  m_DescriptorSetPool = ezDescriptorSetPoolVulkan::GetPool(m_ResourceUsage);

  return EZ_SUCCESS;
}

ezResult ezGALBindGroupLayoutVulkan::DeInitPlatform(ezGALDevice* pDevice)
{
  m_DescriptorSetPool = nullptr;
  auto* pVulkanDevice = static_cast<ezGALDeviceVulkan*>(pDevice);
  pVulkanDevice->DeleteLater(m_DescriptorSetLayout);
  return EZ_SUCCESS;
}

ezDescriptorSetPoolVulkan* ezGALBindGroupLayoutVulkan::GetDescriptorSetPool() const
{
  return m_DescriptorSetPool.Borrow();
}
