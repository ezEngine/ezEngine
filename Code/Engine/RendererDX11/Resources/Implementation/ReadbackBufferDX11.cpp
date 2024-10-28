#include <RendererDX11/RendererDX11PCH.h>

#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Resources/BufferDX11.h>
#include <RendererDX11/Resources/ReadbackBufferDX11.h>

#include <d3d11.h>

ezGALReadbackBufferDX11::ezGALReadbackBufferDX11(const ezGALBufferCreationDescription& Description)
  : ezGALReadbackBuffer(Description)
{
}

ezGALReadbackBufferDX11::~ezGALReadbackBufferDX11() = default;

ezResult ezGALReadbackBufferDX11::InitPlatform(ezGALDevice* pDevice)
{
  ezGALDeviceDX11* pDXDevice = static_cast<ezGALDeviceDX11*>(pDevice);

  D3D11_BUFFER_DESC BufferDesc = {};
  EZ_SUCCEED_OR_RETURN(ezGALBufferDX11::CreateBufferDesc(m_Description, BufferDesc, m_IndexFormat));
  BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
  BufferDesc.Usage = D3D11_USAGE_STAGING;

  if (SUCCEEDED(pDXDevice->GetDXDevice()->CreateBuffer(&BufferDesc, nullptr, &m_pDXBuffer)))
  {
    return EZ_SUCCESS;
  }
  else
  {
    ezLog::Error("Creation of native DirectX buffer failed!");
    return EZ_FAILURE;
  }
}

ezResult ezGALReadbackBufferDX11::DeInitPlatform(ezGALDevice* pDevice)
{
  EZ_IGNORE_UNUSED(pDevice);
  EZ_GAL_DX11_RELEASE(m_pDXBuffer);
  return EZ_SUCCESS;
}

void ezGALReadbackBufferDX11::SetDebugNamePlatform(const char* szName) const
{
  ezUInt32 uiLength = ezStringUtils::GetStringElementCount(szName);

  if (m_pDXBuffer != nullptr)
  {
    m_pDXBuffer->SetPrivateData(WKPDID_D3DDebugObjectName, uiLength, szName);
  }
}
