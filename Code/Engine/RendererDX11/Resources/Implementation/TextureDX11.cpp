
#include <RendererDX11/PCH.h>
#include <RendererDX11/Basics.h>
#include <RendererDX11/Resources/TextureDX11.h>
#include <RendererDX11/Device/DeviceDX11.h>

#include <d3d11.h>

ezGALTextureDX11::ezGALTextureDX11(const ezGALTextureCreationDescription& Description)
  : ezGALTexture(Description), m_pDXTexture(nullptr), m_pDXStagingTexture(nullptr)
{

}

ezGALTextureDX11::~ezGALTextureDX11()
{

}

EZ_DEFINE_AS_POD_TYPE(D3D11_SUBRESOURCE_DATA);

ezResult ezGALTextureDX11::InitPlatform(ezGALDevice* pDevice, const ezArrayPtr<ezGALSystemMemoryDescription>* pInitialData)
{
  ezGALDeviceDX11* pDXDevice = static_cast<ezGALDeviceDX11*>(pDevice);

  if(m_Description.m_pExisitingNativeObject != nullptr)
  {
    /// \todo Validation if interface of corresponding texture object exists
    m_pDXTexture = static_cast<ID3D11Resource*>(m_Description.m_pExisitingNativeObject);

    if (!m_Description.m_ResourceAccess.IsImmutable() || m_Description.m_ResourceAccess.m_bReadBack)
      return CreateStagingTexture(pDXDevice);

    return EZ_SUCCESS;
  }


  switch(m_Description.m_Type)
  {
    case ezGALTextureType::Texture2D:
    case ezGALTextureType::TextureCube:
      {
        D3D11_TEXTURE2D_DESC Tex2DDesc;
        Tex2DDesc.ArraySize = (m_Description.m_Type == ezGALTextureType::Texture2D ? m_Description.m_uiArraySize : (m_Description.m_uiArraySize * 6));
        Tex2DDesc.BindFlags = 0;

        if(m_Description.m_bAllowShaderResourceView)
          Tex2DDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
        if(m_Description.m_bAllowUAV)
          Tex2DDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;

        /// \todo Marc: Should this maybe use some kind of flags like "IsDepthFormat" ?
        if(m_Description.m_bCreateRenderTarget)
          Tex2DDesc.BindFlags |= (m_Description.m_Format == ezGALResourceFormat::D24S8 || m_Description.m_Format == ezGALResourceFormat::DFloat ? D3D11_BIND_DEPTH_STENCIL : D3D11_BIND_RENDER_TARGET); /// \todo Get format info!

        Tex2DDesc.CPUAccessFlags = 0; // We always use staging textures to update the data
        Tex2DDesc.Usage = m_Description.m_ResourceAccess.IsImmutable() ? D3D11_USAGE_IMMUTABLE : D3D11_USAGE_DEFAULT;

        if (m_Description.m_bCreateRenderTarget || m_Description.m_bAllowUAV)
          Tex2DDesc.Usage = D3D11_USAGE_DEFAULT;
        
        Tex2DDesc.Format = pDXDevice->GetFormatLookupTable().GetFormatInfo(m_Description.m_Format).m_eStorage;

        if(Tex2DDesc.Format == DXGI_FORMAT_UNKNOWN)
        {
          ezLog::Error("No storage format available for given format: %d", m_Description.m_Format);
          return EZ_FAILURE;
        }

        Tex2DDesc.Width = m_Description.m_uiWidth;
        Tex2DDesc.Height = m_Description.m_uiHeight;
        Tex2DDesc.MipLevels = m_Description.m_uiMipSliceCount;

        Tex2DDesc.MiscFlags = 0;

        if(m_Description.m_bAllowDynamicMipGeneration)
          Tex2DDesc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;

        if(m_Description.m_Type == ezGALTextureType::TextureCube)
          Tex2DDesc.MiscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE;

        Tex2DDesc.SampleDesc.Count = m_Description.m_SampleCount;
        Tex2DDesc.SampleDesc.Quality = 0;

        ezHybridArray<D3D11_SUBRESOURCE_DATA, 16> InitialData;
        if(pInitialData != nullptr)
        {
          const ezUInt32 uiInitialDataCount = (m_Description.m_uiMipSliceCount * (m_Description.m_Type  == ezGALTextureType::Texture2D ? 1 : 6));
          EZ_ASSERT_DEV(pInitialData->GetCount() == uiInitialDataCount, "The array of initial data values is not equal to the amount of mip levels!");

          InitialData.SetCountUninitialized(uiInitialDataCount);

          for(ezUInt32 i = 0; i < uiInitialDataCount; i++)
          {
            InitialData[i].pSysMem = pInitialData->GetPtr()[i].m_pData;
            InitialData[i].SysMemPitch = pInitialData->GetPtr()[i].m_uiRowPitch;
            InitialData[i].SysMemSlicePitch = pInitialData->GetPtr()[i].m_uiSlicePitch;
          }
        }

        if(FAILED(pDXDevice->GetDXDevice()->CreateTexture2D(&Tex2DDesc, pInitialData != nullptr ? &InitialData[0] : nullptr, reinterpret_cast<ID3D11Texture2D**>(&m_pDXTexture))))
        {
          return EZ_FAILURE;
        }
        else
        {
          if (!m_Description.m_ResourceAccess.IsImmutable() || m_Description.m_ResourceAccess.m_bReadBack)
            return CreateStagingTexture(pDXDevice);

          return EZ_SUCCESS;
        }

      }
      break;

    case ezGALTextureType::Texture3D:
      EZ_ASSERT_NOT_IMPLEMENTED;
      break;

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
      return EZ_FAILURE;
  }


  return EZ_FAILURE;
}

ezResult ezGALTextureDX11::DeInitPlatform(ezGALDevice* pDevice)
{
  EZ_GAL_DX11_RELEASE(m_pDXTexture);
  EZ_GAL_DX11_RELEASE(m_pDXStagingTexture);
  return EZ_SUCCESS;
}

ezResult ezGALTextureDX11::CreateStagingTexture(ezGALDeviceDX11* pDevice)
{

  switch (m_Description.m_Type)
  {
    case ezGALTextureType::Texture2D:
    case ezGALTextureType::TextureCube:
      {
        D3D11_TEXTURE2D_DESC Desc;
        static_cast<ID3D11Texture2D*>(m_pDXTexture)->GetDesc(&Desc);
        Desc.BindFlags = 0;
        Desc.CPUAccessFlags = 0;
        Desc.Usage = D3D11_USAGE_STAGING;
        Desc.SampleDesc.Count = 1; // We need to disable MSAA for the readback texture, the conversion needs to happen during readback!

        if (m_Description.m_ResourceAccess.m_bReadBack)
          Desc.CPUAccessFlags |= D3D11_CPU_ACCESS_READ;
        if (!m_Description.m_ResourceAccess.IsImmutable())
          Desc.CPUAccessFlags |= D3D11_CPU_ACCESS_WRITE;

        if (FAILED(pDevice->GetDXDevice()->CreateTexture2D(&Desc, nullptr, reinterpret_cast<ID3D11Texture2D**>(&m_pDXStagingTexture))))
        {
          ezLog::Error("Couldn't create staging resource for data upload and/or read back!");
          return EZ_FAILURE;
        }

      }
      break;

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }



  return EZ_SUCCESS;
}



EZ_STATICLINK_FILE(RendererDX11, RendererDX11_Resources_Implementation_TextureDX11);

