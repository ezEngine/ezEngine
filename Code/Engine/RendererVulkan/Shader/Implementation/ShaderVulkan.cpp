#include <RendererVulkan/RendererVulkanPCH.h>

#include <Foundation/Algorithm/HashStream.h>
#include <RendererFoundation/Device/ImmutableSamplers.h>
#include <RendererVulkan/Cache/ResourceCacheVulkan.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Shader/ShaderVulkan.h>
#include <RendererVulkan/State/StateVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>

void ezGALShaderVulkan::DescriptorSetLayoutDesc::ComputeHash()
{
  ezHashStreamWriter32 writer;
  const ezUInt32 uiSize = m_bindings.GetCount();
  for (ezUInt32 i = 0; i < uiSize; i++)
  {
    const auto& binding = m_bindings[i];
    writer << binding.binding;
    writer << ezConversionUtilsVulkan::GetUnderlyingValue(binding.descriptorType);
    writer << binding.descriptorCount;
    writer << ezConversionUtilsVulkan::GetUnderlyingFlagsValue(binding.stageFlags);
    writer << binding.pImmutableSamplers;
  }
  m_uiHash = writer.GetHashValue();
}

ezGALShaderVulkan::ezGALShaderVulkan(const ezGALShaderCreationDescription& Description)
  : ezGALShader(Description)
{
}

ezGALShaderVulkan::~ezGALShaderVulkan() {}

void ezGALShaderVulkan::SetDebugName(ezStringView sName) const
{
  ezStringBuilder tmp;

  ezGALDeviceVulkan* pVulkanDevice = static_cast<ezGALDeviceVulkan*>(ezGALDevice::GetDefaultDevice());
  for (ezUInt32 i = 0; i < ezGALShaderStage::ENUM_COUNT; i++)
  {
    pVulkanDevice->SetDebugName(sName.GetData(tmp), m_Shaders[i]);
  }
}

ezResult ezGALShaderVulkan::InitPlatform(ezGALDevice* pDevice)
{
  EZ_SUCCEED_OR_RETURN(CreateBindingMapping(false));

  ezGALDeviceVulkan* pVulkanDevice = static_cast<ezGALDeviceVulkan*>(pDevice);

  m_SetBindings.Clear();

  for (const ezShaderResourceBinding& binding : GetBindingMapping())
  {
    if (binding.m_ResourceType == ezGALShaderResourceType::PushConstants)
      continue;

    ezUInt32 iMaxSets = ezMath::Max((ezUInt32)m_SetBindings.GetCount(), static_cast<ezUInt32>(binding.m_iSet + 1));
    m_SetBindings.SetCount(iMaxSets);
    m_SetBindings[binding.m_iSet].PushBack(binding);
  }

  const ezGALImmutableSamplers::ImmutableSamplers& immutableSamplers = ezGALImmutableSamplers::GetImmutableSamplers();
  auto GetImmutableSampler = [&](const ezHashedString& sName) -> const vk::Sampler*
  {
    if (const ezGALSamplerStateHandle* hSampler = immutableSamplers.GetValue(sName))
    {
      const auto* pSampler = static_cast<const ezGALSamplerStateVulkan*>(pVulkanDevice->GetSamplerState(*hSampler));
      return &pSampler->GetImageInfo().sampler;
    }
    return nullptr;
  };

  // If no descriptor set is needed, we still need to create an empty one to fulfil the Vulkan spec :-/
  if (m_SetBindings.IsEmpty())
  {
    m_SetBindings.SetCount(1);
  }

  // Sort mappings and build descriptor set layout
  ezHybridArray<DescriptorSetLayoutDesc, 4> descriptorSetLayoutDesc;
  descriptorSetLayoutDesc.SetCount(m_SetBindings.GetCount());
  m_descriptorSetLayout.SetCount(m_SetBindings.GetCount());
  for (ezUInt32 iSet = 0; iSet < m_SetBindings.GetCount(); ++iSet)
  {
    m_SetBindings[iSet].Sort([](const ezShaderResourceBinding& lhs, const ezShaderResourceBinding& rhs)
      { return lhs.m_iSlot < rhs.m_iSlot; });

    // Build Vulkan descriptor set layout
    for (ezUInt32 i = 0; i < m_SetBindings[iSet].GetCount(); i++)
    {
      const ezShaderResourceBinding& ezBinding = m_SetBindings[iSet][i];
      vk::DescriptorSetLayoutBinding& binding = descriptorSetLayoutDesc[ezBinding.m_iSet].m_bindings.ExpandAndGetRef();

      binding.binding = ezBinding.m_iSlot;
      binding.descriptorType = ezConversionUtilsVulkan::GetDescriptorType(ezBinding.m_ResourceType);
      binding.descriptorCount = ezBinding.m_uiArraySize;
      binding.stageFlags = ezConversionUtilsVulkan::GetShaderStages(ezBinding.m_Stages);
      binding.pImmutableSamplers = ezBinding.m_ResourceType == ezGALShaderResourceType::Sampler ? GetImmutableSampler(ezBinding.m_sName) : nullptr;
    }

    descriptorSetLayoutDesc[iSet].ComputeHash();
    m_descriptorSetLayout[iSet] = ezResourceCacheVulkan::RequestDescriptorSetLayout(descriptorSetLayoutDesc[iSet]);
  }

  // Remove immutable samplers and push constants from binding info
  {
    for (ezUInt32 uiSet = 0; uiSet < m_SetBindings.GetCount(); ++uiSet)
    {
      for (ezInt32 iIndex = (ezInt32)m_SetBindings[uiSet].GetCount() - 1; iIndex >= 0; --iIndex)
      {
        const bool bIsImmutableSample = m_SetBindings[uiSet][iIndex].m_ResourceType == ezGALShaderResourceType::Sampler && immutableSamplers.Contains(m_SetBindings[uiSet][iIndex].m_sName);

        if (bIsImmutableSample)
        {
          m_SetBindings[uiSet].RemoveAtAndCopy(iIndex);
        }
      }
    }
    for (ezInt32 iIndex = (ezInt32)m_BindingMapping.GetCount() - 1; iIndex >= 0; --iIndex)
    {
      const bool bIsImmutableSample = m_BindingMapping[iIndex].m_ResourceType == ezGALShaderResourceType::Sampler && immutableSamplers.Contains(m_BindingMapping[iIndex].m_sName);
      const bool bIsPushConstant = m_BindingMapping[iIndex].m_ResourceType == ezGALShaderResourceType::PushConstants;

      if (bIsPushConstant)
      {
        const auto& pushConstant = m_BindingMapping[iIndex];
        m_pushConstants.size = pushConstant.m_pLayout->m_uiTotalSize;
        m_pushConstants.offset = 0;
        m_pushConstants.stageFlags = ezConversionUtilsVulkan::GetShaderStages(pushConstant.m_Stages);
      }

      if (bIsImmutableSample || bIsPushConstant)
      {
        m_BindingMapping.RemoveAtAndCopy(iIndex);
      }
    }
  }

  // Build shaders
  vk::ShaderModuleCreateInfo createInfo;
  for (ezUInt32 i = 0; i < ezGALShaderStage::ENUM_COUNT; i++)
  {
    if (m_Description.HasByteCodeForStage((ezGALShaderStage::Enum)i))
    {
      createInfo.codeSize = m_Description.m_ByteCodes[i]->m_ByteCode.GetCount();
      EZ_ASSERT_DEV(createInfo.codeSize % 4 == 0, "Spirv shader code should be a multiple of 4.");
      createInfo.pCode = reinterpret_cast<const ezUInt32*>(m_Description.m_ByteCodes[i]->m_ByteCode.GetData());
      VK_SUCCEED_OR_RETURN_EZ_FAILURE(pVulkanDevice->GetVulkanDevice().createShaderModule(&createInfo, nullptr, &m_Shaders[i]));
    }
  }

  // Build pipeline Layout
  {
    ezResourceCacheVulkan::PipelineLayoutDesc m_LayoutDesc;
    const ezUInt32 uiSets = GetSetCount();
    m_LayoutDesc.m_layout.SetCount(uiSets);
    m_LayoutDesc.m_pushConstants = GetPushConstantRange();
    for (ezUInt32 uiSet = 0; uiSet < uiSets; ++uiSet)
    {
      m_LayoutDesc.m_layout[uiSet] = GetDescriptorSetLayout(uiSet);
    }

    m_PipelineLayout = ezResourceCacheVulkan::RequestPipelineLayout(m_LayoutDesc);
  }
  return EZ_SUCCESS;
}

ezResult ezGALShaderVulkan::DeInitPlatform(ezGALDevice* pDevice)
{
  DestroyBindingMapping();

  // Right now, we do not destroy descriptor set layouts as they are shared among many shaders.
  m_descriptorSetLayout.Clear();
  m_SetBindings.Clear();

  auto* pVulkanDevice = static_cast<ezGALDeviceVulkan*>(pDevice);
  for (auto& m_Shader : m_Shaders)
  {
    pVulkanDevice->DeleteLater(m_Shader);
  }
  return EZ_SUCCESS;
}
