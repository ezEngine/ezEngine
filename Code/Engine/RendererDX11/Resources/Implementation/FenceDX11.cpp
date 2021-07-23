#include <RendererDX11PCH.h>

#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/RendererDX11DLL.h>
#include <RendererDX11/Resources/FenceDX11.h>

#include <d3d11_4.h>

ezGALFenceDX11::ezGALFenceDX11()
  : m_pDXFence(nullptr)
  , m_pDXDeviceContext{nullptr}
{
}

ezGALFenceDX11::~ezGALFenceDX11() {}


ezResult ezGALFenceDX11::InitPlatform(ezGALDevice* pDevice, ezUInt64 initialValue)
{
  ezGALDeviceDX11* pDXDevice = static_cast<ezGALDeviceDX11*>(pDevice);

  ID3D11Device5* pD3d11Device5 = nullptr;

  if (!SUCCEEDED(pDXDevice->GetDXImmediateContext()->QueryInterface(__uuidof(ID3D11DeviceContext4), (void**)&m_pDXDeviceContext)))
  {
    ezLog::Error("ID3D11DeviceContext4 is not supported!");
    return EZ_FAILURE;
  }

  if (!SUCCEEDED(pDXDevice->GetDXDevice()->QueryInterface(__uuidof(ID3D11Device5), (void**)&pD3d11Device5)))
  {
    ezLog::Error("ID3D11Device5 is not supported!");
    return EZ_FAILURE;
  }

  ezResult result = SUCCEEDED(pD3d11Device5->CreateFence(initialValue, D3D11_FENCE_FLAG_NONE, __uuidof(ID3D11Fence), (void**)&m_pDXFence)) ? EZ_SUCCESS : EZ_FAILURE;

  if (result == EZ_SUCCESS)
  {
    m_FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
  }
  else
  {
    ezLog::Error("Creation of native DirectX fence failed!");
  }

  EZ_GAL_DX11_RELEASE(pD3d11Device5);

  return result;
}

ezResult ezGALFenceDX11::DeInitPlatform(ezGALDevice* pDevice)
{
  EZ_GAL_DX11_RELEASE(m_pDXFence);
  EZ_GAL_DX11_RELEASE(m_pDXDeviceContext);

  return EZ_SUCCESS;
}

ezUInt64 ezGALFenceDX11::GetCompletedValuePlatform() const
{
  return m_pDXFence->GetCompletedValue();
}

void ezGALFenceDX11::WaitPlatform(ezUInt64 value) const
{
  if (GetCompletedValuePlatform() < value)
  {
    m_pDXFence->SetEventOnCompletion(value, m_FenceEvent);
    WaitForSingleObjectEx(m_FenceEvent, INFINITE, FALSE);
  }
}

void ezGALFenceDX11::SignalPlatform(ezUInt64 value) const
{
  m_pDXDeviceContext->Signal(m_pDXFence, value);
}



EZ_STATICLINK_FILE(RendererDX11, RendererDX11_Resources_Implementation_FenceDX11);
