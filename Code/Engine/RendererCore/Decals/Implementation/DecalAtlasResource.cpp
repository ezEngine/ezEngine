#include <RendererCore/RendererCorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Math/Rect.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <RendererCore/Decals/DecalAtlasResource.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Textures/TextureUtils.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/Image.h>

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
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDecalAtlasResource, 1, ezRTTIDefaultAllocator<ezDecalAtlasResource>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_RESOURCE_IMPLEMENT_COMMON_CODE(ezDecalAtlasResource);
// clang-format on

ezUInt32 ezDecalAtlasResource::s_uiDecalAtlasResources = 0;

ezDecalAtlasResource::ezDecalAtlasResource()
  : ezResource(DoUpdate::OnAnyThread, 1)
  , m_vBaseColorSize(ezVec2U32::MakeZero())
  , m_vNormalSize(ezVec2U32::MakeZero())
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
  EZ_LOG_BLOCK("ezDecalAtlasResource::UpdateContent", GetResourceIdOrDescription());

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::LoadedResourceMissing;

  if (Stream == nullptr)
    return res;

  // the standard file reader writes the absolute file path into the stream
  ezStringBuilder sAbsFilePath;
  (*Stream) >> sAbsFilePath;

  // skip the asset header
  {
    ezAssetFileHeader header;
    header.Read(*Stream).IgnoreResult();
  }

  {
    ezUInt8 uiVersion = 0;
    *Stream >> uiVersion;
    EZ_ASSERT_DEV(uiVersion <= 3, "Invalid decal atlas version {0}", uiVersion);

    // this version is now incompatible
    if (uiVersion < 3)
      return res;
  }

  // read the textures
  {
    ezDdsFileFormat dds;
    ezImage baseColor, normal, orm;

    if (dds.ReadImage(*Stream, baseColor, "dds").Failed())
    {
      ezLog::Error("Failed to load baseColor image for decal atlas");
      return res;
    }

    if (dds.ReadImage(*Stream, normal, "dds").Failed())
    {
      ezLog::Error("Failed to load normal image for decal atlas");
      return res;
    }

    if (dds.ReadImage(*Stream, orm, "dds").Failed())
    {
      ezLog::Error("Failed to load normal image for decal atlas");
      return res;
    }

    CreateLayerTexture(baseColor, true, m_hBaseColor);
    CreateLayerTexture(normal, false, m_hNormal);
    CreateLayerTexture(orm, false, m_hORM);

    m_vBaseColorSize = ezVec2U32(baseColor.GetWidth(), baseColor.GetHeight());
    m_vNormalSize = ezVec2U32(normal.GetWidth(), normal.GetHeight());
    m_vORMSize = ezVec2U32(orm.GetWidth(), orm.GetHeight());
  }

  ReadDecalInfo(Stream);

  res.m_State = ezResourceState::Loaded;
  return res;
}

void ezDecalAtlasResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(ezDecalAtlasResource) + (ezUInt32)m_Atlas.m_Items.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

EZ_RESOURCE_IMPLEMENT_CREATEABLE(ezDecalAtlasResource, ezDecalAtlasResourceDescriptor)
{
  ezResourceLoadDesc ret;
  ret.m_uiQualityLevelsDiscardable = 0;
  ret.m_uiQualityLevelsLoadable = 0;
  ret.m_State = ezResourceState::Loaded;

  m_Atlas.Clear();

  return ret;
}

void ezDecalAtlasResource::CreateLayerTexture(const ezImage& img, bool bSRGB, ezTexture2DResourceHandle& out_hTexture)
{
  ezTexture2DResourceDescriptor td;
  td.m_SamplerDesc.m_AddressU = ezImageAddressMode::Clamp;
  td.m_SamplerDesc.m_AddressV = ezImageAddressMode::Clamp;
  td.m_SamplerDesc.m_AddressW = ezImageAddressMode::Clamp;

  ezUInt32 uiMemory;
  ezHybridArray<ezGALSystemMemoryDescription, 32> initData;
  ezTexture2DResource::FillOutDescriptor(td, &img, bSRGB, img.GetNumMipLevels(), uiMemory, initData);
  ezTextureUtils::ConfigureSampler(ezTextureFilterSetting::HighQuality, td.m_SamplerDesc);

  ezStringBuilder sTexId;
  sTexId.SetFormat("{0}_Tex{1}", GetResourceID(), s_uiDecalAtlasResources);
  ++s_uiDecalAtlasResources;

  out_hTexture = ezResourceManager::CreateResource<ezTexture2DResource>(sTexId, std::move(td));
}

void ezDecalAtlasResource::ReadDecalInfo(ezStreamReader* Stream)
{
  m_Atlas.Deserialize(*Stream).IgnoreResult();
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
