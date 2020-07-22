#include <RendererDX11PCH.h>

#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/RendererDX11DLL.h>
#include <RendererDX11/Resources/BufferDX11.h>

#include <d3d11.h>

ezGALBufferDX11::ezGALBufferDX11(const ezGALBufferCreationDescription& Description)
  : ezGALBuffer(Description)
  , m_pDXBuffer(nullptr)
  , m_IndexFormat(DXGI_FORMAT_UNKNOWN)
{
}

ezGALBufferDX11::~ezGALBufferDX11() {}

ezResult ezGALBufferDX11::InitPlatform(ezGALDevice* pDevice, ezArrayPtr<const ezUInt8> pInitialData)
{
  ezGALDeviceDX11* pDXDevice = static_cast<ezGALDeviceDX11*>(pDevice);

  D3D11_BUFFER_DESC BufferDesc;

  switch (m_Description.m_BufferType)
  {
    case ezGALBufferType::ConstantBuffer:
      BufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
      break;
    case ezGALBufferType::IndexBuffer:
      BufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

      m_IndexFormat = m_Description.m_uiStructSize == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;

      break;
    case ezGALBufferType::VertexBuffer:
      BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
      break;
    case ezGALBufferType::Generic:
      BufferDesc.BindFlags = 0;
      break;
    default:
      ezLog::Error("Unknown buffer type supplied to CreateBuffer()!");
      return EZ_FAILURE;
  }

  if (m_Description.m_bAllowShaderResourceView)
    BufferDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;

  if (m_Description.m_bAllowUAV)
    BufferDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;

  if (m_Description.m_bStreamOutputTarget)
    BufferDesc.BindFlags |= D3D11_BIND_STREAM_OUTPUT;

  BufferDesc.ByteWidth = m_Description.m_uiTotalSize;
  BufferDesc.CPUAccessFlags = 0;
  BufferDesc.MiscFlags = 0;

  if (m_Description.m_bUseForIndirectArguments)
    BufferDesc.MiscFlags |= D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;

  if (m_Description.m_bAllowRawViews)
    BufferDesc.MiscFlags |= D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;

  if (m_Description.m_bUseAsStructuredBuffer)
    BufferDesc.MiscFlags |= D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

  BufferDesc.StructureByteStride = m_Description.m_uiStructSize;

  if (m_Description.m_BufferType == ezGALBufferType::ConstantBuffer)
  {
    BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    BufferDesc.Usage = D3D11_USAGE_DYNAMIC;

    // If constant buffer: Patch size to be aligned to 64 bytes for easier usability
    BufferDesc.ByteWidth = ezMemoryUtils::AlignSize(BufferDesc.ByteWidth, 64u);
  }
  else
  {
    if (m_Description.m_ResourceAccess.IsImmutable())
    {
      BufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    }
    else
    {
      if (m_Description.m_bAllowUAV) // UAVs allow writing from the GPU which cannot be combined with CPU write access.
      {
        BufferDesc.Usage = D3D11_USAGE_DEFAULT;
      }
      else
      {
        BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
      }
    }
  }

  D3D11_SUBRESOURCE_DATA DXInitialData;
  DXInitialData.pSysMem = pInitialData.GetPtr();
  DXInitialData.SysMemPitch = DXInitialData.SysMemSlicePitch = 0;

  if (SUCCEEDED(pDXDevice->GetDXDevice()->CreateBuffer(&BufferDesc, pInitialData.IsEmpty() ? nullptr : &DXInitialData, &m_pDXBuffer)))
  {
    return EZ_SUCCESS;
  }
  else
  {
    ezLog::Error("Creation of native DirectX buffer failed!");
    return EZ_FAILURE;
  }
}

ezResult ezGALBufferDX11::DeInitPlatform(ezGALDevice* pDevice)
{
  EZ_GAL_DX11_RELEASE(m_pDXBuffer);

  return EZ_SUCCESS;
}

void ezGALBufferDX11::SetDebugNamePlatform(const char* szName) const
{
  ezUInt32 uiLength = ezStringUtils::GetStringElementCount(szName);

  if (m_pDXBuffer != nullptr)
  {
    m_pDXBuffer->SetPrivateData(WKPDID_D3DDebugObjectName, uiLength, szName);
  }
}

EZ_STATICLINK_FILE(RendererDX11, RendererDX11_Resources_Implementation_BufferDX11);
