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
    out_bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
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

ID3D11ShaderResourceView* ezGALBufferDX11::GetSRV(ezGALBufferRange bufferRange, ezEnum<ezGALShaderResourceType> resourceType, ezEnum<ezGALResourceFormat> overrideTexelBufferFormat) const
{
  ID3D11ShaderResourceView* pSRV = nullptr;

  View view;
  view.m_BufferRange = bufferRange;
  view.m_ResourceType = resourceType;
  view.m_OverrideTexelBufferFormat = overrideTexelBufferFormat;

  if (!m_SRVs.TryGetValue(view, pSRV))
  {
    ID3D11Resource* pDXResource = GetDXBuffer();
    const ezGALBufferCreationDescription& bufferDesc = GetDescription();

    D3D11_SHADER_RESOURCE_VIEW_DESC DXSRVDesc;
    DXSRVDesc.Format = DXGI_FORMAT_UNKNOWN;
    DXSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
    DXSRVDesc.BufferEx.Flags = 0;

    switch (resourceType)
    {
      case ezGALShaderResourceType::TexelBuffer:
      {
        const ezGALResourceFormat::Enum viewFormat = overrideTexelBufferFormat == ezGALResourceFormat::Invalid ? m_Description.m_Format : overrideTexelBufferFormat;

        const auto& formatInfo = m_pDevice->GetFormatLookupTable().GetFormatInfo(viewFormat);
        const ezUInt32 uiBytesPerElement = ezGALResourceFormat::GetBitsPerElement(viewFormat) / 8;

        DXSRVDesc.BufferEx.FirstElement = bufferRange.m_uiByteOffset / uiBytesPerElement;
        DXSRVDesc.BufferEx.NumElements = bufferRange.m_uiByteCount / uiBytesPerElement;
        DXSRVDesc.Format = ezGALResourceFormat::IsDepthFormat(viewFormat) ? formatInfo.m_eDepthOnlyType : formatInfo.m_eResourceViewType;
        if (DXSRVDesc.Format == DXGI_FORMAT_UNKNOWN)
        {
          ezLog::Error("Couldn't get valid DXGI format for resource view! ({0})", viewFormat);
          return nullptr;
        }
      }
      break;
      case ezGALShaderResourceType::StructuredBuffer:
      {
        DXSRVDesc.BufferEx.FirstElement = bufferRange.m_uiByteOffset / bufferDesc.m_uiStructSize;
        DXSRVDesc.BufferEx.NumElements = bufferRange.m_uiByteCount / bufferDesc.m_uiStructSize;
      }
      break;
      case ezGALShaderResourceType::ByteAddressBuffer:
      {
        DXSRVDesc.Format = DXGI_FORMAT_R32_TYPELESS;
        DXSRVDesc.BufferEx.FirstElement = bufferRange.m_uiByteOffset / 4;
        DXSRVDesc.BufferEx.NumElements = bufferRange.m_uiByteCount / 4;
        DXSRVDesc.BufferEx.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
      }
      break;
      default:
        EZ_REPORT_FAILURE("Unsupported resource type: {}", (ezUInt32)resourceType);
    }

    if (FAILED(m_pDevice->GetDXDevice()->CreateShaderResourceView(pDXResource, &DXSRVDesc, &pSRV)))
    {
      return nullptr;
    }
    m_SRVs.Insert(view, pSRV);
  }

  return pSRV;
}

ID3D11UnorderedAccessView* ezGALBufferDX11::GetUAV(ezGALBufferRange bufferRange, ezEnum<ezGALShaderResourceType> resourceType, ezEnum<ezGALResourceFormat> overrideTexelBufferFormat) const
{
  ID3D11UnorderedAccessView* pUAV = nullptr;

  View view;
  view.m_BufferRange = bufferRange;
  view.m_OverrideTexelBufferFormat = overrideTexelBufferFormat;

  if (!m_UAVs.TryGetValue(view, pUAV))
  {
    ID3D11Resource* pDXResource = GetDXBuffer();
    const ezGALBufferCreationDescription& bufferDesc = GetDescription();

    D3D11_UNORDERED_ACCESS_VIEW_DESC DXUAVDesc;
    DXUAVDesc.Format = DXGI_FORMAT_UNKNOWN;
    DXUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    DXUAVDesc.Buffer.Flags = 0;
    switch (resourceType)
    {
      case ezGALShaderResourceType::TexelBufferRW:
      {
        const ezGALResourceFormat::Enum viewFormat = overrideTexelBufferFormat == ezGALResourceFormat::Invalid ? m_Description.m_Format : overrideTexelBufferFormat;

        const auto& formatInfo = m_pDevice->GetFormatLookupTable().GetFormatInfo(viewFormat);
        const ezUInt32 uiBytesPerElement = ezGALResourceFormat::GetBitsPerElement(viewFormat) / 8;

        DXUAVDesc.Buffer.FirstElement = bufferRange.m_uiByteOffset / uiBytesPerElement;
        DXUAVDesc.Buffer.NumElements = bufferRange.m_uiByteCount / uiBytesPerElement;
        DXUAVDesc.Format = ezGALResourceFormat::IsDepthFormat(viewFormat) ? formatInfo.m_eDepthOnlyType : formatInfo.m_eResourceViewType;
        if (DXUAVDesc.Format == DXGI_FORMAT_UNKNOWN)
        {
          ezLog::Error("Couldn't get valid DXGI format for unordered access view! ({0})", viewFormat);
          return nullptr;
        }
      }
      break;
      case ezGALShaderResourceType::StructuredBufferRW:
      {
        DXUAVDesc.Buffer.FirstElement = bufferRange.m_uiByteOffset / bufferDesc.m_uiStructSize;
        DXUAVDesc.Buffer.NumElements = bufferRange.m_uiByteCount / bufferDesc.m_uiStructSize;
      }
      break;
      case ezGALShaderResourceType::ByteAddressBufferRW:
      {
        DXUAVDesc.Format = DXGI_FORMAT_R32_TYPELESS;
        DXUAVDesc.Buffer.FirstElement = bufferRange.m_uiByteOffset / 4;
        DXUAVDesc.Buffer.NumElements = bufferRange.m_uiByteCount / 4;
        DXUAVDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
      }
      break;
      default:
        EZ_REPORT_FAILURE("Unsupported resource type: {}", (ezUInt32)resourceType);
    }

    if (FAILED(m_pDevice->GetDXDevice()->CreateUnorderedAccessView(pDXResource, &DXUAVDesc, &pUAV)))
    {
      return nullptr;
    }

    m_UAVs.Insert(view, pUAV);
  }

  return pUAV;
}

ezResult ezGALBufferDX11::InitPlatform(ezGALDevice* pDevice, ezArrayPtr<const ezUInt8> pInitialData)
{
  ezGALDeviceDX11* pDXDevice = static_cast<ezGALDeviceDX11*>(pDevice);
  m_pDevice = pDXDevice;

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

  for (auto it : m_SRVs)
  {
    EZ_GAL_DX11_RELEASE(it.Value());
  }
  m_SRVs.Clear();
  for (auto it : m_UAVs)
  {
    EZ_GAL_DX11_RELEASE(it.Value());
  }
  m_UAVs.Clear();
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
