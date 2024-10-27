#include <RendererDX11/RendererDX11PCH.h>

#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/RendererDX11DLL.h>
#include <RendererDX11/Resources/BufferDX11.h>

#include <d3d11.h>

ezGALBufferDX11::ezGALBufferDX11(const ezGALBufferCreationDescription& Description)
  : ezGALBuffer(Description)
{
}

ezGALBufferDX11::~ezGALBufferDX11() = default;

ezResult ezGALBufferDX11::CreateBufferDesc(const ezGALBufferCreationDescription& description, D3D11_BUFFER_DESC& out_bufferDesc, DXGI_FORMAT& out_indexFormat)
{
  for (ezGALBufferUsageFlags::Enum flag : description.m_BufferFlags)
  {
    switch (flag)
    {
      case ezGALBufferUsageFlags::ConstantBuffer:
        out_bufferDesc.BindFlags |= D3D11_BIND_CONSTANT_BUFFER;
        break;
      case ezGALBufferUsageFlags::IndexBuffer:
        out_bufferDesc.BindFlags |= D3D11_BIND_INDEX_BUFFER;
        out_indexFormat = description.m_uiStructSize == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
        break;
      case ezGALBufferUsageFlags::VertexBuffer:
        out_bufferDesc.BindFlags |= D3D11_BIND_VERTEX_BUFFER;
        break;
      case ezGALBufferUsageFlags::TexelBuffer:
        break;
      case ezGALBufferUsageFlags::StructuredBuffer:
        out_bufferDesc.MiscFlags |= D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        break;
      case ezGALBufferUsageFlags::ByteAddressBuffer:
        out_bufferDesc.MiscFlags |= D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
        break;
      case ezGALBufferUsageFlags::ShaderResource:
        out_bufferDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
        break;
      case ezGALBufferUsageFlags::UnorderedAccess:
        out_bufferDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
        break;
      case ezGALBufferUsageFlags::DrawIndirect:
        out_bufferDesc.MiscFlags |= D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
        break;
      case ezGALBufferUsageFlags::Transient:
        // Nothing to set here. We only use this flag to decide whether its safe to use D3D11_MAP_WRITE_NO_OVERWRITE / D3D11_MAP_WRITE_DISCARD inside ezGALCommandEncoderImplDX11::UpdateBufferPlatform.
        break;
      default:
        ezLog::Error("Unknown buffer type supplied to CreateBuffer()!");
        return EZ_FAILURE;
    }
  }

  out_bufferDesc.ByteWidth = description.m_uiTotalSize;
  out_bufferDesc.CPUAccessFlags = 0;
  out_bufferDesc.StructureByteStride = description.m_uiStructSize;

  out_bufferDesc.CPUAccessFlags = 0;
  if (description.m_BufferFlags.IsSet(ezGALBufferUsageFlags::ConstantBuffer))
  {
    out_bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    out_bufferDesc.Usage = D3D11_USAGE_DYNAMIC;

    // If constant buffer: Patch size to be aligned to 64 bytes for easier usability
    out_bufferDesc.ByteWidth = ezMemoryUtils::AlignSize(out_bufferDesc.ByteWidth, 64u);
  }
  else
  {
    if (description.m_ResourceAccess.IsImmutable())
    {
      out_bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    }
    else
    {
      if (description.m_BufferFlags.IsSet(ezGALBufferUsageFlags::UnorderedAccess)) // UAVs allow writing from the GPU which cannot be combined with CPU write access.
      {
        out_bufferDesc.Usage = D3D11_USAGE_DEFAULT;
      }
      else
      {
        out_bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        out_bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
      }
    }
  }
  return EZ_SUCCESS;
}

ezResult ezGALBufferDX11::InitPlatform(ezGALDevice* pDevice, ezArrayPtr<const ezUInt8> pInitialData)
{
  ezGALDeviceDX11* pDXDevice = static_cast<ezGALDeviceDX11*>(pDevice);

  D3D11_BUFFER_DESC BufferDesc = {};
  EZ_SUCCEED_OR_RETURN(CreateBufferDesc(m_Description, BufferDesc, m_IndexFormat));

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
  EZ_IGNORE_UNUSED(pDevice);
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
