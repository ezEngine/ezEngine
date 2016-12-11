#include <RendererCore/PCH.h>
#include <RendererCore/Textures/TextureResource.h>
#include <RendererFoundation/Resources/Texture.h>
#include <CoreUtils/Image/Formats/DdsFileFormat.h>
#include <CoreUtils/Assets/AssetFileHeader.h>
#include <RendererCore/RenderContext/RenderContext.h>

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


EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTextureResource, 1, ezRTTIDefaultAllocator<ezTextureResource>);
EZ_END_DYNAMIC_REFLECTED_TYPE

bool ezTextureResource::s_bForceFullQualityAlways = false;

ezTextureResource::ezTextureResource() : ezResource<ezTextureResource, ezTextureResourceDescriptor>(DoUpdate::OnAnyThread, s_bForceFullQualityAlways ? 1 : 2)
{
  m_uiLoadedTextures = 0;
  m_uiMemoryGPU[0] = 0;
  m_uiMemoryGPU[1] = 0;
  m_Format = ezGALResourceFormat::Invalid;
  m_uiWidth = 0;
  m_uiHeight = 0;
  m_Type = ezGALTextureType::Invalid;
}

ezResourceLoadDesc ezTextureResource::UnloadData(Unload WhatToUnload)
{
  if (m_uiLoadedTextures > 0)
  {
    for (ezInt32 r = 0; r < 2; ++r)
    {
      --m_uiLoadedTextures;

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
    /// \todo This only works because samplers are shared.
    /// Since they are not kept alive by ezRenderContext, this will crash if a sampler actually is deleted, but still in use by D3D.

    if (!m_hSamplerState.IsInvalidated())
    {
      ezGALDevice::GetDefaultDevice()->DestroySamplerState(m_hSamplerState);
      m_hSamplerState.Invalidate();
    }

    if (!m_hOldSamplerState.IsInvalidated())
    {
      ezGALDevice::GetDefaultDevice()->DestroySamplerState(m_hOldSamplerState);
      m_hOldSamplerState.Invalidate();
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

ezResourceLoadDesc ezTextureResource::UpdateContent(ezStreamReader* Stream)
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

  ezEnum<ezGALTextureAddressMode> addressModeU;
  ezEnum<ezGALTextureAddressMode> addressModeV;
  ezEnum<ezGALTextureAddressMode> addressModeW;
  ezEnum<ezTextureFilterSetting> textureFilter;
  *Stream >> addressModeU;
  *Stream >> addressModeV;
  *Stream >> addressModeW;
  *Stream >> textureFilter;

  const ezUInt32 uiNumMipmapsLowRes = s_bForceFullQualityAlways ? pImage->GetNumMipLevels() : 6;

  const ezUInt32 uiNumMipLevels = ezMath::Min(m_uiLoadedTextures == 0 ? uiNumMipmapsLowRes : pImage->GetNumMipLevels(), pImage->GetNumMipLevels());
  const ezUInt32 uiHighestMipLevel = pImage->GetNumMipLevels() - uiNumMipLevels;

  m_Format = ImgToGalFormat(pImage->GetImageFormat(), bSRGB);
  m_uiWidth = pImage->GetWidth(uiHighestMipLevel);
  m_uiHeight = pImage->GetHeight(uiHighestMipLevel);

  ezGALTextureCreationDescription texDesc;
  texDesc.m_Format = m_Format;
  texDesc.m_uiWidth = m_uiWidth;
  texDesc.m_uiHeight = m_uiHeight;
  texDesc.m_uiDepth = pImage->GetDepth(uiHighestMipLevel);
  texDesc.m_uiMipLevelCount = uiNumMipLevels;
  texDesc.m_uiArraySize = pImage->GetNumArrayIndices();

  if (texDesc.m_uiDepth > 1)
    texDesc.m_Type = ezGALTextureType::Texture3D;

  if (pImage->GetNumFaces() == 6)
    texDesc.m_Type = ezGALTextureType::TextureCube;

  m_Type = texDesc.m_Type;

  EZ_ASSERT_DEV(pImage->GetNumFaces() == 1 || pImage->GetNumFaces() == 6, "Invalid number of image faces (resource: '%s')", GetResourceID().GetData());

  m_uiMemoryGPU[m_uiLoadedTextures] = 0;

  ezHybridArray<ezGALSystemMemoryDescription, 32> InitData;

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
          const ezUInt32 uiMemPitchFactor = ezGALResourceFormat::GetBitsPerElement(m_Format) * 4 / 8;

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
  td.m_DescGAL = texDesc;
  td.m_SamplerDesc.m_AddressU = addressModeU;
  td.m_SamplerDesc.m_AddressV = addressModeV;
  td.m_SamplerDesc.m_AddressW = addressModeW;
  td.m_InitialContent = InitDataPtr;

  td.ConfigureSampler(textureFilter);

  // ignore its return value here, we build our own
  CreateResource(td);

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

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  m_Format = descriptor.m_DescGAL.m_Format;
  m_uiWidth = descriptor.m_DescGAL.m_uiWidth;
  m_uiHeight = descriptor.m_DescGAL.m_uiHeight;

  m_hGALTexture[m_uiLoadedTextures] = pDevice->CreateTexture(descriptor.m_DescGAL, descriptor.m_InitialContent);
  EZ_ASSERT_DEV(!m_hGALTexture[m_uiLoadedTextures].IsInvalidated(), "Texture Data could not be uploaded to the GPU");

  pDevice->GetTexture(m_hGALTexture[m_uiLoadedTextures])->SetDebugName(GetResourceDescription());

  SetupSamplerState(pDevice, descriptor);

  ++m_uiLoadedTextures;

  return ret;
}

void ezTextureResourceDescriptor::ConfigureSampler(ezTextureFilterSetting::Enum filter)
{
  const ezTextureFilterSetting::Enum thisFilter = ezRenderContext::GetDefaultInstance()->GetSpecificTextureFilter(filter);

  m_SamplerDesc.m_MinFilter = ezGALTextureFilterMode::Linear;
  m_SamplerDesc.m_MagFilter = ezGALTextureFilterMode::Linear;
  m_SamplerDesc.m_MipFilter = ezGALTextureFilterMode::Linear;
  m_SamplerDesc.m_uiMaxAnisotropy = 1;

  switch (thisFilter)
  {
  case ezTextureFilterSetting::FixedNearest:
    m_SamplerDesc.m_MinFilter = ezGALTextureFilterMode::Point;
    m_SamplerDesc.m_MagFilter = ezGALTextureFilterMode::Point;
    m_SamplerDesc.m_MipFilter = ezGALTextureFilterMode::Point;
    break;
  case ezTextureFilterSetting::FixedBilinear:
    m_SamplerDesc.m_MipFilter = ezGALTextureFilterMode::Point;
    break;
  case ezTextureFilterSetting::FixedTrilinear:
    break;
  case ezTextureFilterSetting::FixedAnisotropic2x:
    m_SamplerDesc.m_MinFilter = ezGALTextureFilterMode::Anisotropic;
    m_SamplerDesc.m_MagFilter = ezGALTextureFilterMode::Anisotropic;
    m_SamplerDesc.m_uiMaxAnisotropy = 2;
    break;
  case ezTextureFilterSetting::FixedAnisotropic4x:
    m_SamplerDesc.m_MinFilter = ezGALTextureFilterMode::Anisotropic;
    m_SamplerDesc.m_MagFilter = ezGALTextureFilterMode::Anisotropic;
    m_SamplerDesc.m_uiMaxAnisotropy = 4;
    break;
  case ezTextureFilterSetting::FixedAnisotropic8x:
    m_SamplerDesc.m_MinFilter = ezGALTextureFilterMode::Anisotropic;
    m_SamplerDesc.m_MagFilter = ezGALTextureFilterMode::Anisotropic;
    m_SamplerDesc.m_uiMaxAnisotropy = 8;
    break;
  case ezTextureFilterSetting::FixedAnisotropic16x:
    m_SamplerDesc.m_MinFilter = ezGALTextureFilterMode::Anisotropic;
    m_SamplerDesc.m_MagFilter = ezGALTextureFilterMode::Anisotropic;
    m_SamplerDesc.m_uiMaxAnisotropy = 16;
    break;
  }
}


void ezTextureResource::SetupSamplerState(ezGALDevice* pDevice, const ezTextureResourceDescriptor &descriptor)
{
  /// \todo This is a hack! The sampler state is still bound in ezRenderContext and if we destroy it here, it might be deleted entirely
  /// we would need to do ref-counting of sampler states, but that is currently not really implemented.
  /// Se we keep the old sampler around for a bit longer, and hope it is not in use anymore on the second change.

  ezGALSamplerStateHandle hNewSampler = pDevice->CreateSamplerState(descriptor.m_SamplerDesc);
  EZ_ASSERT_DEV(!hNewSampler.IsInvalidated(), "Sampler state error");

  if (hNewSampler == m_hSamplerState)
  {
    // make sure ref-count is correct
    pDevice->DestroySamplerState(m_hSamplerState);
    m_hSamplerState = hNewSampler;
    return;
  }

  // first time
  if (m_hSamplerState.IsInvalidated())
  {
    m_hSamplerState = hNewSampler;
    return;
  }

  {
    // get rid of a previous sampler
    if (!m_hOldSamplerState.IsInvalidated())
    {
      pDevice->DestroySamplerState(m_hOldSamplerState);
      m_hOldSamplerState.Invalidate();
    }

    // store the old sampler
    m_hOldSamplerState = m_hSamplerState;

    // take the new one from now on
    m_hSamplerState = hNewSampler;
  }
}


ezResourceLoadData ezTextureResourceLoader::OpenDataStream(const ezResourceBase* pResource)
{
  LoadedData* pData = EZ_DEFAULT_NEW(LoadedData);

  ezResourceLoadData res;

  bool bSRGB = false;
  ezEnum<ezGALTextureAddressMode> addressModeU = ezGALTextureAddressMode::Wrap;
  ezEnum<ezGALTextureAddressMode> addressModeV = ezGALTextureAddressMode::Wrap;
  ezEnum<ezGALTextureAddressMode> addressModeW = ezGALTextureAddressMode::Wrap;
  ezEnum<ezTextureFilterSetting> textureFilter = ezTextureFilterSetting::Default;

  // Solid Color Textures
  if (ezPathUtils::HasExtension(pResource->GetResourceID(), "color"))
  {
    ezStringBuilder sName = pResource->GetResourceID();
    sName.RemoveFileExtension();

    bool bValidColor = false;
    const ezColorGammaUB color = ezConversionUtils::GetColorByName(sName, &bValidColor);

    if (!bValidColor)
    {
      ezLog::Error("'{0}' is not a valid color name. Using 'RebeccaPurple' as fallback.", sName.GetData());
    }

    bSRGB = true;
    pData->m_Image.SetWidth(4);
    pData->m_Image.SetHeight(4);
    pData->m_Image.SetDepth(1);
    pData->m_Image.SetImageFormat(ezImageFormat::R8G8B8A8_UNORM_SRGB);
    pData->m_Image.SetNumMipLevels(1);
    pData->m_Image.SetNumFaces(1);
    pData->m_Image.AllocateImageData();
    ezUInt8* pPixels = pData->m_Image.GetPixelPointer<ezUInt8>();

    for (ezUInt32 px = 0; px < 4 * 4 * 4; px += 4)
    {
      pPixels[px + 0] = color.r;
      pPixels[px + 1] = color.g;
      pPixels[px + 2] = color.b;
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
      ezUInt8 uiTexFileFormatVersion = 0;
      File >> uiTexFileFormatVersion;

      if (uiTexFileFormatVersion < 2)
      {
        // There is no version 0 or 1, some idiot forgot to add a version number to the format.
        // However, the first byte in those versions is a bool, so either 0 or 1 and therefore we can detect
        // old versions by making the minimum file format version 2. I'm so clever! And handsome.

        bSRGB = (uiTexFileFormatVersion == 1);
        uiTexFileFormatVersion = 1;
      }
      else
      {
        File >> bSRGB;
      }

      File >> addressModeU;
      File >> addressModeV;
      File >> addressModeW;

      if (uiTexFileFormatVersion >= 2)
      {
        ezUInt8 uiFilter = 0;
        File >> uiFilter;
        textureFilter = (ezTextureFilterSetting::Enum) uiFilter;
      }

      ezDdsFileFormat fmt;
      if (fmt.ReadImage(File, pData->m_Image, ezGlobalLog::GetOrCreateInstance()).Failed())
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

        ezLog::Warning("Texture resource uses inefficient BGR format, converting to BGRX: '{0}'", sAbsolutePath.GetData());
        if (ezImageConversion::Convert(pData->m_Image, pData->m_Image, ezImageFormat::B8G8R8A8_UNORM).Failed())
          return res;
      }
    }
  }

  ezMemoryStreamWriter w(&pData->m_Storage);

  ezImage* pImage = &pData->m_Image;
  w.WriteBytes(&pImage, sizeof(ezImage*));

  w << bSRGB;
  w << addressModeU;
  w << addressModeV;
  w << addressModeW;
  w << textureFilter;

  res.m_pDataStream = &pData->m_Reader;
  res.m_pCustomLoaderData = pData;

  return res;
}

void ezTextureResourceLoader::CloseDataStream(const ezResourceBase* pResource, const ezResourceLoadData& LoaderData)
{
  LoadedData* pData = (LoadedData*)LoaderData.m_pCustomLoaderData;

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
    if (ezFileSystem::ResolvePath(pResource->GetResourceID(), &sAbs, nullptr).Failed())
      return false;

    ezFileStats stat;
    if (ezOSFile::GetFileStats(sAbs, stat).Failed())
      return false;

    return !stat.m_LastModificationTime.Compare(pResource->GetLoadedFileModificationTime(), ezTimestamp::CompareMode::FileTimeEqual);
  }

#endif

  return true;
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Textures_TextureResource);

