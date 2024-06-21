#include <RendererDX11/RendererDX11PCH.h>

#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Resources/TextureDX11.h>

#include <d3d11.h>

ezGALTextureDX11::ezGALTextureDX11(const ezGALTextureCreationDescription& Description)
  : ezGALTexture(Description)
{
}

ezGALTextureDX11::~ezGALTextureDX11() = default;

ezResult ezGALTextureDX11::InitPlatform(ezGALDevice* pDevice, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData)
{
  ezGALDeviceDX11* pDXDevice = static_cast<ezGALDeviceDX11*>(pDevice);

  if (m_Description.m_pExisitingNativeObject != nullptr)
  {
    return InitFromNativeObject(pDXDevice);
  }

  switch (m_Description.m_Type)
  {
    case ezGALTextureType::Texture2D:
    case ezGALTextureType::TextureCube:
    {
      D3D11_TEXTURE2D_DESC Tex2DDesc;
      EZ_SUCCEED_OR_RETURN(Create2DDesc(m_Description, pDXDevice, Tex2DDesc));

      ezHybridArray<D3D11_SUBRESOURCE_DATA, 16> InitialData;
      ConvertInitialData(m_Description, pInitialData, InitialData);

      if (FAILED(pDXDevice->GetDXDevice()->CreateTexture2D(&Tex2DDesc, pInitialData.IsEmpty() ? nullptr : &InitialData[0], reinterpret_cast<ID3D11Texture2D**>(&m_pDXTexture))))
      {
        return EZ_FAILURE;
      }
    }
    break;

    case ezGALTextureType::Texture3D:
    {
      D3D11_TEXTURE3D_DESC Tex3DDesc;
      EZ_SUCCEED_OR_RETURN(Create3DDesc(m_Description, pDXDevice, Tex3DDesc));

      ezHybridArray<D3D11_SUBRESOURCE_DATA, 16> InitialData;
      ConvertInitialData(m_Description, pInitialData, InitialData);

      if (FAILED(pDXDevice->GetDXDevice()->CreateTexture3D(&Tex3DDesc, pInitialData.IsEmpty() ? nullptr : &InitialData[0], reinterpret_cast<ID3D11Texture3D**>(&m_pDXTexture))))
      {
        return EZ_FAILURE;
      }
    }
    break;

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
      return EZ_FAILURE;
  }

  if (!m_Description.m_ResourceAccess.IsImmutable() || m_Description.m_ResourceAccess.m_bReadBack)
    return CreateStagingTexture(pDXDevice);

  return EZ_SUCCESS;
}


ezResult ezGALTextureDX11::InitFromNativeObject(ezGALDeviceDX11* pDXDevice)
{
  /// \todo Validation if interface of corresponding texture object exists
  m_pDXTexture = static_cast<ID3D11Resource*>(m_Description.m_pExisitingNativeObject);
  if (!m_Description.m_ResourceAccess.IsImmutable() || m_Description.m_ResourceAccess.m_bReadBack)
  {
    ezResult res = CreateStagingTexture(pDXDevice);
    if (res == EZ_FAILURE)
    {
      m_pDXTexture = nullptr;
      return res;
    }
  }
  return EZ_SUCCESS;
}


ezResult ezGALTextureDX11::Create2DDesc(const ezGALTextureCreationDescription& description, ezGALDeviceDX11* pDXDevice, D3D11_TEXTURE2D_DESC& out_Tex2DDesc)
{
  out_Tex2DDesc.ArraySize = (description.m_Type == ezGALTextureType::Texture2D ? description.m_uiArraySize : (description.m_uiArraySize * 6));
  out_Tex2DDesc.BindFlags = 0;

  if (description.m_bAllowShaderResourceView || description.m_bAllowDynamicMipGeneration)
    out_Tex2DDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
  if (description.m_bAllowUAV)
    out_Tex2DDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;

  if (description.m_bCreateRenderTarget || description.m_bAllowDynamicMipGeneration)
    out_Tex2DDesc.BindFlags |= ezGALResourceFormat::IsDepthFormat(description.m_Format) ? D3D11_BIND_DEPTH_STENCIL : D3D11_BIND_RENDER_TARGET;

  out_Tex2DDesc.CPUAccessFlags = 0; // We always use staging textures to update the data
  out_Tex2DDesc.Usage = description.m_ResourceAccess.IsImmutable() ? D3D11_USAGE_IMMUTABLE : D3D11_USAGE_DEFAULT;

  if (description.m_bCreateRenderTarget || description.m_bAllowUAV)
    out_Tex2DDesc.Usage = D3D11_USAGE_DEFAULT;

  out_Tex2DDesc.Format = pDXDevice->GetFormatLookupTable().GetFormatInfo(description.m_Format).m_eStorage;

  if (out_Tex2DDesc.Format == DXGI_FORMAT_UNKNOWN)
  {
    ezLog::Error("No storage format available for given format: {0}", description.m_Format);
    return EZ_FAILURE;
  }

  out_Tex2DDesc.Width = description.m_uiWidth;
  out_Tex2DDesc.Height = description.m_uiHeight;
  out_Tex2DDesc.MipLevels = description.m_uiMipLevelCount;

  out_Tex2DDesc.MiscFlags = 0;

  if (description.m_bAllowDynamicMipGeneration)
    out_Tex2DDesc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;

  if (description.m_Type == ezGALTextureType::TextureCube)
    out_Tex2DDesc.MiscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE;

  out_Tex2DDesc.SampleDesc.Count = description.m_SampleCount;
  out_Tex2DDesc.SampleDesc.Quality = 0;
  return EZ_SUCCESS;
}

ezResult ezGALTextureDX11::Create3DDesc(const ezGALTextureCreationDescription& description, ezGALDeviceDX11* pDXDevice, D3D11_TEXTURE3D_DESC& out_Tex3DDesc)
{
  out_Tex3DDesc.BindFlags = 0;

  if (description.m_bAllowShaderResourceView)
    out_Tex3DDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
  if (description.m_bAllowUAV)
    out_Tex3DDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;

  if (description.m_bCreateRenderTarget)
    out_Tex3DDesc.BindFlags |= ezGALResourceFormat::IsDepthFormat(description.m_Format) ? D3D11_BIND_DEPTH_STENCIL : D3D11_BIND_RENDER_TARGET;

  out_Tex3DDesc.CPUAccessFlags = 0; // We always use staging textures to update the data
  out_Tex3DDesc.Usage = description.m_ResourceAccess.IsImmutable() ? D3D11_USAGE_IMMUTABLE : D3D11_USAGE_DEFAULT;

  if (description.m_bCreateRenderTarget || description.m_bAllowUAV)
    out_Tex3DDesc.Usage = D3D11_USAGE_DEFAULT;

  out_Tex3DDesc.Format = pDXDevice->GetFormatLookupTable().GetFormatInfo(description.m_Format).m_eStorage;

  if (out_Tex3DDesc.Format == DXGI_FORMAT_UNKNOWN)
  {
    ezLog::Error("No storage format available for given format: {0}", description.m_Format);
    return EZ_FAILURE;
  }

  out_Tex3DDesc.Width = description.m_uiWidth;
  out_Tex3DDesc.Height = description.m_uiHeight;
  out_Tex3DDesc.Depth = description.m_uiDepth;
  out_Tex3DDesc.MipLevels = description.m_uiMipLevelCount;

  out_Tex3DDesc.MiscFlags = 0;

  if (description.m_bAllowDynamicMipGeneration)
    out_Tex3DDesc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;

  if (description.m_Type == ezGALTextureType::TextureCube)
    out_Tex3DDesc.MiscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE;

  return EZ_SUCCESS;
}


void ezGALTextureDX11::ConvertInitialData(const ezGALTextureCreationDescription& description, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData, ezHybridArray<D3D11_SUBRESOURCE_DATA, 16>& out_InitialData)
{
  if (!pInitialData.IsEmpty())
  {
    ezUInt32 uiArraySize = 1;
    switch (description.m_Type)
    {
      case ezGALTextureType::Texture2D:
        uiArraySize = description.m_uiArraySize;
        break;
      case ezGALTextureType::TextureCube:
        uiArraySize = description.m_uiArraySize * 6;
        break;
      case ezGALTextureType::Texture3D:
      default:
        break;
    }
    const ezUInt32 uiInitialDataCount = (description.m_uiMipLevelCount * uiArraySize);

    EZ_ASSERT_DEV(pInitialData.GetCount() == uiInitialDataCount, "The array of initial data values is not equal to the amount of mip levels!");

    out_InitialData.SetCountUninitialized(uiInitialDataCount);

    for (ezUInt32 i = 0; i < uiInitialDataCount; i++)
    {
      out_InitialData[i].pSysMem = pInitialData[i].m_pData;
      out_InitialData[i].SysMemPitch = pInitialData[i].m_uiRowPitch;
      out_InitialData[i].SysMemSlicePitch = pInitialData[i].m_uiSlicePitch;
    }
  }
}

ezResult ezGALTextureDX11::DeInitPlatform(ezGALDevice* pDevice)
{
  EZ_IGNORE_UNUSED(pDevice);

  EZ_GAL_DX11_RELEASE(m_pDXTexture);
  EZ_GAL_DX11_RELEASE(m_pDXStagingTexture);
  return EZ_SUCCESS;
}

void ezGALTextureDX11::SetDebugNamePlatform(const char* szName) const
{
  ezUInt32 uiLength = ezStringUtils::GetStringElementCount(szName);

  if (m_pDXTexture != nullptr)
  {
    m_pDXTexture->SetPrivateData(WKPDID_D3DDebugObjectName, uiLength, szName);
  }
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
      // Need to remove this flag on the staging resource or texture readback no longer works.
      Desc.MiscFlags &= ~D3D11_RESOURCE_MISC_GENERATE_MIPS;
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


