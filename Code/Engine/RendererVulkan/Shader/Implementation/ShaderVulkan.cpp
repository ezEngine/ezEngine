#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Cache/ResourceCacheVulkan.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Shader/BindGroupLayoutVulkan.h>
#include <RendererVulkan/Shader/PipelineLayoutVulkan.h>
#include <RendererVulkan/Shader/ShaderVulkan.h>

ezGALShaderVulkan::ezGALShaderVulkan(const ezGALShaderCreationDescription& Description)
  : ezGALShader(Description)
{
}

ezGALShaderVulkan::~ezGALShaderVulkan() {}

void ezGALShaderVulkan::SetDebugName(ezStringView sName) const
{
  ezStringBuilder tmp;
  for (ezUInt32 i = 0; i < ezGALShaderStage::ENUM_COUNT; i++)
  {
    static_cast<ezGALDeviceVulkan*>(m_pDevice)->SetDebugName(sName.GetData(tmp), m_Shaders[i]);
  }
}

ezResult ezGALShaderVulkan::InitPlatform(ezGALDevice* pDevice)
{
  m_pDevice = pDevice;
  EZ_SUCCEED_OR_RETURN(CreateBindingMapping(false));
  EZ_SUCCEED_OR_RETURN(CreateLayouts(pDevice, true));

  auto pDeviceVulkan = static_cast<ezGALDeviceVulkan*>(pDevice);

  // Build shaders
  vk::ShaderModuleCreateInfo createInfo;
  for (ezUInt32 i = 0; i < ezGALShaderStage::ENUM_COUNT; i++)
  {
    if (m_Description.HasByteCodeForStage((ezGALShaderStage::Enum)i))
    {
      createInfo.codeSize = m_Description.m_ByteCodes[i]->m_ByteCode.GetCount();
      EZ_ASSERT_DEV(createInfo.codeSize % 4 == 0, "Spirv shader code should be a multiple of 4.");
      createInfo.pCode = reinterpret_cast<const ezUInt32*>(m_Description.m_ByteCodes[i]->m_ByteCode.GetData());
      VK_SUCCEED_OR_RETURN_EZ_FAILURE(pDeviceVulkan->GetVulkanDevice().createShaderModule(&createInfo, nullptr, &m_Shaders[i]));
    }
  }

  return EZ_SUCCESS;
}

ezResult ezGALShaderVulkan::DeInitPlatform(ezGALDevice* pDevice)
{
  DestroyBindingMapping();
  DestroyLayouts(pDevice);

  auto* pVulkanDevice = static_cast<ezGALDeviceVulkan*>(pDevice);
  for (auto& m_Shader : m_Shaders)
  {
    pVulkanDevice->DeleteLater(m_Shader);
  }
  return EZ_SUCCESS;
}

vk::PipelineLayout ezGALShaderVulkan::GetVkPipelineLayout() const
{
  return static_cast<const ezGALPipelineLayoutVulkan*>(m_pDevice->GetPipelineLayout(m_hPipelineLayout))->GetVkPipelineLayout();
}

vk::DescriptorSetLayout ezGALShaderVulkan::GetDescriptorSetLayout(ezUInt32 uiSet) const
{
  EZ_ASSERT_DEBUG(uiSet < GetSetCount(), "Set index out of range.");
  return static_cast<const ezGALBindGroupLayoutVulkan*>(m_pDevice->GetBindGroupLayout(m_BindGroupLayouts[uiSet]))->GetDescriptorSetLayout();
}

vk::PushConstantRange ezGALShaderVulkan::GetPushConstantRange() const
{
  return static_cast<const ezGALPipelineLayoutVulkan*>(m_pDevice->GetPipelineLayout(m_hPipelineLayout))->GetPushConstantRange();
}
