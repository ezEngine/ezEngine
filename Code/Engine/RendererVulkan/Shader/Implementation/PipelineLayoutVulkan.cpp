#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Shader/BindGroupLayoutVulkan.h>
#include <RendererVulkan/Shader/PipelineLayoutVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>

ezGALPipelineLayoutVulkan::ezGALPipelineLayoutVulkan(const ezGALPipelineLayoutCreationDescription& Description)
  : ezGALPipelineLayout(Description)
{
}

ezGALPipelineLayoutVulkan::~ezGALPipelineLayoutVulkan() = default;

ezResult ezGALPipelineLayoutVulkan::InitPlatform(ezGALDevice* pDevice)
{
  ezGALDeviceVulkan* pVulkanDevice = static_cast<ezGALDeviceVulkan*>(pDevice);

  // There must always be at least one empty set.
  ezUInt32 uiMaxSets = 1;
  for (ezUInt32 i = 1; i < EZ_GAL_MAX_BIND_GROUPS; ++i)
  {
    if (!m_Description.m_BindGroups[i].IsInvalidated())
    {
      uiMaxSets = i + 1;
    }
  }

  ezHybridArray<vk::DescriptorSetLayout, EZ_GAL_MAX_BIND_GROUPS> descriptorSetLayouts;
  descriptorSetLayouts.SetCount(uiMaxSets);
  for (ezUInt32 i = 0; i < uiMaxSets; ++i)
  {
    const ezGALBindGroupLayout* pBindGroupLayout = pVulkanDevice->GetBindGroupLayout(m_Description.m_BindGroups[i]);
    if (pBindGroupLayout == nullptr)
    {
      // Vulkan needs a valid (possibly empty) layout for every set in the range, gaps cannot be expressed.
      ezLog::Error("Failed to create Vulkan pipeline layout: no bind group layout for set {} of {}.", i, uiMaxSets);
      return EZ_FAILURE;
    }
    descriptorSetLayouts[i] = static_cast<const ezGALBindGroupLayoutVulkan*>(pBindGroupLayout)->GetDescriptorSetLayout();
  }

  m_PushConstants.size = m_Description.m_PushConstants.m_uiSize;
  m_PushConstants.offset = m_Description.m_PushConstants.m_uiOffset;
  m_PushConstants.stageFlags = ezConversionUtilsVulkan::GetShaderStages(m_Description.m_PushConstants.m_Stages);

  vk::PipelineLayoutCreateInfo layoutInfo;
  layoutInfo.setLayoutCount = descriptorSetLayouts.GetCount();
  layoutInfo.pSetLayouts = descriptorSetLayouts.GetData();
  if (m_PushConstants.size != 0)
  {
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &m_PushConstants;
  }

  VK_SUCCEED_OR_RETURN_EZ_FAILURE(pVulkanDevice->GetVulkanDevice().createPipelineLayout(&layoutInfo, nullptr, &m_PipelineLayout));

  return EZ_SUCCESS;
}

ezResult ezGALPipelineLayoutVulkan::DeInitPlatform(ezGALDevice* pDevice)
{
  auto* pVulkanDevice = static_cast<ezGALDeviceVulkan*>(pDevice);
  pVulkanDevice->DeleteLater(m_PipelineLayout);
  return EZ_SUCCESS;
}
