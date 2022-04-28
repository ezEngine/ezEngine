#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Shader/ShaderVulkan.h>

ezGALShaderVulkan::ezGALShaderVulkan(const ezGALShaderCreationDescription& Description)
  : ezGALShader(Description)
  , m_pVertexShader(nullptr)
  , m_pHullShader(nullptr)
  , m_pDomainShader(nullptr)
  , m_pGeometryShader(nullptr)
  , m_pPixelShader(nullptr)
  , m_pComputeShader(nullptr)
{
}

ezGALShaderVulkan::~ezGALShaderVulkan() {}

void ezGALShaderVulkan::SetDebugName(const char* szName) const
{
  ezUInt32 uiLength = ezStringUtils::GetStringElementCount(szName);

  // TODO
#if 0
  if (m_pVertexShader != nullptr)
  {
    m_pVertexShader->SetPrivateData(WKPDID_D3DDebugObjectName, uiLength, szName);
  }

  if (m_pHullShader != nullptr)
  {
    m_pHullShader->SetPrivateData(WKPDID_D3DDebugObjectName, uiLength, szName);
  }

  if (m_pDomainShader != nullptr)
  {
    m_pDomainShader->SetPrivateData(WKPDID_D3DDebugObjectName, uiLength, szName);
  }

  if (m_pGeometryShader != nullptr)
  {
    m_pGeometryShader->SetPrivateData(WKPDID_D3DDebugObjectName, uiLength, szName);
  }

  if (m_pPixelShader != nullptr)
  {
    m_pPixelShader->SetPrivateData(WKPDID_D3DDebugObjectName, uiLength, szName);
  }

  if (m_pComputeShader != nullptr)
  {
    m_pComputeShader->SetPrivateData(WKPDID_D3DDebugObjectName, uiLength, szName);
  }
#endif
}

ezResult ezGALShaderVulkan::InitPlatform(ezGALDevice* pDevice)
{
  ezGALDeviceVulkan* pVulkanDevice = static_cast<ezGALDeviceVulkan*>(pDevice);
  vk::ShaderModuleCreateInfo createInfo;

  if (m_Description.HasByteCodeForStage(ezGALShaderStage::VertexShader))
  {
    createInfo.codeSize = m_Description.m_ByteCodes[ezGALShaderStage::VertexShader]->GetSize();
    EZ_ASSERT_DEV(createInfo.codeSize % 4 == 0, "");
    createInfo.pCode = reinterpret_cast<const ezUInt32*>(m_Description.m_ByteCodes[ezGALShaderStage::VertexShader]->GetByteCode());
    VK_SUCCEED_OR_RETURN_EZ_FAILURE(pVulkanDevice->GetVulkanDevice().createShaderModule(&createInfo, nullptr, &m_pVertexShader));
  }

  if (m_Description.HasByteCodeForStage(ezGALShaderStage::HullShader))
  {
    createInfo.codeSize = m_Description.m_ByteCodes[ezGALShaderStage::HullShader]->GetSize();
    EZ_ASSERT_DEV(createInfo.codeSize % 4 == 0, "");
    createInfo.pCode = reinterpret_cast<const ezUInt32*>(m_Description.m_ByteCodes[ezGALShaderStage::HullShader]->GetByteCode());
    VK_SUCCEED_OR_RETURN_EZ_FAILURE(pVulkanDevice->GetVulkanDevice().createShaderModule(&createInfo, nullptr, &m_pHullShader));
  }

  if (m_Description.HasByteCodeForStage(ezGALShaderStage::DomainShader))
  {
    createInfo.codeSize = m_Description.m_ByteCodes[ezGALShaderStage::DomainShader]->GetSize();
    EZ_ASSERT_DEV(createInfo.codeSize % 4 == 0, "");
    createInfo.pCode = reinterpret_cast<const ezUInt32*>(m_Description.m_ByteCodes[ezGALShaderStage::DomainShader]->GetByteCode());
    VK_SUCCEED_OR_RETURN_EZ_FAILURE(pVulkanDevice->GetVulkanDevice().createShaderModule(&createInfo, nullptr, &m_pDomainShader));
  }

  if (m_Description.HasByteCodeForStage(ezGALShaderStage::GeometryShader))
  {
    createInfo.codeSize = m_Description.m_ByteCodes[ezGALShaderStage::GeometryShader]->GetSize();
    EZ_ASSERT_DEV(createInfo.codeSize % 4 == 0, "");
    createInfo.pCode = reinterpret_cast<const ezUInt32*>(m_Description.m_ByteCodes[ezGALShaderStage::GeometryShader]->GetByteCode());
    VK_SUCCEED_OR_RETURN_EZ_FAILURE(pVulkanDevice->GetVulkanDevice().createShaderModule(&createInfo, nullptr, &m_pGeometryShader));
  }

  if (m_Description.HasByteCodeForStage(ezGALShaderStage::PixelShader))
  {
    createInfo.codeSize = m_Description.m_ByteCodes[ezGALShaderStage::PixelShader]->GetSize();
    EZ_ASSERT_DEV(createInfo.codeSize % 4 == 0, "");
    createInfo.pCode = reinterpret_cast<const ezUInt32*>(m_Description.m_ByteCodes[ezGALShaderStage::PixelShader]->GetByteCode());
    VK_SUCCEED_OR_RETURN_EZ_FAILURE(pVulkanDevice->GetVulkanDevice().createShaderModule(&createInfo, nullptr, &m_pPixelShader));
  }

  if (m_Description.HasByteCodeForStage(ezGALShaderStage::ComputeShader))
  {
    createInfo.codeSize = m_Description.m_ByteCodes[ezGALShaderStage::ComputeShader]->GetSize();
    EZ_ASSERT_DEV(createInfo.codeSize % 4 == 0, "");
    createInfo.pCode = reinterpret_cast<const ezUInt32*>(m_Description.m_ByteCodes[ezGALShaderStage::ComputeShader]->GetByteCode());
    VK_SUCCEED_OR_RETURN_EZ_FAILURE(pVulkanDevice->GetVulkanDevice().createShaderModule(&createInfo, nullptr, &m_pComputeShader));
  }

  return EZ_SUCCESS;
}

ezResult ezGALShaderVulkan::DeInitPlatform(ezGALDevice* pDevice)
{
  ezGALDeviceVulkan* pVulkanDevice = static_cast<ezGALDeviceVulkan*>(pDevice);
  pVulkanDevice->DeleteLater(m_pVertexShader);
  pVulkanDevice->DeleteLater(m_pHullShader);
  pVulkanDevice->DeleteLater(m_pDomainShader);
  pVulkanDevice->DeleteLater(m_pGeometryShader);
  pVulkanDevice->DeleteLater(m_pPixelShader);
  pVulkanDevice->DeleteLater(m_pComputeShader);
  return EZ_SUCCESS;
}

EZ_STATICLINK_FILE(RendererVulkan, RendererVulkan_Shader_Implementation_ShaderVulkan);
