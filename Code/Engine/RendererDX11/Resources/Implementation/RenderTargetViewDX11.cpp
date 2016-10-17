
#include <RendererDX11/PCH.h>
#include <RendererDX11/Resources/RenderTargetViewDX11.h>
#include <RendererDX11/Resources/TextureDX11.h>
#include <RendererDX11/Device/DeviceDX11.h>

#include <d3d11.h>

ezGALRenderTargetViewDX11::ezGALRenderTargetViewDX11(ezGALTexture* pTexture, const ezGALRenderTargetViewCreationDescription& Description)
  : ezGALRenderTargetView(pTexture, Description)
  , m_pRenderTargetView(nullptr)
  , m_pDepthStencilView(nullptr)
  , m_pUnorderedAccessView(nullptr)
{
}

ezGALRenderTargetViewDX11::~ezGALRenderTargetViewDX11()
{
}

ezResult ezGALRenderTargetViewDX11::InitPlatform(ezGALDevice* pDevice)
{
  const ezGALTextureDX11* pTexture = nullptr;
  if(!m_Description.m_hTexture.IsInvalidated())
    pTexture = static_cast<const ezGALTextureDX11*>(pDevice->GetTexture(m_Description.m_hTexture));
    
  if (pTexture == nullptr)
  {
    ezLog::Error("No valid texture handle given for rendertarget view creation!");
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

  if(DXViewFormat == DXGI_FORMAT_UNKNOWN)
  {
    ezLog::Error("Couldn't get DXGI format for view!");
    return EZ_FAILURE;
  }  

  if (bIsDepthFormat)
  {
    D3D11_DEPTH_STENCIL_VIEW_DESC DSViewDesc;
    DSViewDesc.Format = DXViewFormat;

    if (pTexture->GetDescription().m_SampleCount == ezGALMSAASampleCount::None)
    {
      DSViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
      DSViewDesc.Texture2D.MipSlice = m_Description.m_uiMipLevel;
    }
    else
    {
      DSViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
    }

    DSViewDesc.Flags = 0;
    if (m_Description.m_bReadOnly)
      DSViewDesc.Flags |= (D3D11_DSV_READ_ONLY_DEPTH | D3D11_DSV_READ_ONLY_STENCIL);

    if (FAILED(pDXDevice->GetDXDevice()->CreateDepthStencilView(pTexture->GetDXTexture(), &DSViewDesc, &m_pDepthStencilView)))
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

    if (pTexture->GetDescription().m_SampleCount == ezGALMSAASampleCount::None)
    {
      RTViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
      RTViewDesc.Texture2D.MipSlice = m_Description.m_uiMipLevel;
    }
    else
    {
      RTViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
    }

    if (FAILED(pDXDevice->GetDXDevice()->CreateRenderTargetView(pTexture->GetDXTexture(), &RTViewDesc, &m_pRenderTargetView)))
    {
      ezLog::Error("Couldn't create rendertarget view!");
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
  EZ_GAL_DX11_RELEASE(m_pRenderTargetView);
  EZ_GAL_DX11_RELEASE(m_pDepthStencilView);
  EZ_GAL_DX11_RELEASE(m_pUnorderedAccessView);

  return EZ_SUCCESS;
}



EZ_STATICLINK_FILE(RendererDX11, RendererDX11_Resources_Implementation_RenderTargetViewDX11);

