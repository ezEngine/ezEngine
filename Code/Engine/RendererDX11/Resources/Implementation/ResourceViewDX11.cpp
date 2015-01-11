
#include <RendererDX11/PCH.h>
#include <RendererDX11/Resources/ResourceViewDX11.h>
#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Resources/TextureDX11.h>
#include <RendererDX11/Resources/BufferDX11.h>

#include <d3d11.h>


ezGALResourceViewDX11::ezGALResourceViewDX11(const ezGALResourceViewCreationDescription& Description)
  : ezGALResourceView(Description),
    m_pDXResourceView(nullptr)
{
}

ezGALResourceViewDX11::~ezGALResourceViewDX11()
{
}

ezResult ezGALResourceViewDX11::InitPlatform(ezGALDevice* pDevice) 
{
  const ezGALTextureDX11* pTexture = nullptr;
  if(!m_Description.m_hTexture.IsInvalidated())
    pTexture =  static_cast<const ezGALTextureDX11*>(pDevice->GetTexture(m_Description.m_hTexture));

  const ezGALBufferDX11* pBuffer = nullptr;
  if(!m_Description.m_hBuffer.IsInvalidated())
    pBuffer = static_cast<const ezGALBufferDX11*>(pDevice->GetBuffer(m_Description.m_hBuffer));

  if(pTexture == nullptr && pBuffer == nullptr)
  {
    ezLog::Error("No valid texture handle or buffer handle given for resource view creation!");
    return EZ_FAILURE;
  }


  ezGALResourceFormat::Enum ViewFormat = m_Description.m_OverrideViewFormat;

  if(pTexture)
  {
    const ezGALTextureCreationDescription& TexDesc = pTexture->GetDescription();

    if(ViewFormat == ezGALResourceFormat::Invalid)
      ViewFormat = TexDesc.m_Format;
  }
  else if(pBuffer)
  {
    ViewFormat = ezGALResourceFormat::RUInt;

    if(!pBuffer->GetDescription().m_bAllowRawViews && m_Description.m_bRawView)
    {
      ezLog::Error("Trying to creat a raw view for a buffer with no raw view flag is invalid!");
      return EZ_FAILURE;
    }
  }

  ezGALDeviceDX11* pDXDevice = static_cast<ezGALDeviceDX11*>(pDevice);

  DXGI_FORMAT DXViewFormat = pDXDevice->GetFormatLookupTable().GetFormatInfo(ViewFormat).m_eResourceViewType;

  if(DXViewFormat == DXGI_FORMAT_UNKNOWN)
  {
    ezLog::Error("Couldn't get valid DXGI format for resource view! (%d)", ViewFormat);
    return EZ_FAILURE;
  }


  D3D11_SHADER_RESOURCE_VIEW_DESC DXSRVDesc;
  DXSRVDesc.Format = DXViewFormat;

  ID3D11Resource* pDXResource = nullptr;

  if(pTexture)
  {
    pDXResource = pTexture->GetDXTexture();
    const ezGALTextureCreationDescription& TexDesc = pTexture->GetDescription();

    switch(TexDesc.m_Type)
    {
      case ezGALTextureType::Texture2D:

        if(TexDesc.m_uiArraySize == 1)
        {
          if(TexDesc.m_SampleCount == ezGALMSAASampleCount::None)
          {
            DXSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            DXSRVDesc.Texture2D.MipLevels = m_Description.m_uiMipLevelsToUse;
            DXSRVDesc.Texture2D.MostDetailedMip = m_Description.m_uiMostDetailedMipLevel;
          }
          else
          {
            DXSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
          }
        }
        else
        {
          if(TexDesc.m_SampleCount == ezGALMSAASampleCount::None)
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
        }

        break;

      case ezGALTextureType::TextureCube:

        if(TexDesc.m_uiArraySize == 1)
        {
          DXSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
          DXSRVDesc.TextureCube.MipLevels = m_Description.m_uiMipLevelsToUse;
          DXSRVDesc.TextureCube.MostDetailedMip = m_Description.m_uiMostDetailedMipLevel;
        }
        else
        {
          DXSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
          DXSRVDesc.TextureCube.MipLevels = m_Description.m_uiMipLevelsToUse;
          DXSRVDesc.TextureCube.MostDetailedMip = m_Description.m_uiMostDetailedMipLevel;
          DXSRVDesc.TextureCubeArray.NumCubes = m_Description.m_uiArraySize;
          DXSRVDesc.TextureCubeArray.First2DArrayFace = m_Description.m_uiFirstArraySlice;
        }

        break;

      case ezGALTextureType::Texture3D:

        DXSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
        DXSRVDesc.Texture3D.MipLevels = m_Description.m_uiMipLevelsToUse;
        DXSRVDesc.Texture3D.MostDetailedMip = m_Description.m_uiMostDetailedMipLevel;

        break;

      default:
        EZ_ASSERT_NOT_IMPLEMENTED;
        return EZ_FAILURE;
    }

  }
  else if(pBuffer)
  {
    /// \todo
    EZ_ASSERT_NOT_IMPLEMENTED;
  }

  if(FAILED(pDXDevice->GetDXDevice()->CreateShaderResourceView(pDXResource, &DXSRVDesc, &m_pDXResourceView)))
  {
    return EZ_FAILURE;
  }
  else
  {
    return EZ_SUCCESS;
  }
}

ezResult ezGALResourceViewDX11::DeInitPlatform(ezGALDevice* pDevice)
{
  EZ_GAL_DX11_RELEASE(m_pDXResourceView);
  return EZ_SUCCESS;
}



EZ_STATICLINK_FILE(RendererDX11, RendererDX11_Resources_Implementation_ResourceViewDX11);

