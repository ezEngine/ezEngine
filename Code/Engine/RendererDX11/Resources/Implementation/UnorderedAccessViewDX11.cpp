#include <RendererDX11/RendererDX11PCH.h>

#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Resources/BufferDX11.h>
#include <RendererDX11/Resources/TextureDX11.h>
#include <RendererDX11/Resources/UnorderedAccessViewDX11.h>

#include <d3d11.h>

ezGALTextureUnorderedAccessViewDX11::ezGALTextureUnorderedAccessViewDX11(
  ezGALTexture* pResource, const ezGALTextureUnorderedAccessViewCreationDescription& Description)
  : ezGALTextureUnorderedAccessView(pResource, Description)

{
}

ezGALTextureUnorderedAccessViewDX11::~ezGALTextureUnorderedAccessViewDX11() = default;

ezResult ezGALTextureUnorderedAccessViewDX11::InitPlatform(ezGALDevice* pDevice)
{
  const ezGALTexture* pTexture = nullptr;
  if (!m_Description.m_hTexture.IsInvalidated())
    pTexture = pDevice->GetTexture(m_Description.m_hTexture);

  if (pTexture == nullptr)
  {
    ezLog::Error("No valid texture handle given for unordered access view creation!");
    return EZ_FAILURE;
  }

  ezGALResourceFormat::Enum ViewFormat = m_Description.m_OverrideViewFormat;

  {
    const ezGALTextureCreationDescription& TexDesc = pTexture->GetDescription();
    if (ViewFormat == ezGALResourceFormat::Invalid)
      ViewFormat = TexDesc.m_Format;
  }

  ezGALDeviceDX11* pDXDevice = static_cast<ezGALDeviceDX11*>(pDevice);

  DXGI_FORMAT DXViewFormat = DXGI_FORMAT_UNKNOWN;
  if (ezGALResourceFormat::IsDepthFormat(ViewFormat))
  {
    DXViewFormat = pDXDevice->GetFormatLookupTable().GetFormatInfo(ViewFormat).m_eDepthOnlyType;
  }
  else
  {
    DXViewFormat = pDXDevice->GetFormatLookupTable().GetFormatInfo(ViewFormat).m_eResourceViewType;
  }

  if (DXViewFormat == DXGI_FORMAT_UNKNOWN)
  {
    ezLog::Error("Couldn't get valid DXGI format for resource view! ({0})", ViewFormat);
    return EZ_FAILURE;
  }

  D3D11_UNORDERED_ACCESS_VIEW_DESC DXUAVDesc;
  DXUAVDesc.Format = DXViewFormat;

  ID3D11Resource* pDXResource = static_cast<const ezGALTextureDX11*>(pTexture->GetParentResource())->GetDXTexture();
  const ezGALTextureCreationDescription& texDesc = pTexture->GetDescription();
  EZ_IGNORE_UNUSED(texDesc);

  const ezEnum<ezGALTextureType> type = m_Description.m_OverrideViewType != ezGALTextureType::Invalid ? m_Description.m_OverrideViewType : pTexture->GetDescription().m_Type;
  switch (type)
  {
    case ezGALTextureType::Texture2D:
    case ezGALTextureType::Texture2DShared:
      EZ_ASSERT_DEV(texDesc.m_uiArraySize == 1 && m_Description.m_uiFirstArraySlice == 0, "These options can only be used with array texture types.");

      DXUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
      DXUAVDesc.Texture2D.MipSlice = m_Description.m_uiMipLevelToUse;
      break;

    case ezGALTextureType::Texture2DProxy:
    case ezGALTextureType::Texture2DArray:
      DXUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
      DXUAVDesc.Texture2DArray.MipSlice = m_Description.m_uiMipLevelToUse;
      DXUAVDesc.Texture2DArray.ArraySize = m_Description.m_uiArraySize;
      DXUAVDesc.Texture2DArray.FirstArraySlice = m_Description.m_uiFirstArraySlice;
      break;

    case ezGALTextureType::Texture3D:
      DXUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
      DXUAVDesc.Texture3D.MipSlice = m_Description.m_uiMipLevelToUse;
      DXUAVDesc.Texture3D.FirstWSlice = m_Description.m_uiFirstArraySlice;
      DXUAVDesc.Texture3D.WSize = m_Description.m_uiArraySize;
      break;

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
      return EZ_FAILURE;
  }

  if (FAILED(pDXDevice->GetDXDevice()->CreateUnorderedAccessView(pDXResource, &DXUAVDesc, &m_pDXUnorderedAccessView)))
  {
    return EZ_FAILURE;
  }
  else
  {
    return EZ_SUCCESS;
  }
}

ezResult ezGALTextureUnorderedAccessViewDX11::DeInitPlatform(ezGALDevice* pDevice)
{
  EZ_IGNORE_UNUSED(pDevice);

  EZ_GAL_DX11_RELEASE(m_pDXUnorderedAccessView);
  return EZ_SUCCESS;
}

/////////////////////////////////////////////////////////////////////

ezGALBufferUnorderedAccessViewDX11::ezGALBufferUnorderedAccessViewDX11(
  ezGALBuffer* pResource, const ezGALBufferUnorderedAccessViewCreationDescription& Description)
  : ezGALBufferUnorderedAccessView(pResource, Description)

{
}

ezGALBufferUnorderedAccessViewDX11::~ezGALBufferUnorderedAccessViewDX11() = default;

ezResult ezGALBufferUnorderedAccessViewDX11::InitPlatform(ezGALDevice* pDevice)
{
  ezGALDeviceDX11* pDXDevice = static_cast<ezGALDeviceDX11*>(pDevice);
  const ezGALBuffer* pBuffer = pDevice->GetBuffer(m_Description.m_hBuffer);
  ID3D11Resource* pDXResource = static_cast<const ezGALBufferDX11*>(pBuffer)->GetDXBuffer();
  const ezGALBufferCreationDescription& bufferDesc = pBuffer->GetDescription();

  D3D11_UNORDERED_ACCESS_VIEW_DESC DXUAVDesc;
  DXUAVDesc.Format = DXGI_FORMAT_UNKNOWN;
  DXUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
  DXUAVDesc.Buffer.Flags = 0;
  switch (m_Description.m_ResourceType)
  {
    case ezGALShaderResourceType::TexelBufferRW:
    {
      const ezGALResourceFormat::Enum viewFormat = m_Description.m_Format;
      const auto& formatInfo = pDXDevice->GetFormatLookupTable().GetFormatInfo(viewFormat);
      const ezUInt32 uiBytesPerElement = ezGALResourceFormat::GetBitsPerElement(viewFormat) / 8;

      DXUAVDesc.Buffer.FirstElement = m_Description.m_uiByteOffset / uiBytesPerElement;
      DXUAVDesc.Buffer.NumElements = m_Description.m_uiByteCount / uiBytesPerElement;
      DXUAVDesc.Format = ezGALResourceFormat::IsDepthFormat(viewFormat) ? formatInfo.m_eDepthOnlyType : formatInfo.m_eResourceViewType;
      if (DXUAVDesc.Format == DXGI_FORMAT_UNKNOWN)
      {
        ezLog::Error("Couldn't get valid DXGI format for unordered access view! ({0})", viewFormat);
        return EZ_FAILURE;
      }
    }
    break;
    case ezGALShaderResourceType::StructuredBufferRW:
    {
      DXUAVDesc.Buffer.FirstElement = m_Description.m_uiByteOffset / bufferDesc.m_uiStructSize;
      DXUAVDesc.Buffer.NumElements = m_Description.m_uiByteCount / bufferDesc.m_uiStructSize;
    }
    break;
    case ezGALShaderResourceType::ByteAddressBufferRW:
    {
      DXUAVDesc.Format = DXGI_FORMAT_R32_TYPELESS;
      DXUAVDesc.Buffer.FirstElement = m_Description.m_uiByteOffset / 4;
      DXUAVDesc.Buffer.NumElements = m_Description.m_uiByteCount / 4;
      DXUAVDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
    }
    break;
    default:
      EZ_REPORT_FAILURE("Unsupported resource type: {}", (ezUInt32)m_Description.m_ResourceType);
  }

  if (FAILED(pDXDevice->GetDXDevice()->CreateUnorderedAccessView(pDXResource, &DXUAVDesc, &m_pDXUnorderedAccessView)))
  {
    return EZ_FAILURE;
  }
  else
  {
    return EZ_SUCCESS;
  }
}

ezResult ezGALBufferUnorderedAccessViewDX11::DeInitPlatform(ezGALDevice* pDevice)
{
  EZ_IGNORE_UNUSED(pDevice);

  EZ_GAL_DX11_RELEASE(m_pDXUnorderedAccessView);
  return EZ_SUCCESS;
}
