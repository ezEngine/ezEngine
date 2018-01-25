#include <PCH.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererFoundation/Resources/Texture.h>
#include <Foundation/Image/Formats/DdsFileFormat.h>
#include <Core/Assets/AssetFileHeader.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Textures/TextureUtils.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTexture2DResource, 1, ezRTTIDefaultAllocator<ezTexture2DResource>);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezTexture2DResource::ezTexture2DResource() : ezResource<ezTexture2DResource, ezTexture2DResourceDescriptor>(DoUpdate::OnAnyThread, ezTextureUtils::s_bForceFullQualityAlways ? 1 : 2)
{
  m_uiLoadedTextures = 0;
  m_uiMemoryGPU[0] = 0;
  m_uiMemoryGPU[1] = 0;
  m_Format = ezGALResourceFormat::Invalid;
  m_uiWidth = 0;
  m_uiHeight = 0;
  m_Type = ezGALTextureType::Invalid;
}

ezResourceLoadDesc ezTexture2DResource::UnloadData(Unload WhatToUnload)
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

void ezTexture2DResource::FillOutDescriptor(ezTexture2DResourceDescriptor& td, const ezImage* pImage, bool bSRGB, ezUInt32 uiNumMipLevels, ezUInt32& out_MemoryUsed, ezHybridArray<ezGALSystemMemoryDescription, 32>& initData)
{
  const ezUInt32 uiHighestMipLevel = pImage->GetNumMipLevels() - uiNumMipLevels;

  const ezGALResourceFormat::Enum format = ezTextureUtils::ImageFormatToGalFormat(pImage->GetImageFormat(), bSRGB);

  td.m_DescGAL.m_Format = format;
  td.m_DescGAL.m_uiWidth = pImage->GetWidth(uiHighestMipLevel);
  td.m_DescGAL.m_uiHeight = pImage->GetHeight(uiHighestMipLevel);
  td.m_DescGAL.m_uiDepth = pImage->GetDepth(uiHighestMipLevel);
  td.m_DescGAL.m_uiMipLevelCount = uiNumMipLevels;
  td.m_DescGAL.m_uiArraySize = pImage->GetNumArrayIndices();

  if (td.m_DescGAL.m_uiDepth > 1)
    td.m_DescGAL.m_Type = ezGALTextureType::Texture3D;

  if (pImage->GetNumFaces() == 6)
    td.m_DescGAL.m_Type = ezGALTextureType::TextureCube;

  EZ_ASSERT_DEV(pImage->GetNumFaces() == 1 || pImage->GetNumFaces() == 6, "Invalid number of image faces");

  out_MemoryUsed = 0;

  initData.Clear();

  for (ezUInt32 array_index = 0; array_index < pImage->GetNumArrayIndices(); ++array_index)
  {
    for (ezUInt32 face = 0; face < pImage->GetNumFaces(); ++face)
    {
      for (ezUInt32 mip = uiHighestMipLevel; mip < pImage->GetNumMipLevels(); ++mip)
      {
        ezGALSystemMemoryDescription& id = initData.ExpandAndGetRef();

        id.m_pData = const_cast<ezUInt8*>(pImage->GetDataPointer<ezUInt8>() + pImage->GetDataOffSet(mip, face, array_index));

        if (ezImageFormat::GetType(pImage->GetImageFormat()) == ezImageFormatType::BLOCK_COMPRESSED)
        {
          const ezUInt32 uiMemPitchFactor = ezGALResourceFormat::GetBitsPerElement(format) * 4 / 8;

          id.m_uiRowPitch = ezMath::Max<ezUInt32>(4, pImage->GetWidth(mip)) * uiMemPitchFactor;
        }
        else
        {
          id.m_uiRowPitch = pImage->GetRowPitch(mip);
        }

        id.m_uiSlicePitch = pImage->GetDepthPitch(mip);

        out_MemoryUsed += id.m_uiSlicePitch;
      }
    }
  }

  const ezArrayPtr<ezGALSystemMemoryDescription> InitDataPtr(initData);

  td.m_InitialContent = InitDataPtr;
}

ezResourceLoadDesc ezTexture2DResource::UpdateContent(ezStreamReader* Stream)
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


  ezTexture2DResourceDescriptor td;
  td.m_SamplerDesc.m_AddressU = addressModeU;
  td.m_SamplerDesc.m_AddressV = addressModeV;
  td.m_SamplerDesc.m_AddressW = addressModeW;

  const ezUInt32 uiNumMipmapsLowRes = ezTextureUtils::s_bForceFullQualityAlways ? pImage->GetNumMipLevels() : ezMath::Min(pImage->GetNumMipLevels(), 6U);
  const ezUInt32 uiNumMipLevels = ezMath::Min(m_uiLoadedTextures == 0 ? uiNumMipmapsLowRes : pImage->GetNumMipLevels(), pImage->GetNumMipLevels());
    
  ezHybridArray<ezGALSystemMemoryDescription, 32> initData;
  FillOutDescriptor(td, pImage, bSRGB, uiNumMipLevels, m_uiMemoryGPU[m_uiLoadedTextures], initData);

  ezTextureUtils::ConfigureSampler(textureFilter, td.m_SamplerDesc);

  // ignore its return value here, we build our own
  CreateResource(td);

  {
    ezResourceLoadDesc res;
    res.m_uiQualityLevelsDiscardable = m_uiLoadedTextures;

    const ezUInt32 uiHighestMipLevel = pImage->GetNumMipLevels() - uiNumMipLevels;
    if (uiHighestMipLevel == 0 && !bIsFallback)
      res.m_uiQualityLevelsLoadable = 0;
    else
      res.m_uiQualityLevelsLoadable = 1;

    res.m_State = ezResourceState::Loaded;

    return res;
  }
}

void ezTexture2DResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(ezTexture2DResource);
  out_NewMemoryUsage.m_uiMemoryGPU = m_uiMemoryGPU[0] + m_uiMemoryGPU[1];
}

ezResourceLoadDesc ezTexture2DResource::CreateResource(const ezTexture2DResourceDescriptor& descriptor)
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

EZ_STATICLINK_FILE(RendererCore, RendererCore_Textures_Texture2DResource);

