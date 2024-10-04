
#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Utilities/AssetFileHeader.h>

#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/Image.h>

#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Textures/Texture3DResource.h>
#include <RendererCore/Textures/TextureUtils.h>
#include <RendererFoundation/Resources/Texture.h>
#include <Texture/ezTexFormat/ezTexFormat.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTexture3DResource, 1, ezRTTIDefaultAllocator<ezTexture3DResource>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

EZ_RESOURCE_IMPLEMENT_COMMON_CODE(ezTexture3DResource);

ezTexture3DResource::ezTexture3DResource()
  : ezResource(DoUpdate::OnGraphicsResourceThreads, ezTextureUtils::s_bForceFullQualityAlways ? 1 : 2)
{
}

ezTexture3DResource::ezTexture3DResource(ezResource::DoUpdate ResourceUpdateThread)
  : ezResource(ResourceUpdateThread, ezTextureUtils::s_bForceFullQualityAlways ? 1 : 2)
{
}

ezResourceLoadDesc ezTexture3DResource::UnloadData(Unload WhatToUnload)
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

void ezTexture3DResource::FillOutDescriptor(ezTexture3DResourceDescriptor& ref_td, const ezImage* pImage, bool bSRGB, ezUInt32 uiNumMipLevels,
  ezUInt32& out_uiMemoryUsed, ezHybridArray<ezGALSystemMemoryDescription, 32>& ref_initData)
{
  const ezUInt32 uiHighestMipLevel = pImage->GetNumMipLevels() - uiNumMipLevels;

  const ezGALResourceFormat::Enum format = ezTextureUtils::ImageFormatToGalFormat(pImage->GetImageFormat(), bSRGB);

  ref_td.m_DescGAL.m_Format = format;
  ref_td.m_DescGAL.m_uiWidth = pImage->GetWidth(uiHighestMipLevel);
  ref_td.m_DescGAL.m_uiHeight = pImage->GetHeight(uiHighestMipLevel);
  ref_td.m_DescGAL.m_uiDepth = pImage->GetDepth(uiHighestMipLevel);
  ref_td.m_DescGAL.m_uiMipLevelCount = uiNumMipLevels;
  ref_td.m_DescGAL.m_uiArraySize = pImage->GetNumArrayIndices();

  if (ref_td.m_DescGAL.m_uiDepth > 1)
    ref_td.m_DescGAL.m_Type = ezGALTextureType::Texture3D;

  out_uiMemoryUsed = 0;

  ref_initData.Clear();

  for (ezUInt32 array_index = 0; array_index < pImage->GetNumArrayIndices(); ++array_index)
  {
    for (ezUInt32 face = 0; face < pImage->GetNumFaces(); ++face)
    {
      for (ezUInt32 mip = uiHighestMipLevel; mip < pImage->GetNumMipLevels(); ++mip)
      {
        ezGALSystemMemoryDescription& id = ref_initData.ExpandAndGetRef();

        id.m_pData = pImage->GetSubImageView(mip, face, array_index).GetByteBlobPtr();

        if (ezImageFormat::GetType(pImage->GetImageFormat()) == ezImageFormatType::BLOCK_COMPRESSED)
        {
          const ezUInt32 uiMemPitchFactor = ezGALResourceFormat::GetBitsPerElement(format) * 4 / 8;

          id.m_uiRowPitch = ezMath::Max<ezUInt32>(4, pImage->GetWidth(mip)) * uiMemPitchFactor;
        }
        else
        {
          id.m_uiRowPitch = static_cast<ezUInt32>(pImage->GetRowPitch(mip));
        }

        EZ_ASSERT_DEV(pImage->GetDepthPitch(mip) < ezMath::MaxValue<ezUInt32>(), "Depth pitch exceeds ezGAL limits.");
        id.m_uiSlicePitch = static_cast<ezUInt32>(pImage->GetDepthPitch(mip));

        out_uiMemoryUsed += id.m_uiSlicePitch;
      }
    }
  }

  const ezArrayPtr<ezGALSystemMemoryDescription> InitDataPtr(ref_initData);

  ref_td.m_InitialContent = InitDataPtr;
}


ezResourceLoadDesc ezTexture3DResource::UpdateContent(ezStreamReader* Stream)
{
  if (Stream == nullptr)
  {
    ezResourceLoadDesc res;
    res.m_uiQualityLevelsDiscardable = 0;
    res.m_uiQualityLevelsLoadable = 0;
    res.m_State = ezResourceState::LoadedResourceMissing;

    return res;
  }

  ezTexture3DResourceDescriptor td;
  ezImage* pImage = nullptr;
  bool bIsFallback = false;
  ezTexFormat texFormat;

  // load image data
  {
    Stream->ReadBytes(&pImage, sizeof(ezImage*));
    *Stream >> bIsFallback;
    texFormat.ReadHeader(*Stream);

    td.m_SamplerDesc.m_AddressU = texFormat.m_AddressModeU;
    td.m_SamplerDesc.m_AddressV = texFormat.m_AddressModeV;
    td.m_SamplerDesc.m_AddressW = texFormat.m_AddressModeW;
  }

  const bool bIsRenderTarget = texFormat.m_iRenderTargetResolutionX != 0;
  EZ_ASSERT_DEV(!bIsRenderTarget, "Render targets are not supported by regular 2D texture resources");

  {

    const ezUInt32 uiNumMipmapsLowRes =
      ezTextureUtils::s_bForceFullQualityAlways ? pImage->GetNumMipLevels() : ezMath::Min(pImage->GetNumMipLevels(), 6U);
    ezUInt32 uiUploadNumMipLevels = 0;
    bool bCouldLoadMore = false;

    if (bIsFallback)
    {
      if (m_uiLoadedTextures == 0)
      {
        // only upload fallback textures, if we don't have any texture data at all, yet
        bCouldLoadMore = true;
        uiUploadNumMipLevels = uiNumMipmapsLowRes;
      }
      else if (m_uiLoadedTextures == 1)
      {
        // ignore this texture entirely, if we already have low res data
        // but assume we could load a higher resolution version
        bCouldLoadMore = true;
        ezLog::Debug("Ignoring fallback texture data, low-res resource data is already loaded.");
      }
      else
      {
        ezLog::Debug("Ignoring fallback texture data, resource is already fully loaded.");
      }
    }
    else
    {
      if (m_uiLoadedTextures == 0)
      {
        bCouldLoadMore = uiNumMipmapsLowRes < pImage->GetNumMipLevels();
        uiUploadNumMipLevels = uiNumMipmapsLowRes;
      }
      else if (m_uiLoadedTextures == 1)
      {
        uiUploadNumMipLevels = pImage->GetNumMipLevels();
      }
      else
      {
        // ignore the texture, if we already have fully loaded data
        ezLog::Debug("Ignoring texture data, resource is already fully loaded.");
      }
    }

    if (uiUploadNumMipLevels > 0)
    {
      EZ_ASSERT_DEBUG(m_uiLoadedTextures < 2, "Invalid texture upload");

      ezHybridArray<ezGALSystemMemoryDescription, 32> initData;
      FillOutDescriptor(td, pImage, texFormat.m_bSRGB, uiUploadNumMipLevels, m_uiMemoryGPU[m_uiLoadedTextures], initData);

      ezTextureUtils::ConfigureSampler(static_cast<ezTextureFilterSetting::Enum>(texFormat.m_TextureFilter.GetValue()), td.m_SamplerDesc);

      // ignore its return value here, we build our own
      CreateResource(std::move(td));
    }

    {
      ezResourceLoadDesc res;
      res.m_uiQualityLevelsDiscardable = m_uiLoadedTextures;
      res.m_uiQualityLevelsLoadable = bCouldLoadMore ? 1 : 0;
      res.m_State = ezResourceState::Loaded;

      return res;
    }
  }
}

void ezTexture3DResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(ezTexture3DResource);
  out_NewMemoryUsage.m_uiMemoryGPU = m_uiMemoryGPU[0] + m_uiMemoryGPU[1];
}

EZ_RESOURCE_IMPLEMENT_CREATEABLE(ezTexture3DResource, ezTexture3DResourceDescriptor)
{
  ezResourceLoadDesc ret;
  ret.m_uiQualityLevelsDiscardable = descriptor.m_uiQualityLevelsDiscardable;
  ret.m_uiQualityLevelsLoadable = descriptor.m_uiQualityLevelsLoadable;
  ret.m_State = ezResourceState::Loaded;

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  m_Type = descriptor.m_DescGAL.m_Type;
  m_Format = descriptor.m_DescGAL.m_Format;
  m_uiWidth = descriptor.m_DescGAL.m_uiWidth;
  m_uiHeight = descriptor.m_DescGAL.m_uiHeight;
  m_uiDepth = descriptor.m_DescGAL.m_uiDepth;

  m_hGALTexture[m_uiLoadedTextures] = pDevice->CreateTexture(descriptor.m_DescGAL, descriptor.m_InitialContent);
  EZ_ASSERT_DEV(!m_hGALTexture[m_uiLoadedTextures].IsInvalidated(), "Texture Data could not be uploaded to the GPU");

  pDevice->GetTexture(m_hGALTexture[m_uiLoadedTextures])->SetDebugName(GetResourceDescription());

  if (!m_hSamplerState.IsInvalidated())
  {
    pDevice->DestroySamplerState(m_hSamplerState);
  }

  m_hSamplerState = pDevice->CreateSamplerState(descriptor.m_SamplerDesc);
  EZ_ASSERT_DEV(!m_hSamplerState.IsInvalidated(), "Sampler state error");

  ++m_uiLoadedTextures;

  return ret;
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Textures_Texture3DResource);
