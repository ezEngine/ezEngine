#include <PCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Image/Formats/DdsFileFormat.h>
#include <Foundation/Image/Image.h>
#include <Foundation/Math/Rect.h>
#include <RendererCore/Decals/DecalAtlasResource.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Textures/TextureUtils.h>

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, DecalAtlasResource)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "Foundation",
  "Core",
  "TextureResource"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    ezDecalAtlasResourceDescriptor desc;
    ezDecalAtlasResourceHandle hFallback = ezResourceManager::CreateResource<ezDecalAtlasResource>("Fallback Decal Atlas", std::move(desc), "Empty Decal Atlas for loading and missing decals");

    ezResourceManager::SetResourceTypeLoadingFallback<ezDecalAtlasResource>(hFallback);
    ezResourceManager::SetResourceTypeMissingFallback<ezDecalAtlasResource>(hFallback);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    ezResourceManager::SetResourceTypeLoadingFallback<ezDecalAtlasResource>(ezDecalAtlasResourceHandle());
  ezResourceManager::SetResourceTypeMissingFallback<ezDecalAtlasResource>(ezDecalAtlasResourceHandle());
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on


//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDecalAtlasResource, 1, ezRTTIDefaultAllocator<ezDecalAtlasResource>);
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_RESOURCE_IMPLEMENT_COMMON_CODE(ezDecalAtlasResource);
// clang-format on

ezUInt32 ezDecalAtlasResource::s_uiDecalAtlasResources = 0;

ezDecalAtlasResource::ezDecalAtlasResource()
    : ezResource(DoUpdate::OnAnyThread, 1)
    , m_BaseColorSize(ezVec2U32::ZeroVector())
    , m_NormalSize(ezVec2U32::ZeroVector())
{
}


ezDecalAtlasResourceHandle ezDecalAtlasResource::GetDecalAtlasResource()
{
  return ezResourceManager::LoadResource<ezDecalAtlasResource>("{ ProjectDecalAtlas }");
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
    ezImage baseColor, normal;

    if (dds.ReadImage(*Stream, baseColor, ezLog::GetThreadLocalLogSystem(), "dds").Failed())
    {
      ezLog::Error("Failed to load baseColor image for decal atlas");
      return res;
    }

    if (dds.ReadImage(*Stream, normal, ezLog::GetThreadLocalLogSystem(), "dds").Failed())
    {
      ezLog::Error("Failed to load normal image for decal atlas");
      return res;
    }

    CreateLayerTexture(baseColor, true, m_hBaseColor);
    CreateLayerTexture(normal, false, m_hNormal);

    m_BaseColorSize = ezVec2U32(baseColor.GetWidth(), baseColor.GetHeight());
    m_NormalSize = ezVec2U32(normal.GetWidth(), normal.GetHeight());
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

EZ_RESOURCE_IMPLEMENT_CREATEABLE(ezDecalAtlasResource, ezDecalAtlasResourceDescriptor)
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

  out_hTexture = ezResourceManager::CreateResource<ezTexture2DResource>(sTexId, std::move(td));
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

    *Stream >> decal.m_BaseColorRect.x;
    *Stream >> decal.m_BaseColorRect.y;
    *Stream >> decal.m_BaseColorRect.width;
    *Stream >> decal.m_BaseColorRect.height;

    *Stream >> decal.m_NormalRect.x;
    *Stream >> decal.m_NormalRect.y;
    *Stream >> decal.m_NormalRect.width;
    *Stream >> decal.m_NormalRect.height;
  }
}

void ezDecalAtlasResource::ReportResourceIsMissing()
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  // normal during development, don't care much
  ezLog::Debug("Decal Atlas Resource is missing: '{0}' ('{1}')", GetResourceID(), GetResourceDescription());
#else
  // should probably exist for shipped applications, report this
  ezLog::Warning("Decal Atlas Resource is missing: '{0}' ('{1}')", GetResourceID(), GetResourceDescription());
#endif
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Decals_Implementation_DecalAtlasResource);
