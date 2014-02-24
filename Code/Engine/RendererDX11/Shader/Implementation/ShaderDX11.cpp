

#include <RendererDX11/PCH.h>
#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Shader/ShaderDX11.h>
#include <Foundation/Logging/Log.h>

#include <d3d11.h>

ezGALShaderDX11::ezGALShaderDX11(const ezGALShaderCreationDescription& Description)
  : ezGALShader(Description),
    m_pVertexShader(NULL),
    m_pHullShader(NULL),
    m_pDomainShader(NULL),
    m_pGeometryShader(NULL),
    m_pPixelShader(NULL),
    m_pComputeShader(NULL)
{
}

ezGALShaderDX11::~ezGALShaderDX11()
{
}

ezResult ezGALShaderDX11::InitPlatform(ezGALDevice* pDevice)
{
  ezGALDeviceDX11* pDXDevice = static_cast<ezGALDeviceDX11*>(pDevice);
  ID3D11Device* pD3D11Device = pDXDevice->GetDXDevice();

  if(m_Description.HasByteCodeForStage(ezGALShaderStage::VertexShader))
  {
    if(FAILED(pD3D11Device->CreateVertexShader(m_Description.m_ByteCodes[ezGALShaderStage::VertexShader]->GetByteCode(), m_Description.m_ByteCodes[ezGALShaderStage::VertexShader]->GetSize(), NULL, &m_pVertexShader)))
    {
      ezLog::Error("Couldn't create native vertex shader from bytecode!");
      return EZ_FAILURE;
    }
  }

  if(m_Description.HasByteCodeForStage(ezGALShaderStage::HullShader))
  {
    if(FAILED(pD3D11Device->CreateHullShader(m_Description.m_ByteCodes[ezGALShaderStage::VertexShader]->GetByteCode(), m_Description.m_ByteCodes[ezGALShaderStage::HullShader]->GetSize(), NULL, &m_pHullShader)))
    {
      ezLog::Error("Couldn't create native hull shader from bytecode!");
      return EZ_FAILURE;
    }
  }

  if(m_Description.HasByteCodeForStage(ezGALShaderStage::DomainShader))
  {
    if(FAILED(pD3D11Device->CreateDomainShader(m_Description.m_ByteCodes[ezGALShaderStage::DomainShader]->GetByteCode(), m_Description.m_ByteCodes[ezGALShaderStage::DomainShader]->GetSize(), NULL, &m_pDomainShader)))
    {
      ezLog::Error("Couldn't create native domain shader from bytecode!");
      return EZ_FAILURE;
    }
  }

  if(m_Description.HasByteCodeForStage(ezGALShaderStage::GeometryShader))
  {
    if(FAILED(pD3D11Device->CreateGeometryShader(m_Description.m_ByteCodes[ezGALShaderStage::GeometryShader]->GetByteCode(), m_Description.m_ByteCodes[ezGALShaderStage::GeometryShader]->GetSize(), NULL, &m_pGeometryShader)))
    {
      ezLog::Error("Couldn't create native geometry shader from bytecode!");
      return EZ_FAILURE;
    }
  }

  if(m_Description.HasByteCodeForStage(ezGALShaderStage::PixelShader))
  {
    if(FAILED(pD3D11Device->CreatePixelShader(m_Description.m_ByteCodes[ezGALShaderStage::PixelShader]->GetByteCode(), m_Description.m_ByteCodes[ezGALShaderStage::PixelShader]->GetSize(), NULL, &m_pPixelShader)))
    {
      ezLog::Error("Couldn't create native pixel shader from bytecode!");
      return EZ_FAILURE;
    }
  }

  if(m_Description.HasByteCodeForStage(ezGALShaderStage::ComputeShader))
  {
    if(FAILED(pD3D11Device->CreateComputeShader(m_Description.m_ByteCodes[ezGALShaderStage::ComputeShader]->GetByteCode(), m_Description.m_ByteCodes[ezGALShaderStage::ComputeShader]->GetSize(), NULL, &m_pComputeShader)))
    {
      ezLog::Error("Couldn't create native compute shader from bytecode!");
      return EZ_FAILURE;
    }
  }


  return EZ_SUCCESS;
}

ezResult ezGALShaderDX11::DeInitPlatform(ezGALDevice* pDevice)
{
  EZ_GAL_DX11_RELEASE(m_pVertexShader);
  EZ_GAL_DX11_RELEASE(m_pHullShader);
  EZ_GAL_DX11_RELEASE(m_pDomainShader);
  EZ_GAL_DX11_RELEASE(m_pGeometryShader);
  EZ_GAL_DX11_RELEASE(m_pPixelShader);
  EZ_GAL_DX11_RELEASE(m_pComputeShader);

  return EZ_SUCCESS;
}
