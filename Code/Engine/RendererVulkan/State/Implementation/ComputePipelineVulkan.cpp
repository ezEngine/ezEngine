#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Cache/ResourceCacheVulkan.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Shader/ShaderVulkan.h>
#include <RendererVulkan/Shader/VertexDeclarationVulkan.h>
#include <RendererVulkan/State/ComputePipelineVulkan.h>
#include <RendererVulkan/State/StateVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>

ezGALComputePipelineVulkan::ezGALComputePipelineVulkan(const ezGALComputePipelineCreationDescription& description)
  : ezGALComputePipeline(description)
{
}

ezGALComputePipelineVulkan::~ezGALComputePipelineVulkan() = default;

ezResult ezGALComputePipelineVulkan::InitPlatform(ezGALDevice* pDevice)
{
  ezGALDeviceVulkan* pDeviceVulkan = static_cast<ezGALDeviceVulkan*>(pDevice);

  const ezGALShaderVulkan* pShader = static_cast<const ezGALShaderVulkan*>(pDevice->GetShader(m_Description.m_hShader));
  if (pShader == nullptr)
  {
    ezLog::Error("Failed to create Vulkan Compute pipeline: Invalid shader handle.");
    return EZ_FAILURE;
  }

  vk::ComputePipelineCreateInfo pipe;
  pipe.layout = pShader->GetVkPipelineLayout();
  {
    vk::ShaderModule shader = pShader->GetShader(ezGALShaderStage::ComputeShader);
    EZ_ASSERT_DEV(shader != nullptr, "No compute shader stage present in the bound shader");
    pipe.stage.stage = ezConversionUtilsVulkan::GetShaderStage(ezGALShaderStage::ComputeShader);
    pipe.stage.module = shader;
    pipe.stage.pName = "main";
  }

  VK_ASSERT_DEV(pDeviceVulkan->GetVulkanDevice().createComputePipelines(ezResourceCacheVulkan::GetPipelineCache(), 1, &pipe, nullptr, &m_Pipeline));

  return EZ_SUCCESS;
}

ezResult ezGALComputePipelineVulkan::DeInitPlatform(ezGALDevice* pDevice)
{
  ezGALDeviceVulkan* pDeviceVulkan = static_cast<ezGALDeviceVulkan*>(pDevice);

  if (m_Pipeline)
  {
    pDeviceVulkan->DeleteLater(m_Pipeline);
    m_Pipeline = nullptr;
  }
  return EZ_SUCCESS;
}

void ezGALComputePipelineVulkan::SetDebugName(const char*)
{
}
