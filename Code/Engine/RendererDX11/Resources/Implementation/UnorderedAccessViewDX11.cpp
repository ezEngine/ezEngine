#include <RendererDX11/PCH.h>
#include <RendererDX11/Resources/UnorderedAccessViewDX11.h>
#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Resources/TextureDX11.h>
#include <RendererDX11/Resources/BufferDX11.h>

#include <d3d11.h>


ezGALUnorderedAccessViewDX11::ezGALUnorderedAccessViewDX11(ezGALResourceBase* pResource, const ezGALUnorderedAccessViewCreationDescription& Description)
  : ezGALUnorderedAccessView(pResource, Description)
  , m_pDXUnorderedAccessView(nullptr)
{
}

ezGALUnorderedAccessViewDX11::~ezGALUnorderedAccessViewDX11()
{
}

ezResult ezGALUnorderedAccessViewDX11::InitPlatform(ezGALDevice* pDevice)
{
  const ezGALTextureDX11* pTexture = nullptr;
  if(!m_Description.m_hTexture.IsInvalidated())
    pTexture =  static_cast<const ezGALTextureDX11*>(pDevice->GetTexture(m_Description.m_hTexture));

  const ezGALBufferDX11* pBuffer = nullptr;
  if(!m_Description.m_hBuffer.IsInvalidated())
    pBuffer = static_cast<const ezGALBufferDX11*>(pDevice->GetBuffer(m_Description.m_hBuffer));

  if(pTexture == nullptr && pBuffer == nullptr)
  {
    ezLog::ErrorPrintf("No valid texture handle or buffer handle given for unordered access view creation!");
    return EZ_FAILURE;
  }


  ezGALResourceFormat::Enum ViewFormat = m_Description.m_OverrideViewFormat;

  if(pTexture)
  {
    const ezGALTextureCreationDescription& TexDesc = pTexture->GetDescription();

    if(ViewFormat == ezGALResourceFormat::Invalid)
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

  if(DXViewFormat == DXGI_FORMAT_UNKNOWN)
  {
    ezLog::ErrorPrintf("Couldn't get valid DXGI format for resource view! (%d)", ViewFormat);
    return EZ_FAILURE;
  }


  D3D11_UNORDERED_ACCESS_VIEW_DESC DXUAVDesc;
  DXUAVDesc.Format = DXViewFormat;

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
          DXUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
          DXUAVDesc.Texture2D.MipSlice = m_Description.m_uiMipLevelToUse;
        }
        else
        {
          DXUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
          DXUAVDesc.Texture2DArray.MipSlice = m_Description.m_uiMipLevelToUse;
          DXUAVDesc.Texture2DArray.ArraySize = m_Description.m_uiArraySize;
          DXUAVDesc.Texture2DArray.FirstArraySlice = m_Description.m_uiFirstArraySlice;
        }
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

  }
  else if(pBuffer)
  {
    pDXResource = pBuffer->GetDXBuffer();

    if (pBuffer->GetDescription().m_bUseAsStructuredBuffer)
      DXUAVDesc.Format = DXGI_FORMAT_UNKNOWN;

    DXUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    DXUAVDesc.Buffer.FirstElement = m_Description.m_uiFirstElement;
    DXUAVDesc.Buffer.NumElements = m_Description.m_uiNumElements;
    DXUAVDesc.Buffer.Flags = 0;
    if (m_Description.m_bRawView)
      DXUAVDesc.Buffer.Flags |= D3D11_BUFFER_UAV_FLAG_RAW;
    if (m_Description.m_bAppend)
      DXUAVDesc.Buffer.Flags |= D3D11_BUFFER_UAV_FLAG_APPEND;
  }

  if(FAILED(pDXDevice->GetDXDevice()->CreateUnorderedAccessView(pDXResource, &DXUAVDesc, &m_pDXUnorderedAccessView)))
  {
    return EZ_FAILURE;
  }
  else
  {
    return EZ_SUCCESS;
  }
}

ezResult ezGALUnorderedAccessViewDX11::DeInitPlatform(ezGALDevice* pDevice)
{
  EZ_GAL_DX11_RELEASE(m_pDXUnorderedAccessView);
  return EZ_SUCCESS;
}



EZ_STATICLINK_FILE(RendererDX11, RendererDX11_Resources_Implementation_UnorderedAccessViewDX11);

