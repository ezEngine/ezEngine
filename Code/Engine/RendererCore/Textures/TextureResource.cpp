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

static ezGALResourceFormat::Enum ImgToGalFormat(ezImageFormat::Enum format, bool bSRGB)
{
  switch (format)
  {
  case ezImageFormat::R8G8B8A8_UNORM:
    if (bSRGB)
      return ezGALResourceFormat::RGBAUByteNormalizedsRGB;
    else
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
    if (bSRGB)
      return ezGALResourceFormat::BGRAUByteNormalizedsRGB;
    else
      return ezGALResourceFormat::BGRAUByteNormalized;

  case ezImageFormat::B8G8R8X8_UNORM:
    if (bSRGB)
      return ezGALResourceFormat::BGRAUByteNormalizedsRGB;
    else
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
    if (bSRGB)
      return ezGALResourceFormat::BC1sRGB;
    else
      return ezGALResourceFormat::BC1;

  case ezImageFormat::BC1_UNORM_SRGB:
    return ezGALResourceFormat::BC1sRGB;

  //case ezImageFormat::BC2_TYPELESS:
  case ezImageFormat::BC2_UNORM:
    if (bSRGB)
      return ezGALResourceFormat::BC2sRGB;
    else
      return ezGALResourceFormat::BC2;

  case ezImageFormat::BC2_UNORM_SRGB:
    return ezGALResourceFormat::BC2sRGB;

  //case ezImageFormat::BC3_TYPELESS:
  case ezImageFormat::BC3_UNORM:
    if (bSRGB)
      return ezGALResourceFormat::BC3sRGB;
    else
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
    if (bSRGB)
      return ezGALResourceFormat::BC7UNormalizedsRGB;
    else
      return ezGALResourceFormat::BC7UNormalized;

  case ezImageFormat::BC7_UNORM_SRGB:
    return ezGALResourceFormat::BC7UNormalizedsRGB;

  case ezImageFormat::B5G6R5_UNORM:
    return ezGALResourceFormat::B5G6R5UNormalized; /// \todo Not supported by some GPUs ?

  default:
    EZ_ASSERT_NOT_IMPLEMENTED;
    break;
  }

  return ezGALResourceFormat::Invalid;
}

static ezUInt32 GetBCnMemPitchFactor(ezImageFormat::Enum format)
{
  /// \todo Finish (verify) this function

  switch (format)
  {
  case ezImageFormat::BC1_TYPELESS:
  case ezImageFormat::BC1_UNORM:
  case ezImageFormat::BC1_UNORM_SRGB:
    return 2;
    break;
  case ezImageFormat::BC2_TYPELESS:
  case ezImageFormat::BC2_UNORM:
  case ezImageFormat::BC2_UNORM_SRGB:
  case ezImageFormat::BC3_TYPELESS:
  case ezImageFormat::BC3_UNORM:
  case ezImageFormat::BC3_UNORM_SRGB:
    return 4;
    //case ezImageFormat::BC4_TYPELESS:
    //case ezImageFormat::BC4_UNORM:
    //case ezImageFormat::BC4_SNORM:
    //  uiMemPitchFactor = 8;
    //  break;
    //case ezImageFormat::BC5_TYPELESS:
    //case ezImageFormat::BC5_UNORM:
    //case ezImageFormat::BC5_SNORM:
    //case ezImageFormat::BC6H_TYPELESS:
    //case ezImageFormat::BC6H_UF16:
    //case ezImageFormat::BC6H_SF16:
    //case ezImageFormat::BC7_TYPELESS:
    //case ezImageFormat::BC7_UNORM:
    //case ezImageFormat::BC7_UNORM_SRGB:
    //  return 4;
  default:
    EZ_ASSERT_NOT_IMPLEMENTED;
    break;
  }

  return 1;
}

void ezTextureResource::UpdateContent(ezStreamReaderBase* Stream)
{
  bool bSuccess = false;
  *Stream >> bSuccess;

  if (bSuccess)
  {
    ezImage* pImage = nullptr;
    Stream->ReadBytes(&pImage, sizeof(ezImage*));

    bool bSRGB = false;
    *Stream >> bSRGB;

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

    EZ_ASSERT(pImage->GetNumFaces() == 1 || pImage->GetNumFaces() == 6, "Invalid number of image faces (resource: '%s')", GetResourceID().GetData());

    TexDesc.m_Format = ImgToGalFormat(pImage->GetImageFormat(), bSRGB);


    ezHybridArray<ezGALSystemMemoryDescription, 32> InitData;

    /// \todo Figure out the correct order of the arrays/faces/mips
    for (ezUInt32 array_index = 0; array_index < pImage->GetNumArrayIndices(); ++array_index)
    {
      for (ezUInt32 face = 0; face < pImage->GetNumFaces(); ++face)
      {
        for (ezUInt32 mip = 0; mip < pImage->GetNumMipLevels(); ++mip)
        {
          ezGALSystemMemoryDescription& id = InitData.ExpandAndGetRef();

          id.m_pData = pImage->GetDataPointer<ezUInt8>() + pImage->GetDataOffSet(mip, face, array_index);

          if (ezImageFormat::GetType(pImage->GetImageFormat()) == ezImageFormatType::BLOCK_COMPRESSED)
          {
            const ezUInt32 uiMemPitchFactor = GetBCnMemPitchFactor(pImage->GetImageFormat());

            id.m_uiRowPitch = pImage->GetWidth(mip) * uiMemPitchFactor;
            id.m_uiSlicePitch = id.m_uiRowPitch * pImage->GetHeight(mip) / uiMemPitchFactor;
          }
          else
          {
            id.m_uiRowPitch = pImage->GetRowPitch(mip);
            id.m_uiSlicePitch = pImage->GetDepthPitch(mip);
          }
        }
      }
    }

    const ezArrayPtr<ezGALSystemMemoryDescription> InitDataPtr(InitData);

    m_hGALTexture = ezGALDevice::GetDefaultDevice()->CreateTexture(TexDesc, &InitDataPtr);

    if (m_hGALTexture.IsInvalidated())
    {
      /// \todo Fallback
      ezLog::Error("Texture state error");
    }

    ezGALResourceViewCreationDescription TexViewDesc;
    TexViewDesc.m_hTexture = m_hGALTexture;

    m_hGALTexView = ezGALDevice::GetDefaultDevice()->CreateResourceView(TexViewDesc);

    EZ_ASSERT(!m_hGALTexView.IsInvalidated(), "No resource view could be created for texture '%s'. Maybe the format table is incorrect?", GetResourceID().GetData());

    /// \todo HACK
    {
      ezGALSamplerStateCreationDescription SamplerDesc;
      SamplerDesc.m_MagFilter = ezGALTextureFilterMode::Linear;
      SamplerDesc.m_MinFilter = ezGALTextureFilterMode::Point;
      SamplerDesc.m_MipFilter = ezGALTextureFilterMode::Point;

      if (pImage->GetNumMipLevels() > 1)
      {
        SamplerDesc.m_MinFilter = ezGALTextureFilterMode::Linear;
        SamplerDesc.m_MipFilter = ezGALTextureFilterMode::Linear;
      }

      m_hSamplerState = ezGALDevice::GetDefaultDevice()->CreateSamplerState(SamplerDesc);

      if (m_hSamplerState.IsInvalidated())
      {
        ezLog::Error("Sampler state error");
      }
    }
  }
  else
  {
    /// \todo Setup fallback texture
    ezLog::Error("Loading the texture failed: '%s'", GetResourceID().GetData());
  }

  m_LoadingState = ezResourceLoadState::Loaded;
  m_uiLoadedQualityLevel = 1;
  m_uiMaxQualityLevel = 1;
}

void ezTextureResource::UpdateMemoryUsage()
{
  /// \todo Compute memory usage

  //SetMemoryUsageCPU(0);
  //SetMemoryUsageGPU(0);
}

void ezTextureResource::CreateResource(const ezTextureResourceDescriptor& descriptor)
{
  /// \todo Implement texture creation
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

    /// \todo As long as we don't have a custom format or asset meta data, this is a hack to get the SRGB information for the texture

    const ezStringBuilder sName = ezPathUtils::GetFileName(pResource->GetResourceID());

    bool bSRGB = (sName.EndsWith_NoCase("_D") || sName.EndsWith_NoCase("_SRGB"));

    w << bSRGB;
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
