
#include <PCH.h>
#include <RendererDX11/Basics.h>
#include <RendererDX11/Resources/FenceDX11.h>
#include <RendererDX11/Device/DeviceDX11.h>

#include <d3d11.h>

ezGALFenceDX11::ezGALFenceDX11()
  : m_pDXFence(nullptr)
{
}

ezGALFenceDX11::~ezGALFenceDX11()
{
}


ezResult ezGALFenceDX11::InitPlatform(ezGALDevice* pDevice)
{
  ezGALDeviceDX11* pDXDevice = static_cast<ezGALDeviceDX11*>(pDevice);

  D3D11_QUERY_DESC QueryDesc;
  QueryDesc.Query = D3D11_QUERY_EVENT;
  QueryDesc.MiscFlags = 0;

  if (SUCCEEDED(pDXDevice->GetDXDevice()->CreateQuery(&QueryDesc, &m_pDXFence)))
  {
    return EZ_SUCCESS;
  }
  else
  {
    ezLog::Error("Creation of native DirectX fence failed!");
    return EZ_FAILURE;
  }
}

ezResult ezGALFenceDX11::DeInitPlatform(ezGALDevice* pDevice)
{
  EZ_GAL_DX11_RELEASE(m_pDXFence);

  return EZ_SUCCESS;
}



EZ_STATICLINK_FILE(RendererDX11, RendererDX11_Resources_Implementation_FenceDX11);

