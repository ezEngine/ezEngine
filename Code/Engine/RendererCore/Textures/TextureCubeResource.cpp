#include <PCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/Image/Formats/DdsFileFormat.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Textures/TextureCubeResource.h>
#include <RendererCore/Textures/TextureUtils.h>
#include <RendererFoundation/Resources/Texture.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTextureCubeResource, 1, ezRTTIDefaultAllocator<ezTextureCubeResource>);
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezTextureCubeResource::ezTextureCubeResource()
    : ezResource<ezTextureCubeResource, ezTextureCubeResourceDescriptor>(DoUpdate::OnAnyThread,
                                                                         ezTextureUtils::s_bForceFullQualityAlways ? 1 : 2)
{
  m_uiLoadedTextures = 0;
  m_uiMemoryGPU[0] = 0;
  m_uiMemoryGPU[1] = 0;
  m_Format = ezGALResourceFormat::Invalid;
  m_uiWidthAndHeight = 0;
}

ezResourceLoadDesc ezTextureCubeResource::UnloadData(Unload WhatToUnload)
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

ezResourceLoadDesc ezTextureCubeResource::UpdateContent(ezStreamReader* Stream)
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

  bool bIsFallback = false;
  *Stream >> bIsFallback;

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

  const ezUInt32 uiNumMipmapsLowRes = ezTextureUtils::s_bForceFullQualityAlways ? pImage->GetNumMipLevels() : 6;

  const ezUInt32 uiNumMipLevels =
      ezMath::Min(m_uiLoadedTextures == 0 ? uiNumMipmapsLowRes : pImage->GetNumMipLevels(), pImage->GetNumMipLevels());
  const ezUInt32 uiHighestMipLevel = pImage->GetNumMipLevels() - uiNumMipLevels;

  if (pImage->GetWidth(uiHighestMipLevel) != pImage->GetHeight(uiHighestMipLevel))
  {
    ezLog::Error("Cubemap width '{0}' is not identical to height '{1}'", pImage->GetWidth(uiHighestMipLevel),
                 pImage->GetHeight(uiHighestMipLevel));

    ezResourceLoadDesc res;
    res.m_uiQualityLevelsDiscardable = 0;
    res.m_uiQualityLevelsLoadable = 0;
    res.m_State = ezResourceState::LoadedResourceMissing;

    return res;
  }

  m_Format = ezTextureUtils::ImageFormatToGalFormat(pImage->GetImageFormat(), bSRGB);
  m_uiWidthAndHeight = pImage->GetWidth(uiHighestMipLevel);

  ezGALTextureCreationDescription texDesc;
  texDesc.m_Format = m_Format;
  texDesc.m_uiWidth = m_uiWidthAndHeight;
  texDesc.m_uiHeight = m_uiWidthAndHeight;
  texDesc.m_uiDepth = pImage->GetDepth(uiHighestMipLevel);
  texDesc.m_uiMipLevelCount = uiNumMipLevels;
  texDesc.m_uiArraySize = pImage->GetNumArrayIndices();

  if (texDesc.m_uiDepth > 1)
    texDesc.m_Type = ezGALTextureType::Texture3D;

  if (pImage->GetNumFaces() == 6)
    texDesc.m_Type = ezGALTextureType::TextureCube;

  EZ_ASSERT_DEV(pImage->GetNumFaces() == 1 || pImage->GetNumFaces() == 6, "Invalid number of image faces (resource: '{0}')",
                GetResourceID());

  m_uiMemoryGPU[m_uiLoadedTextures] = 0;

  ezHybridArray<ezGALSystemMemoryDescription, 32> InitData;

  for (ezUInt32 array_index = 0; array_index < pImage->GetNumArrayIndices(); ++array_index)
  {
    for (ezUInt32 face = 0; face < pImage->GetNumFaces(); ++face)
    {
      for (ezUInt32 mip = uiHighestMipLevel; mip < pImage->GetNumMipLevels(); ++mip)
      {
        ezGALSystemMemoryDescription& id = InitData.ExpandAndGetRef();

        id.m_pData = pImage->GetPixelPointer<ezUInt8>(mip, face, array_index);

        if (ezImageFormat::GetType(pImage->GetImageFormat()) == ezImageFormatType::BLOCK_COMPRESSED)
        {
          const ezUInt32 uiMemPitchFactor = ezGALResourceFormat::GetBitsPerElement(m_Format) * 4 / 8;

          id.m_uiRowPitch = ezMath::Max<ezUInt32>(4, pImage->GetWidth(mip)) * uiMemPitchFactor;
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

  ezTextureCubeResourceDescriptor td;
  td.m_DescGAL = texDesc;
  td.m_SamplerDesc.m_AddressU = addressModeU;
  td.m_SamplerDesc.m_AddressV = addressModeV;
  td.m_SamplerDesc.m_AddressW = addressModeW;
  td.m_InitialContent = InitDataPtr;

  ezTextureUtils::ConfigureSampler(textureFilter, td.m_SamplerDesc);

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

void ezTextureCubeResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(ezTextureCubeResource);
  out_NewMemoryUsage.m_uiMemoryGPU = m_uiMemoryGPU[0] + m_uiMemoryGPU[1];
}

ezResourceLoadDesc ezTextureCubeResource::CreateResource(const ezTextureCubeResourceDescriptor& descriptor)
{
  ezResourceLoadDesc ret;
  ret.m_uiQualityLevelsDiscardable = descriptor.m_uiQualityLevelsDiscardable;
  ret.m_uiQualityLevelsLoadable = descriptor.m_uiQualityLevelsLoadable;
  ret.m_State = ezResourceState::Loaded;

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  EZ_ASSERT_DEV(descriptor.m_DescGAL.m_uiWidth == descriptor.m_DescGAL.m_uiHeight, "Cubemap width and height must be identical");

  m_Format = descriptor.m_DescGAL.m_Format;
  m_uiWidthAndHeight = descriptor.m_DescGAL.m_uiWidth;

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



EZ_STATICLINK_FILE(RendererCore, RendererCore_Textures_TextureCubeResource);
