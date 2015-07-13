#include <RendererCore/PCH.h>
#include <RendererCore/Textures/TextureResource.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Shader/ShaderPermutationBinary.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Configuration/Startup.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Context/Context.h>
#include <CoreUtils/Image/ImageConversion.h>
#include <CoreUtils/Image/Formats/DdsFileFormat.h>
#include <CoreUtils/Assets/AssetFileHeader.h>

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

bool ezTextureResource::s_bForceFullQualityAlways = false;

ezTextureResource::ezTextureResource() : ezResource<ezTextureResource, ezTextureResourceDescriptor>(DoUpdate::OnMainThread, s_bForceFullQualityAlways ? 1 : 2)
{
  m_uiLoadedTextures = 0;
  m_uiMemoryGPU[0] = 0;
  m_uiMemoryGPU[1] = 0;
}

ezResourceLoadDesc ezTextureResource::UnloadData(Unload WhatToUnload)
{
  if (m_uiLoadedTextures > 0)
  {
    for (ezInt32 r = 0; r < 2; ++r)
    {
      --m_uiLoadedTextures;

      if (!m_hGALTexView[m_uiLoadedTextures].IsInvalidated())
      {
        ezGALDevice::GetDefaultDevice()->DestroyResourceView(m_hGALTexView[m_uiLoadedTextures]);
        m_hGALTexView[m_uiLoadedTextures].Invalidate();
      }

      if (!m_hGALTexture[m_uiLoadedTextures].IsInvalidated())
      {
        ezGALDevice::GetDefaultDevice()->DestroyTexture(m_hGALTexture[m_uiLoadedTextures]);
        m_hGALTexture[m_uiLoadedTextures].Invalidate();
      }

      m_uiMemoryGPU[m_uiLoadedTextures] = 0;

      if (WhatToUnload == Unload::OneQualityLevel || m_uiLoadedTextures == 0)
        break;
    }
  }

  if (WhatToUnload == Unload::AllQualityLevels)
  {
    if (!m_hSamplerState.IsInvalidated())
    {
      ezGALDevice::GetDefaultDevice()->DestroySamplerState(m_hSamplerState);
      m_hSamplerState.Invalidate();
    }
  }

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = m_uiLoadedTextures;
  res.m_uiQualityLevelsLoadable = 2 - m_uiLoadedTextures;
  res.m_State = m_uiLoadedTextures == 0 ? ezResourceState::Unloaded : ezResourceState::Loaded;

  return res;
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

ezResourceLoadDesc ezTextureResource::UpdateContent(ezStreamReaderBase* Stream)
{
  if (Stream == nullptr)
  {
    ezResourceLoadDesc res;
    res.m_uiQualityLevelsDiscardable = 0;
    res.m_uiQualityLevelsLoadable = 0;
    res.m_State = ezResourceState::LoadedResourceMissing;

    return res;
  }

  ezImage* pImage = nullptr;
  Stream->ReadBytes(&pImage, sizeof(ezImage*));

  bool bSRGB = false;
  *Stream >> bSRGB;

  const ezUInt32 uiNumMipmapsLowRes = s_bForceFullQualityAlways ? pImage->GetNumMipLevels() : 6;

  const ezUInt32 uiNumMipLevels = ezMath::Min(m_uiLoadedTextures == 0 ? uiNumMipmapsLowRes : pImage->GetNumMipLevels(), pImage->GetNumMipLevels());
  const ezUInt32 uiHighestMipLevel = pImage->GetNumMipLevels() - uiNumMipLevels;

  ezGALTextureCreationDescription TexDesc;
  TexDesc.m_Format = ezGALResourceFormat::RGBAUByteNormalized;
  TexDesc.m_uiWidth = pImage->GetWidth(uiHighestMipLevel);
  TexDesc.m_uiHeight = pImage->GetHeight(uiHighestMipLevel);
  TexDesc.m_uiDepth = pImage->GetDepth(uiHighestMipLevel);
  TexDesc.m_uiMipSliceCount = uiNumMipLevels;
  TexDesc.m_uiArraySize = pImage->GetNumArrayIndices();

  if (TexDesc.m_uiDepth > 1)
    TexDesc.m_Type = ezGALTextureType::Texture3D;

  if (pImage->GetNumFaces() == 6)
    TexDesc.m_Type = ezGALTextureType::TextureCube;

  EZ_ASSERT_DEV(pImage->GetNumFaces() == 1 || pImage->GetNumFaces() == 6, "Invalid number of image faces (resource: '%s')", GetResourceID().GetData());

  TexDesc.m_Format = ImgToGalFormat(pImage->GetImageFormat(), bSRGB);

  m_uiMemoryGPU[m_uiLoadedTextures] = 0;

  ezHybridArray<ezGALSystemMemoryDescription, 32> InitData;

  /// \todo Figure out the correct order of the arrays/faces/mips
  for (ezUInt32 array_index = 0; array_index < pImage->GetNumArrayIndices(); ++array_index)
  {
    for (ezUInt32 face = 0; face < pImage->GetNumFaces(); ++face)
    {
      for (ezUInt32 mip = uiHighestMipLevel; mip < pImage->GetNumMipLevels(); ++mip)
      {
        ezGALSystemMemoryDescription& id = InitData.ExpandAndGetRef();

        id.m_pData = pImage->GetDataPointer<ezUInt8>() + pImage->GetDataOffSet(mip, face, array_index);

        if (ezImageFormat::GetType(pImage->GetImageFormat()) == ezImageFormatType::BLOCK_COMPRESSED)
        {
          const ezUInt32 uiMemPitchFactor = GetBCnMemPitchFactor(pImage->GetImageFormat());

          id.m_uiRowPitch = pImage->GetWidth(mip) * uiMemPitchFactor;
        }
        else
        {
          id.m_uiRowPitch = pImage->GetRowPitch(mip);
        }

        id.m_uiSlicePitch = pImage->GetDepthPitch(mip);

        m_uiMemoryGPU[m_uiLoadedTextures] += id.m_uiSlicePitch;
      }
    }
  }

  const ezArrayPtr<ezGALSystemMemoryDescription> InitDataPtr(InitData);

  ezTextureResourceDescriptor td;
  td.m_DescGAL = TexDesc;
  td.m_InitialContent = InitDataPtr;

  // ignore its return value here, we build our own
  CreateResource(td);

  //m_hGALTexture[m_uiLoadedTextures] = ezGALDevice::GetDefaultDevice()->CreateTexture(TexDesc, &InitDataPtr);

  //EZ_ASSERT_DEV(!m_hGALTexture[m_uiLoadedTextures].IsInvalidated(), "Texture Data could not be uploaded to the GPU");

  //ezGALResourceViewCreationDescription TexViewDesc;
  //TexViewDesc.m_hTexture = m_hGALTexture[m_uiLoadedTextures];

  //m_hGALTexView[m_uiLoadedTextures] = ezGALDevice::GetDefaultDevice()->CreateResourceView(TexViewDesc);

  //EZ_ASSERT_DEV(!m_hGALTexView[m_uiLoadedTextures].IsInvalidated(), "No resource view could be created for texture '%s'. Maybe the format table is incorrect?", GetResourceID().GetData());

  ///// \todo HACK
  //if (m_hSamplerState.IsInvalidated())
  //{
  //  ezGALSamplerStateCreationDescription SamplerDesc;
  //  SamplerDesc.m_MagFilter = ezGALTextureFilterMode::Linear;
  //  SamplerDesc.m_MinFilter = ezGALTextureFilterMode::Point;
  //  SamplerDesc.m_MipFilter = ezGALTextureFilterMode::Point;

  //  if (pImage->GetNumMipLevels() > 1)
  //  {
  //    SamplerDesc.m_MinFilter = ezGALTextureFilterMode::Linear;
  //    SamplerDesc.m_MipFilter = ezGALTextureFilterMode::Linear;
  //  }

  //  m_hSamplerState = ezGALDevice::GetDefaultDevice()->CreateSamplerState(SamplerDesc);

  //  EZ_ASSERT_DEV(!m_hSamplerState.IsInvalidated(), "Sampler state error");
  //}

  //++m_uiLoadedTextures;

  {
    ezResourceLoadDesc res;
    res.m_uiQualityLevelsDiscardable = m_uiLoadedTextures;

    if (uiHighestMipLevel == 0)
      res.m_uiQualityLevelsLoadable = 0;
    else
      res.m_uiQualityLevelsLoadable = 1;

    res.m_State = ezResourceState::Loaded;

    return res;
  }
}

void ezTextureResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(ezTextureResource);
  out_NewMemoryUsage.m_uiMemoryGPU = m_uiMemoryGPU[0] + m_uiMemoryGPU[1];
}

ezResourceLoadDesc ezTextureResource::CreateResource(const ezTextureResourceDescriptor& descriptor)
{
  ezResourceLoadDesc ret;
  ret.m_uiQualityLevelsDiscardable = descriptor.m_uiQualityLevelsDiscardable;
  ret.m_uiQualityLevelsLoadable = descriptor.m_uiQualityLevelsLoadable;
  ret.m_State = ezResourceState::Loaded;

  m_hGALTexture[m_uiLoadedTextures] = ezGALDevice::GetDefaultDevice()->CreateTexture(descriptor.m_DescGAL, &descriptor.m_InitialContent);

  EZ_ASSERT_DEV(!m_hGALTexture[m_uiLoadedTextures].IsInvalidated(), "Texture Data could not be uploaded to the GPU");

  ezGALResourceViewCreationDescription TexViewDesc;
  TexViewDesc.m_hTexture = m_hGALTexture[m_uiLoadedTextures];

  m_hGALTexView[m_uiLoadedTextures] = ezGALDevice::GetDefaultDevice()->CreateResourceView(TexViewDesc);

  EZ_ASSERT_DEV(!m_hGALTexView[m_uiLoadedTextures].IsInvalidated(), "No resource view could be created for texture '%s'. Maybe the format table is incorrect?", GetResourceID().GetData());

  /// \todo HACK
  if (m_hSamplerState.IsInvalidated())
  {
    ezGALSamplerStateCreationDescription SamplerDesc;
    SamplerDesc.m_MagFilter = ezGALTextureFilterMode::Linear;
    SamplerDesc.m_MinFilter = ezGALTextureFilterMode::Point;
    SamplerDesc.m_MipFilter = ezGALTextureFilterMode::Point;

    if (descriptor.m_DescGAL.m_uiMipSliceCount > 1)
    {
      SamplerDesc.m_MinFilter = ezGALTextureFilterMode::Linear;
      SamplerDesc.m_MipFilter = ezGALTextureFilterMode::Linear;
    }

    m_hSamplerState = ezGALDevice::GetDefaultDevice()->CreateSamplerState(SamplerDesc);

    EZ_ASSERT_DEV(!m_hSamplerState.IsInvalidated(), "Sampler state error");
  }

  ++m_uiLoadedTextures;

  return ret;
}

ezResourceLoadData ezTextureResourceLoader::OpenDataStream(const ezResourceBase* pResource)
{
  LoadedData* pData = EZ_DEFAULT_NEW(LoadedData);

  ezResourceLoadData res;

  bool bSRGB = false;

  // Solid Color Textures
  if (ezPathUtils::HasExtension(pResource->GetResourceID(), "color"))
  {
    ezStringBuilder sName = pResource->GetResourceID();
    sName.RemoveFileExtension();

    const ezColorGammaUB color = ezConversionUtils::GetColorByName(sName);

    bSRGB = true;
    pData->m_Image.SetWidth(4);
    pData->m_Image.SetHeight(4);
    pData->m_Image.SetDepth(1);
    pData->m_Image.SetImageFormat(ezImageFormat::B8G8R8A8_UNORM_SRGB);
    pData->m_Image.SetNumMipLevels(1);
    pData->m_Image.SetNumFaces(1);
    pData->m_Image.AllocateImageData();
    ezUInt8* pPixels = pData->m_Image.GetPixelPointer<ezUInt8>();

    for (ezUInt32 px = 0; px < 4 * 4 * 4; px += 4)
    {
      pPixels[px + 0] = color.b;
      pPixels[px + 1] = color.g;
      pPixels[px + 2] = color.r;
      pPixels[px + 3] = color.a;
    }
  }
  else
  {
    ezFileReader File;
    if (File.Open(pResource->GetResourceID().GetData()).Failed())
      return res;

    const ezStringBuilder sAbsolutePath = File.GetFilePathAbsolute();
    res.m_sResourceDescription = File.GetFilePathRelative().GetData();

#if EZ_ENABLED(EZ_SUPPORTS_FILE_STATS)
    {
      ezFileStats stat;
      if (ezOSFile::GetFileStats(sAbsolutePath, stat).Succeeded())
      {
        res.m_LoadedFileModificationDate = stat.m_LastModificationTime;
      }
    }
#endif

    /// In case this is not a proper asset (ezTex format), this is a hack to get the SRGB information for the texture
    const ezStringBuilder sName = ezPathUtils::GetFileName(sAbsolutePath);
    bSRGB = (sName.EndsWith_NoCase("_D") || sName.EndsWith_NoCase("_SRGB") || sName.EndsWith_NoCase("_diff"));

    if (sAbsolutePath.HasExtension("ezTex"))
    {
      // read the hash, ignore it
      ezAssetFileHeader AssetHash;
      AssetHash.Read(File);

      // read the ezTex file format
      File >> bSRGB;

      ezDdsFileFormat fmt;
      if (fmt.ReadImage(File, pData->m_Image, ezGlobalLog::GetInstance()).Failed())
        return res;
    }
    else
    {
      // read whatever format, as long as ezImage supports it
      File.Close();

      if (pData->m_Image.LoadFrom(pResource->GetResourceID()).Failed())
        return res;

      if (pData->m_Image.GetImageFormat() == ezImageFormat::B8G8R8_UNORM)
      {
        /// \todo A conversion to B8G8R8X8_UNORM currently fails

        ezLog::Warning("Texture resource uses inefficient BGR format, converting to BGRX: '%s'", sAbsolutePath.GetData());
        if (ezImageConversionBase::Convert(pData->m_Image, pData->m_Image, ezImageFormat::B8G8R8A8_UNORM).Failed())
          return res;
      }
    }
  }

  ezMemoryStreamWriter w(&pData->m_Storage);

  ezImage* pImage = &pData->m_Image;
  w.WriteBytes(&pImage, sizeof(ezImage*));

  w << bSRGB;

  res.m_pDataStream = &pData->m_Reader;
  res.m_pCustomLoaderData = pData;

  return res;
}

void ezTextureResourceLoader::CloseDataStream(const ezResourceBase* pResource, const ezResourceLoadData& LoaderData)
{
  LoadedData* pData = (LoadedData*) LoaderData.m_pCustomLoaderData;

  EZ_DEFAULT_DELETE(pData);
}

bool ezTextureResourceLoader::IsResourceOutdated(const ezResourceBase* pResource) const
{
  // solid color textures are never outdated
  if (ezPathUtils::HasExtension(pResource->GetResourceID(), "color"))
    return false;

#if EZ_ENABLED(EZ_SUPPORTS_FILE_STATS)
  if (pResource->GetLoadedFileModificationTime().IsValid())
  {
    ezString sAbs;
    if (ezFileSystem::ResolvePath(pResource->GetResourceID(), false, &sAbs, nullptr).Failed())
      return false;

    ezFileStats stat;
    if (ezOSFile::GetFileStats(sAbs, stat).Failed())
      return false;
  
    return !stat.m_LastModificationTime.IsEqual(pResource->GetLoadedFileModificationTime(), ezTimestamp::CompareMode::FileTime);
  }

#endif

  return true;
}


void ezRenderContext::BindTexture(const ezTempHashedString& sSlotName, const ezTextureResourceHandle& hTexture)
{
  m_BoundTextures[sSlotName.GetHash()] = hTexture;

  m_StateFlags.Add(ezRenderContextFlags::TextureBindingChanged);
}

void ezRenderContext::ApplyTextureBindings(ezGALShaderStage::Enum stage, const ezShaderStageBinary* pBinary)
{
  for (const auto& rb : pBinary->m_ShaderResourceBindings)
  {
    if (rb.m_Type == ezShaderStageResource::ConstantBuffer)
      continue;

    const ezUInt32 uiResourceHash = rb.m_Name.GetHash();

    ezTextureResourceHandle* hTexture;
    if (!m_BoundTextures.TryGetValue(uiResourceHash, hTexture))
    {
      ezLog::Error("No resource is bound for shader slot '%s'", rb.m_Name.GetData());
      continue;
    }

    if (hTexture == nullptr || !hTexture->IsValid())
    {
      ezLog::Error("An invalid resource is bound for shader slot '%s'", rb.m_Name.GetData());
      continue;
    }

    ezResourceLock<ezTextureResource> l(*hTexture, ezResourceAcquireMode::AllowFallback);

    m_pGALContext->SetResourceView(stage, rb.m_iSlot, l->GetGALTextureView());
    m_pGALContext->SetSamplerState(stage, rb.m_iSlot, l->GetGALSamplerState());
  }
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Textures_TextureResource);

