#include <RendererDX11/RendererDX11PCH.h>

#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Resources/RenderTargetViewDX11.h>
#include <RendererDX11/Resources/TextureDX11.h>

#include <d3d11.h>

ezGALRenderTargetViewDX11::ezGALRenderTargetViewDX11(ezGALTexture* pTexture, const ezGALRenderTargetViewCreationDescription& Description)
  : ezGALRenderTargetView(pTexture, Description)

{
}

ezGALRenderTargetViewDX11::~ezGALRenderTargetViewDX11() = default;

ezResult ezGALRenderTargetViewDX11::InitPlatform(ezGALDevice* pDevice)
{
  const ezGALTexture* pTexture = nullptr;
  if (!m_Description.m_hTexture.IsInvalidated())
    pTexture = pDevice->GetTexture(m_Description.m_hTexture);

  if (pTexture == nullptr)
  {
    ezLog::Error("No valid texture handle given for render target view creation!");
    return EZ_FAILURE;
  }

  const ezGALTextureCreationDescription& texDesc = pTexture->GetDescription();
  ezGALResourceFormat::Enum viewFormat = texDesc.m_Format;

  if (m_Description.m_OverrideViewFormat != ezGALResourceFormat::Invalid)
    viewFormat = m_Description.m_OverrideViewFormat;


  ezGALDeviceDX11* pDXDevice = static_cast<ezGALDeviceDX11*>(pDevice);

  DXGI_FORMAT DXViewFormat = DXGI_FORMAT_UNKNOWN;

  const bool bIsDepthFormat = ezGALResourceFormat::IsDepthFormat(viewFormat);
  if (bIsDepthFormat)
  {
    DXViewFormat = pDXDevice->GetFormatLookupTable().GetFormatInfo(viewFormat).m_eDepthStencilType;
  }
  else
  {
    DXViewFormat = pDXDevice->GetFormatLookupTable().GetFormatInfo(viewFormat).m_eRenderTarget;
  }

  if (DXViewFormat == DXGI_FORMAT_UNKNOWN)
  {
    ezLog::Error("Couldn't get DXGI format for view!");
    return EZ_FAILURE;
  }

  ID3D11Resource* pDXResource = static_cast<const ezGALTextureDX11*>(pTexture->GetParentResource())->GetDXTexture();

  if (bIsDepthFormat)
  {
    D3D11_DEPTH_STENCIL_VIEW_DESC DSViewDesc;
    DSViewDesc.Format = DXViewFormat;

    if (texDesc.m_SampleCount == ezGALMSAASampleCount::None)
    {
      switch (texDesc.m_Type)
      {
        case ezGALTextureType::Texture2D:
        case ezGALTextureType::Texture2DShared:
          DSViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
          DSViewDesc.Texture2D.MipSlice = m_Description.m_uiMipLevel;
          break;

        case ezGALTextureType::Texture2DProxy:
        case ezGALTextureType::Texture2DArray:
        case ezGALTextureType::TextureCube:
        case ezGALTextureType::TextureCubeArray:
          DSViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
          DSViewDesc.Texture2DArray.MipSlice = m_Description.m_uiMipLevel;
          DSViewDesc.Texture2DArray.FirstArraySlice = m_Description.m_uiFirstSlice;
          DSViewDesc.Texture2DArray.ArraySize = m_Description.m_uiSliceCount;
          break;

          EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
      }
    }
    else
    {
      switch (texDesc.m_Type)
      {
        case ezGALTextureType::Texture2D:
        case ezGALTextureType::Texture2DShared:
          DSViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
          break;

        case ezGALTextureType::Texture2DProxy:
        case ezGALTextureType::Texture2DArray:
        case ezGALTextureType::TextureCube:
        case ezGALTextureType::TextureCubeArray:
          DSViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY;
          DSViewDesc.Texture2DMSArray.FirstArraySlice = m_Description.m_uiFirstSlice;
          DSViewDesc.Texture2DMSArray.ArraySize = m_Description.m_uiSliceCount;
          break;

          EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
      }
    }

    DSViewDesc.Flags = 0;
    if (m_Description.m_bReadOnly)
      DSViewDesc.Flags |= (D3D11_DSV_READ_ONLY_DEPTH | D3D11_DSV_READ_ONLY_STENCIL);

    if (FAILED(pDXDevice->GetDXDevice()->CreateDepthStencilView(pDXResource, &DSViewDesc, &m_pDepthStencilView)))
    {
      ezLog::Error("Couldn't create depth stencil view!");
      return EZ_FAILURE;
    }
    else
    {
      return EZ_SUCCESS;
    }
  }
  else
  {
    D3D11_RENDER_TARGET_VIEW_DESC RTViewDesc;
    RTViewDesc.Format = DXViewFormat;

    if (texDesc.m_SampleCount == ezGALMSAASampleCount::None)
    {
      switch (texDesc.m_Type)
      {
        case ezGALTextureType::Texture2D:
        case ezGALTextureType::Texture2DShared:
          RTViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
          RTViewDesc.Texture2D.MipSlice = m_Description.m_uiMipLevel;
          break;

        case ezGALTextureType::Texture2DProxy:
        case ezGALTextureType::Texture2DArray:
        case ezGALTextureType::TextureCube:
        case ezGALTextureType::TextureCubeArray:
          RTViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
          RTViewDesc.Texture2DArray.MipSlice = m_Description.m_uiMipLevel;
          RTViewDesc.Texture2DArray.FirstArraySlice = m_Description.m_uiFirstSlice;
          RTViewDesc.Texture2DArray.ArraySize = m_Description.m_uiSliceCount;
          break;

          EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
      }
    }
    else
    {
      switch (texDesc.m_Type)
      {
        case ezGALTextureType::Texture2D:
        case ezGALTextureType::Texture2DShared:
          RTViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
          break;

        case ezGALTextureType::Texture2DProxy:
        case ezGALTextureType::Texture2DArray:
        case ezGALTextureType::TextureCube:
        case ezGALTextureType::TextureCubeArray:
          RTViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY;
          RTViewDesc.Texture2DMSArray.FirstArraySlice = m_Description.m_uiFirstSlice;
          RTViewDesc.Texture2DMSArray.ArraySize = m_Description.m_uiSliceCount;
          break;

          EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
      }
    }

    if (FAILED(pDXDevice->GetDXDevice()->CreateRenderTargetView(pDXResource, &RTViewDesc, &m_pRenderTargetView)))
    {
      ezLog::Error("Couldn't create render target view!");
      return EZ_FAILURE;
    }
    else
    {
      return EZ_SUCCESS;
    }
  }
}

ezResult ezGALRenderTargetViewDX11::DeInitPlatform(ezGALDevice* pDevice)
{
  EZ_IGNORE_UNUSED(pDevice);

  EZ_GAL_DX11_RELEASE(m_pRenderTargetView);
  EZ_GAL_DX11_RELEASE(m_pDepthStencilView);
  EZ_GAL_DX11_RELEASE(m_pUnorderedAccessView);

  return EZ_SUCCESS;
}
