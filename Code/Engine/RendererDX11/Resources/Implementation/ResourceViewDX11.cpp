#include <RendererDX11/RendererDX11PCH.h>

#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Resources/BufferDX11.h>
#include <RendererDX11/Resources/ResourceViewDX11.h>
#include <RendererDX11/Resources/TextureDX11.h>

#include <d3d11.h>

ezGALTextureResourceViewDX11::ezGALTextureResourceViewDX11(ezGALTexture* pResource, const ezGALTextureResourceViewCreationDescription& Description)
  : ezGALTextureResourceView(pResource, Description)

{
}

ezGALTextureResourceViewDX11::~ezGALTextureResourceViewDX11() = default;

ezResult ezGALTextureResourceViewDX11::InitPlatform(ezGALDevice* pDevice)
{
  const ezGALTexture* pTexture = nullptr;
  if (!m_Description.m_hTexture.IsInvalidated())
    pTexture = pDevice->GetTexture(m_Description.m_hTexture);

  if (pTexture == nullptr)
  {
    ezLog::Error("No valid texture handle given for resource view creation!");
    return EZ_FAILURE;
  }

  ezGALResourceFormat::Enum ViewFormat = m_Description.m_OverrideViewFormat;

  if (ViewFormat == ezGALResourceFormat::Invalid)
    ViewFormat = pTexture->GetDescription().m_Format;

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

  D3D11_SHADER_RESOURCE_VIEW_DESC DXSRVDesc;
  DXSRVDesc.Format = DXViewFormat;

  ID3D11Resource* pDXResource = nullptr;

  pDXResource = static_cast<const ezGALTextureDX11*>(pTexture->GetParentResource())->GetDXTexture();
  const ezGALTextureCreationDescription& texDesc = pTexture->GetDescription();

  // DX11 does not care about view types matching the shader. It does care though about view types matching the resource.
  const ezEnum<ezGALTextureType> type = texDesc.m_Type;
  switch (type)
  {
    case ezGALTextureType::Texture2D:
    case ezGALTextureType::Texture2DShared:
      EZ_ASSERT_DEV(texDesc.m_uiArraySize == 1 && m_Description.m_uiFirstArraySlice == 0, "These options can only be used with array texture types.");

      if (texDesc.m_SampleCount == ezGALMSAASampleCount::None)
      {
        DXSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        DXSRVDesc.Texture2D.MipLevels = m_Description.m_uiMipLevelsToUse;
        DXSRVDesc.Texture2D.MostDetailedMip = m_Description.m_uiMostDetailedMipLevel;
      }
      else
      {
        DXSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
      }
      break;

    case ezGALTextureType::Texture2DProxy:
    case ezGALTextureType::Texture2DArray:

      if (texDesc.m_SampleCount == ezGALMSAASampleCount::None)
      {
        DXSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
        DXSRVDesc.Texture2DArray.MipLevels = m_Description.m_uiMipLevelsToUse;
        DXSRVDesc.Texture2DArray.MostDetailedMip = m_Description.m_uiMostDetailedMipLevel;
        DXSRVDesc.Texture2DArray.ArraySize = m_Description.m_uiArraySize;
        DXSRVDesc.Texture2DArray.FirstArraySlice = m_Description.m_uiFirstArraySlice;
      }
      else
      {
        DXSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY;
        DXSRVDesc.Texture2DMSArray.ArraySize = m_Description.m_uiArraySize;
        DXSRVDesc.Texture2DMSArray.FirstArraySlice = m_Description.m_uiFirstArraySlice;
      }

      break;

    case ezGALTextureType::TextureCube:
      EZ_ASSERT_DEV(texDesc.m_uiArraySize == 1 && m_Description.m_uiFirstArraySlice == 0, "These options can only be used with array texture types.");

      DXSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
      DXSRVDesc.TextureCube.MipLevels = m_Description.m_uiMipLevelsToUse;
      DXSRVDesc.TextureCube.MostDetailedMip = m_Description.m_uiMostDetailedMipLevel;
      break;

    case ezGALTextureType::TextureCubeArray:
      DXSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
      DXSRVDesc.TextureCube.MipLevels = m_Description.m_uiMipLevelsToUse;
      DXSRVDesc.TextureCube.MostDetailedMip = m_Description.m_uiMostDetailedMipLevel;
      DXSRVDesc.TextureCubeArray.NumCubes = m_Description.m_uiArraySize;
      DXSRVDesc.TextureCubeArray.First2DArrayFace = m_Description.m_uiFirstArraySlice;
      break;

    case ezGALTextureType::Texture3D:
      EZ_ASSERT_DEV(texDesc.m_uiArraySize == 1 && m_Description.m_uiFirstArraySlice == 0, "These options can only be used with array texture types.");

      DXSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
      DXSRVDesc.Texture3D.MipLevels = m_Description.m_uiMipLevelsToUse;
      DXSRVDesc.Texture3D.MostDetailedMip = m_Description.m_uiMostDetailedMipLevel;

      break;

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
      return EZ_FAILURE;
  }

  if (FAILED(pDXDevice->GetDXDevice()->CreateShaderResourceView(pDXResource, &DXSRVDesc, &m_pDXResourceView)))
  {
    return EZ_FAILURE;
  }
  else
  {
    return EZ_SUCCESS;
  }
}

ezResult ezGALTextureResourceViewDX11::DeInitPlatform(ezGALDevice* pDevice)
{
  EZ_IGNORE_UNUSED(pDevice);

  EZ_GAL_DX11_RELEASE(m_pDXResourceView);
  return EZ_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////////

ezGALBufferResourceViewDX11::ezGALBufferResourceViewDX11(ezGALBuffer* pResource, const ezGALBufferResourceViewCreationDescription& Description)
  : ezGALBufferResourceView(pResource, Description)

{
}

ezGALBufferResourceViewDX11::~ezGALBufferResourceViewDX11() = default;

ezResult ezGALBufferResourceViewDX11::InitPlatform(ezGALDevice* pDevice)
{
  ezGALDeviceDX11* pDXDevice = static_cast<ezGALDeviceDX11*>(pDevice);
  const ezGALBuffer* pBuffer = pDevice->GetBuffer(m_Description.m_hBuffer);
  ID3D11Resource* pDXResource = static_cast<const ezGALBufferDX11*>(pBuffer)->GetDXBuffer();
  const ezGALBufferCreationDescription& bufferDesc = pBuffer->GetDescription();

  D3D11_SHADER_RESOURCE_VIEW_DESC DXSRVDesc;
  DXSRVDesc.Format = DXGI_FORMAT_UNKNOWN;
  DXSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
  DXSRVDesc.BufferEx.Flags = 0;

  switch (m_Description.m_ResourceType)
  {
    case ezGALShaderResourceType::TexelBuffer:
    {
      const ezGALResourceFormat::Enum viewFormat = m_Description.m_Format;
      const auto& formatInfo = pDXDevice->GetFormatLookupTable().GetFormatInfo(viewFormat);
      const ezUInt32 uiBytesPerElement = ezGALResourceFormat::GetBitsPerElement(viewFormat) / 8;

      DXSRVDesc.BufferEx.FirstElement = m_Description.m_uiByteOffset / uiBytesPerElement;
      DXSRVDesc.BufferEx.NumElements = m_Description.m_uiByteCount / uiBytesPerElement;
      DXSRVDesc.Format = ezGALResourceFormat::IsDepthFormat(viewFormat) ? formatInfo.m_eDepthOnlyType : formatInfo.m_eResourceViewType;
      if (DXSRVDesc.Format == DXGI_FORMAT_UNKNOWN)
      {
        ezLog::Error("Couldn't get valid DXGI format for resource view! ({0})", viewFormat);
        return EZ_FAILURE;
      }
    }
    break;
    case ezGALShaderResourceType::StructuredBuffer:
    {
      DXSRVDesc.BufferEx.FirstElement = m_Description.m_uiByteOffset / bufferDesc.m_uiStructSize;
      DXSRVDesc.BufferEx.NumElements = m_Description.m_uiByteCount / bufferDesc.m_uiStructSize;
    }
    break;
    case ezGALShaderResourceType::ByteAddressBuffer:
    {
      DXSRVDesc.Format = DXGI_FORMAT_R32_TYPELESS;
      DXSRVDesc.BufferEx.FirstElement = m_Description.m_uiByteOffset / 4;
      DXSRVDesc.BufferEx.NumElements = m_Description.m_uiByteCount / 4;
      DXSRVDesc.BufferEx.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
    }
    break;
    default:
      EZ_REPORT_FAILURE("Unsupported resource type: {}", (ezUInt32)m_Description.m_ResourceType);
  }

  if (FAILED(pDXDevice->GetDXDevice()->CreateShaderResourceView(pDXResource, &DXSRVDesc, &m_pDXResourceView)))
  {
    return EZ_FAILURE;
  }
  else
  {
    return EZ_SUCCESS;
  }
}

ezResult ezGALBufferResourceViewDX11::DeInitPlatform(ezGALDevice* pDevice)
{
  EZ_IGNORE_UNUSED(pDevice);

  EZ_GAL_DX11_RELEASE(m_pDXResourceView);
  return EZ_SUCCESS;
}
