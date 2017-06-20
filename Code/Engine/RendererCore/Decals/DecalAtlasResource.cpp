#include <PCH.h>
#include <RendererCore/Decals/DecalAtlasResource.h>
#include <Foundation/Configuration/Startup.h>
#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/Math/Rect.h>
#include <Foundation/Image/Formats/DdsFileFormat.h>
#include <Foundation/Image/Image.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Textures/TextureUtils.h>
#include <Core/ResourceManager/ResourceManager.h>

EZ_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, DecalAtlasResource)

BEGIN_SUBSYSTEM_DEPENDENCIES
"Foundation",
"Core",
"TextureResource"
END_SUBSYSTEM_DEPENDENCIES

ON_CORE_STARTUP
{
  ezDecalAtlasResourceDescriptor desc;
  ezDecalAtlasResourceHandle hFallback = ezResourceManager::CreateResource<ezDecalAtlasResource>("Fallback Decal Atlas", desc, "Empty Decal Atlas for loading and missing decals");

  ezDecalAtlasResource::SetTypeFallbackResource(hFallback);
  ezDecalAtlasResource::SetTypeMissingResource(hFallback);
}

ON_CORE_SHUTDOWN
{
}

ON_ENGINE_STARTUP
{
}

ON_ENGINE_SHUTDOWN
{
}

EZ_END_SUBSYSTEM_DECLARATION


//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDecalAtlasResource, 1, ezRTTIDefaultAllocator<ezDecalAtlasResource>);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezUInt32 ezDecalAtlasResource::s_uiDecalAtlasResources = 0;

ezDecalAtlasResource::ezDecalAtlasResource() : ezResource<ezDecalAtlasResource, ezDecalAtlasResourceDescriptor>(DoUpdate::OnAnyThread, 1)
{
}

ezResourceLoadDesc ezDecalAtlasResource::UnloadData(Unload WhatToUnload)
{
  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Unloaded;

  return res;
}

ezResourceLoadDesc ezDecalAtlasResource::UpdateContent(ezStreamReader* Stream)
{
EZ_LOG_BLOCK("ezDecalAtlasResource::UpdateContent", GetResourceDescription().GetData());

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::LoadedResourceMissing;

  if (Stream == nullptr)
    return res;

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    ezString sAbsFilePath;
    (*Stream) >> sAbsFilePath;
  }

  // skip the asset header
  {
    ezAssetFileHeader header;
    header.Read(*Stream);
  }

  {
    ezUInt8 uiVersion = 0;
    *Stream >> uiVersion;
    EZ_ASSERT_DEV(uiVersion == 1, "Invalid decal atlas version {0}", uiVersion);
  }

  // read the textures
  {
    ezDdsFileFormat dds;
    ezImage diffuse, normal;

    if (dds.ReadImage(*Stream, diffuse, ezLog::GetThreadLocalLogSystem()).Failed())
    {
      ezLog::Error("Failed to load diffuse image for decal atlas");
      return res;
    }

    if (dds.ReadImage(*Stream, normal, ezLog::GetThreadLocalLogSystem()).Failed())
    {
      ezLog::Error("Failed to load diffuse image for decal atlas");
      return res;
    }

    CreateLayerTexture(diffuse, true, m_hDiffuse);
    CreateLayerTexture(normal, false, m_hNormal);
  }

  ReadDecalInfo(Stream);

  res.m_State = ezResourceState::Loaded;
  return res;
}

void ezDecalAtlasResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(ezDecalAtlasResource) + (ezUInt32)m_Decals.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

ezResourceLoadDesc ezDecalAtlasResource::CreateResource(const ezDecalAtlasResourceDescriptor& descriptor)
{
  ezResourceLoadDesc ret;
  ret.m_uiQualityLevelsDiscardable = 0;
  ret.m_uiQualityLevelsLoadable = 0;
  ret.m_State = ezResourceState::Loaded;

  m_Decals.Clear();

  return ret;
}

void ezDecalAtlasResource::CreateLayerTexture(const ezImage& img, bool bSRGB, ezTexture2DResourceHandle& out_hTexture)
{
  ezTexture2DResourceDescriptor td;
  td.m_SamplerDesc.m_AddressU = ezGALTextureAddressMode::Clamp;
  td.m_SamplerDesc.m_AddressV = ezGALTextureAddressMode::Clamp;
  td.m_SamplerDesc.m_AddressW = ezGALTextureAddressMode::Clamp;

  ezUInt32 uiMemory;
  ezHybridArray<ezGALSystemMemoryDescription, 32> initData;
  ezTexture2DResource::FillOutDescriptor(td, &img, bSRGB, img.GetNumMipLevels(), uiMemory, initData);
  ezTextureUtils::ConfigureSampler(ezTextureFilterSetting::HighQuality, td.m_SamplerDesc);

  ezStringBuilder sTexId;
  sTexId.Format("{0}_Tex{1}", GetResourceID(), s_uiDecalAtlasResources);
  ++s_uiDecalAtlasResources;

  out_hTexture = ezResourceManager::CreateResource<ezTexture2DResource>(sTexId, td);
}

void ezDecalAtlasResource::ReadDecalInfo(ezStreamReader* Stream)
{
  m_Decals.Clear();

  ezUInt32 uiNumDecals = 0;
  *Stream >> uiNumDecals;

  ezString sIdentifier;
  ezUInt32 uiHash;

  for (ezUInt32 i = 0; i < uiNumDecals; ++i)
  {
    *Stream >> sIdentifier;
    *Stream >> uiHash;

    auto& decal = m_Decals[uiHash];
    decal.m_sIdentifier = sIdentifier;

    *Stream >> decal.m_Rect.x;
    *Stream >> decal.m_Rect.y;
    *Stream >> decal.m_Rect.width;
    *Stream >> decal.m_Rect.height;
  }
}

