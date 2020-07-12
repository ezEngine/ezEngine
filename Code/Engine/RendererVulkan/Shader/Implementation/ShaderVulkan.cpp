#include <RendererVulkanPCH.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Shader/ShaderVulkan.h>

#include <d3d11.h>

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
}

ezResult ezGALShaderVulkan::InitPlatform(ezGALDevice* pDevice)
{
  ezGALDeviceVulkan* pDXDevice = static_cast<ezGALDeviceVulkan*>(pDevice);
  ID3D11Device* pD3D11Device = pDXDevice->GetDXDevice();

  if (m_Description.HasByteCodeForStage(ezGALShaderStage::VertexShader))
  {
    if (FAILED(pD3D11Device->CreateVertexShader(m_Description.m_ByteCodes[ezGALShaderStage::VertexShader]->GetByteCode(),
                                                m_Description.m_ByteCodes[ezGALShaderStage::VertexShader]->GetSize(), nullptr,
                                                &m_pVertexShader)))
    {
      ezLog::Error("Couldn't create native vertex shader from bytecode!");
      return EZ_FAILURE;
    }
  }

  if (m_Description.HasByteCodeForStage(ezGALShaderStage::HullShader))
  {
    if (FAILED(pD3D11Device->CreateHullShader(m_Description.m_ByteCodes[ezGALShaderStage::VertexShader]->GetByteCode(),
                                              m_Description.m_ByteCodes[ezGALShaderStage::HullShader]->GetSize(), nullptr, &m_pHullShader)))
    {
      ezLog::Error("Couldn't create native hull shader from bytecode!");
      return EZ_FAILURE;
    }
  }

  if (m_Description.HasByteCodeForStage(ezGALShaderStage::DomainShader))
  {
    if (FAILED(pD3D11Device->CreateDomainShader(m_Description.m_ByteCodes[ezGALShaderStage::DomainShader]->GetByteCode(),
                                                m_Description.m_ByteCodes[ezGALShaderStage::DomainShader]->GetSize(), nullptr,
                                                &m_pDomainShader)))
    {
      ezLog::Error("Couldn't create native domain shader from bytecode!");
      return EZ_FAILURE;
    }
  }

  if (m_Description.HasByteCodeForStage(ezGALShaderStage::GeometryShader))
  {
    if (FAILED(pD3D11Device->CreateGeometryShader(m_Description.m_ByteCodes[ezGALShaderStage::GeometryShader]->GetByteCode(),
                                                  m_Description.m_ByteCodes[ezGALShaderStage::GeometryShader]->GetSize(), nullptr,
                                                  &m_pGeometryShader)))
    {
      ezLog::Error("Couldn't create native geometry shader from bytecode!");
      return EZ_FAILURE;
    }
  }

  if (m_Description.HasByteCodeForStage(ezGALShaderStage::PixelShader))
  {
    if (FAILED(pD3D11Device->CreatePixelShader(m_Description.m_ByteCodes[ezGALShaderStage::PixelShader]->GetByteCode(),
                                               m_Description.m_ByteCodes[ezGALShaderStage::PixelShader]->GetSize(), nullptr,
                                               &m_pPixelShader)))
    {
      ezLog::Error("Couldn't create native pixel shader from bytecode!");
      return EZ_FAILURE;
    }
  }

  if (m_Description.HasByteCodeForStage(ezGALShaderStage::ComputeShader))
  {
    if (FAILED(pD3D11Device->CreateComputeShader(m_Description.m_ByteCodes[ezGALShaderStage::ComputeShader]->GetByteCode(),
                                                 m_Description.m_ByteCodes[ezGALShaderStage::ComputeShader]->GetSize(), nullptr,
                                                 &m_pComputeShader)))
    {
      ezLog::Error("Couldn't create native compute shader from bytecode!");
      return EZ_FAILURE;
    }
  }


  return EZ_SUCCESS;
}

ezResult ezGALShaderVulkan::DeInitPlatform(ezGALDevice* pDevice)
{
  EZ_GAL_Vulkan_RELEASE(m_pVertexShader);
  EZ_GAL_Vulkan_RELEASE(m_pHullShader);
  EZ_GAL_Vulkan_RELEASE(m_pDomainShader);
  EZ_GAL_Vulkan_RELEASE(m_pGeometryShader);
  EZ_GAL_Vulkan_RELEASE(m_pPixelShader);
  EZ_GAL_Vulkan_RELEASE(m_pComputeShader);

  return EZ_SUCCESS;
}



EZ_STATICLINK_FILE(RendererVulkan, RendererVulkan_Shader_Implementation_ShaderVulkan);
