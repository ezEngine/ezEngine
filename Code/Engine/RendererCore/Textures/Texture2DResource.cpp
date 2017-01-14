#include <RendererCore/PCH.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererFoundation/Resources/Texture.h>
#include <CoreUtils/Image/Formats/DdsFileFormat.h>
#include <CoreUtils/Assets/AssetFileHeader.h>
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

  const ezUInt32 uiNumMipLevels = ezMath::Min(m_uiLoadedTextures == 0 ? uiNumMipmapsLowRes : pImage->GetNumMipLevels(), pImage->GetNumMipLevels());
  const ezUInt32 uiHighestMipLevel = pImage->GetNumMipLevels() - uiNumMipLevels;

  m_Format = ezTextureUtils::ImageFormatToGalFormat(pImage->GetImageFormat(), bSRGB);
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

  EZ_ASSERT_DEV(pImage->GetNumFaces() == 1 || pImage->GetNumFaces() == 6, "Invalid number of image faces (resource: '{0}')", GetResourceID().GetData());

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

  ezTexture2DResourceDescriptor td;
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

EZ_STATICLINK_FILE(RendererCore, RendererCore_Textures_TextureResource);

