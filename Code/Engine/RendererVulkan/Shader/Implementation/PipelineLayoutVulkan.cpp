#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Shader/BindGroupLayoutVulkan.h>
#include <RendererVulkan/Shader/PipelineLayoutVulkan.h>

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
  for (ezUInt32 i = 1; i < EZ_GAL_MAX_SETS; ++i)
  {
    if (!m_Description.m_BindGroups[i].IsInvalidated())
    {
      uiMaxSets = i + 1;
    }
  }

  ezHybridArray<vk::DescriptorSetLayout, EZ_GAL_MAX_SETS> descriptorSetLayouts;
  descriptorSetLayouts.SetCount(uiMaxSets);
  for (ezUInt32 i = 0; i < uiMaxSets; ++i)
  {
    const ezGALBindGroupLayout* pBindGroupLayout = pVulkanDevice->GetBindGroupLayout(m_Description.m_BindGroups[i]);
    descriptorSetLayouts[i] = static_cast<const ezGALBindGroupLayoutVulkan*>(pBindGroupLayout)->GetDescriptorSetLayout();
  }

  m_pushConstants.size = m_Description.m_PushConstants.m_uiSize;
  m_pushConstants.offset = m_Description.m_PushConstants.m_uiOffset;
  m_pushConstants.stageFlags = ezConversionUtilsVulkan::GetShaderStages(m_Description.m_PushConstants.m_Stages);

  vk::PipelineLayoutCreateInfo layoutInfo;
  layoutInfo.setLayoutCount = descriptorSetLayouts.GetCount();
  layoutInfo.pSetLayouts = descriptorSetLayouts.GetData();
  if (m_pushConstants.size != 0)
  {
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &m_pushConstants;
  }

  VK_ASSERT_DEBUG(pVulkanDevice->GetVulkanDevice().createPipelineLayout(&layoutInfo, nullptr, &m_PipelineLayout));

  return EZ_SUCCESS;
}

ezResult ezGALPipelineLayoutVulkan::DeInitPlatform(ezGALDevice* pDevice)
{
  auto* pVulkanDevice = static_cast<ezGALDeviceVulkan*>(pDevice);
  pVulkanDevice->DeleteLater(m_PipelineLayout);
  return EZ_SUCCESS;
}
