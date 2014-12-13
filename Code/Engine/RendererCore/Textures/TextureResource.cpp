#include <RendererCore/PCH.h>
#include <RendererCore/Textures/TextureResource.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Configuration/Startup.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Context/Context.h>
#include <CoreUtils/Image/ImageConversion.h>

static ezTextureResourceLoader s_TextureResourceLoader;

EZ_BEGIN_SUBSYSTEM_DECLARATION(Graphics, TextureResource)

BEGIN_SUBSYSTEM_DEPENDENCIES
"Foundation",
"Core"
END_SUBSYSTEM_DEPENDENCIES

ON_CORE_STARTUP
{
  ezResourceManager::SetResourceTypeLoader<ezTextureResource>(&s_TextureResourceLoader);
}

ON_CORE_SHUTDOWN
{
  ezResourceManager::SetResourceTypeLoader<ezTextureResource>(nullptr);
}

ON_ENGINE_STARTUP
{
}

ON_ENGINE_SHUTDOWN
{
}

EZ_END_SUBSYSTEM_DECLARATION


EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTextureResource, ezResourceBase, 1, ezRTTIDefaultAllocator<ezTextureResource>);
EZ_END_DYNAMIC_REFLECTED_TYPE();


ezTextureResource::ezTextureResource()
{
  m_uiMaxQualityLevel = 1;
  m_Flags |= ezResourceFlags::UpdateOnMainThread;
}

void ezTextureResource::UnloadData(bool bFullUnload)
{
  m_uiLoadedQualityLevel = 0;
  m_LoadingState = ezResourceLoadState::Uninitialized;

  if (!m_hGALTexView.IsInvalidated())
  {
    ezGALDevice::GetDefaultDevice()->DestroyResourceView(m_hGALTexView);
    m_hGALTexView.Invalidate();
  }

  if (!m_hGALTexture.IsInvalidated())
  {
    ezGALDevice::GetDefaultDevice()->DestroyTexture(m_hGALTexture);
    m_hGALTexture.Invalidate();
  }

  if (!m_hSamplerState.IsInvalidated())
  {
    ezGALDevice::GetDefaultDevice()->DestroySamplerState(m_hSamplerState);
    m_hSamplerState.Invalidate();
  }
}

static ezGALResourceFormat::Enum ImgToGalFormat(ezImageFormat::Enum format)
{
  switch (format)
  {
  case ezImageFormat::R8G8B8A8_UNORM:
    return ezGALResourceFormat::RGBAUByteNormalized;

  //case ezImageFormat::R8G8B8A8_TYPELESS:
  case ezImageFormat::R8G8B8A8_UNORM_SRGB:
    return ezGALResourceFormat::RGBAUByteNormalizedsRGB;

  case ezImageFormat::R8G8B8A8_UINT:
    return ezGALResourceFormat::RGBAUInt;

  case ezImageFormat::R8G8B8A8_SNORM:
    return ezGALResourceFormat::RGBAByteNormalized;

  case ezImageFormat::R8G8B8A8_SINT:
    return ezGALResourceFormat::RGBAInt;

  case ezImageFormat::B8G8R8A8_UNORM:
    return ezGALResourceFormat::RGBAUByteNormalized; // HACK
    //return ezGALResourceFormat::BGRAUByteNormalized;

  case ezImageFormat::B8G8R8X8_UNORM:
    return ezGALResourceFormat::BGRAUByteNormalized;

  //case ezImageFormat::B8G8R8A8_TYPELESS:
  case ezImageFormat::B8G8R8A8_UNORM_SRGB:
    return ezGALResourceFormat::BGRAUByteNormalizedsRGB;

  //case ezImageFormat::B8G8R8X8_TYPELESS:
  case ezImageFormat::B8G8R8X8_UNORM_SRGB:
    return ezGALResourceFormat::BGRAUByteNormalizedsRGB;

  //case ezImageFormat::B8G8R8_UNORM:

  //case ezImageFormat::BC1_TYPELESS:
  case ezImageFormat::BC1_UNORM:
    return ezGALResourceFormat::BC1;

  case ezImageFormat::BC1_UNORM_SRGB:
    return ezGALResourceFormat::BC1sRGB;

  //case ezImageFormat::BC2_TYPELESS:
  case ezImageFormat::BC2_UNORM:
    return ezGALResourceFormat::BC2;

  case ezImageFormat::BC2_UNORM_SRGB:
    return ezGALResourceFormat::BC2sRGB;

  //case ezImageFormat::BC3_TYPELESS:
  case ezImageFormat::BC3_UNORM:
    return ezGALResourceFormat::BC3;

  case ezImageFormat::BC3_UNORM_SRGB:
    return ezGALResourceFormat::BC3sRGB;

  //case ezImageFormat::BC4_TYPELESS:
  case ezImageFormat::BC4_UNORM:
    return ezGALResourceFormat::BC4UNormalized;

  case ezImageFormat::BC4_SNORM:
    return ezGALResourceFormat::BC4Normalized;

  //case ezImageFormat::BC5_TYPELESS:
  case ezImageFormat::BC5_UNORM:
    return ezGALResourceFormat::BC5UNormalized;

  case ezImageFormat::BC5_SNORM:
    return ezGALResourceFormat::BC5Normalized;

  //case ezImageFormat::BC6H_TYPELESS:
  case ezImageFormat::BC6H_UF16:
    return ezGALResourceFormat::BC6UFloat;

  case ezImageFormat::BC6H_SF16:
    return ezGALResourceFormat::BC6Float;

  //case ezImageFormat::BC7_TYPELESS:
  case ezImageFormat::BC7_UNORM:
    return ezGALResourceFormat::BC7UNormalized;

  case ezImageFormat::BC7_UNORM_SRGB:
    return ezGALResourceFormat::BC7UNormalizedsRGB;

  default:
    EZ_ASSERT_NOT_IMPLEMENTED;
    break;
  }

  return ezGALResourceFormat::Invalid;
}

void ezTextureResource::UpdateContent(ezStreamReaderBase& Stream)
{
  bool bSuccess = false;
  Stream >> bSuccess;

  if (bSuccess)
  {
    ezImage* pImage = nullptr;
    Stream.ReadBytes(&pImage, sizeof(ezImage*));

    ezGALTextureCreationDescription TexDesc;
    TexDesc.m_Format = ezGALResourceFormat::RGBAUByteNormalized;
    TexDesc.m_uiWidth  = pImage->GetWidth();
    TexDesc.m_uiHeight = pImage->GetHeight();
    TexDesc.m_uiDepth  = pImage->GetDepth();
    TexDesc.m_uiMipSliceCount = pImage->GetNumMipLevels();
    TexDesc.m_uiArraySize = pImage->GetNumArrayIndices();

    if (TexDesc.m_uiDepth > 1)
      TexDesc.m_Type = ezGALTextureType::Texture3D;

    if (pImage->GetNumFaces() == 6)
      TexDesc.m_Type = ezGALTextureType::TextureCube;

    TexDesc.m_Format = ImgToGalFormat(pImage->GetImageFormat());

    ezGALSystemMemoryDescription InitData[1];
    InitData[0].m_pData = pImage->GetDataPointer<void>();
    InitData[0].m_uiRowPitch = pImage->GetRowPitch();
    InitData[0].m_uiSlicePitch = pImage->GetDepthPitch();

    //EZ_ASSERT(pImage->GetRowPitch() > 0, "");

    ezArrayPtr<ezGALSystemMemoryDescription> InitDataPtr(InitData);

    m_hGALTexture = ezGALDevice::GetDefaultDevice()->CreateTexture(TexDesc, &InitDataPtr);

    if (m_hGALTexture.IsInvalidated())
    {
      ezLog::Error("Texture state error");
    }

    ezGALResourceViewCreationDescription TexViewDesc;
    TexViewDesc.m_hTexture = m_hGALTexture;
    
    m_hGALTexView = ezGALDevice::GetDefaultDevice()->CreateResourceView(TexViewDesc);

    if (m_hGALTexView.IsInvalidated())
    {
      ezLog::Error("Texview state error");
    }

    /// \todo HACK
    {
      ezGALSamplerStateCreationDescription SamplerDesc;
      SamplerDesc.m_MagFilter = SamplerDesc.m_MinFilter = SamplerDesc.m_MipFilter = ezGALTextureFilterMode::Point;
      m_hSamplerState = ezGALDevice::GetDefaultDevice()->CreateSamplerState(SamplerDesc);

      if (m_hSamplerState.IsInvalidated())
      {
        ezLog::Error("Sampler state error");
      }

      ezGALDevice::GetDefaultDevice()->GetPrimaryContext()->SetResourceView(ezGALShaderStage::PixelShader, 0, m_hGALTexView);
      ezGALDevice::GetDefaultDevice()->GetPrimaryContext()->SetSamplerState(ezGALShaderStage::PixelShader, 0, m_hSamplerState);
    }
  }
  else
  {
  }

  m_LoadingState = ezResourceLoadState::Loaded;
  m_uiLoadedQualityLevel = 1;
  m_uiMaxQualityLevel = 1;
}

void ezTextureResource::UpdateMemoryUsage()
{
  //SetMemoryUsageCPU(0);
  //SetMemoryUsageGPU(0);
}

void ezTextureResource::CreateResource(const ezTextureResourceDescriptor& descriptor)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

ezResourceLoadData ezTextureResourceLoader::OpenDataStream(const ezResourceBase* pResource)
{
  bool bSuccess = false;

  LoadedData* pData = EZ_DEFAULT_NEW(LoadedData);

  if (pData->m_Image.LoadFrom(pResource->GetResourceID()).Succeeded())
  {
    bSuccess = true;

    if (pData->m_Image.GetImageFormat() == ezImageFormat::B8G8R8_UNORM)
    {
      ezImageConversionBase::Convert(pData->m_Image, pData->m_Image, ezImageFormat::B8G8R8A8_UNORM);
    }
  }

  ezMemoryStreamWriter w(&pData->m_Storage);
  w << bSuccess;

  if (bSuccess)
  {
    ezImage* pImage = &pData->m_Image;
    w.WriteBytes(&pImage, sizeof(ezImage*));
  }

  ezResourceLoadData LoaderData;
  LoaderData.m_pDataStream = &pData->m_Reader;
  LoaderData.m_pCustomLoaderData = pData;

  return LoaderData;
}

void ezTextureResourceLoader::CloseDataStream(const ezResourceBase* pResource, const ezResourceLoadData& LoaderData)
{
  LoadedData* pData = (LoadedData*) LoaderData.m_pCustomLoaderData;

  EZ_DEFAULT_DELETE(pData);
}
