#include <RendererCorePCH.h>

#include <Foundation/Configuration/Startup.h>
#include <RendererCore/Decals/DecalResource.h>

static ezDecalResourceLoader s_DecalResourceLoader;

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, DecalResource)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "Foundation",
  "Core",
  "TextureResource"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    ezResourceManager::SetResourceTypeLoader<ezDecalResource>(&s_DecalResourceLoader);

    ezDecalResourceDescriptor desc;
    ezDecalResourceHandle hFallback = ezResourceManager::CreateResource<ezDecalResource>("Fallback Decal", std::move(desc), "Empty Decal for loading and missing decals");

    ezResourceManager::SetResourceTypeLoadingFallback<ezDecalResource>(hFallback);
    ezResourceManager::SetResourceTypeMissingFallback<ezDecalResource>(hFallback);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    ezResourceManager::SetResourceTypeLoader<ezDecalResource>(nullptr);

    ezResourceManager::SetResourceTypeLoadingFallback<ezDecalResource>(ezDecalResourceHandle());
    ezResourceManager::SetResourceTypeMissingFallback<ezDecalResource>(ezDecalResourceHandle());
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
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDecalResource, 1, ezRTTIDefaultAllocator<ezDecalResource>);
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_RESOURCE_IMPLEMENT_COMMON_CODE(ezDecalResource);
// clang-format on

ezDecalResource::ezDecalResource()
    : ezResource(DoUpdate::OnAnyThread, 1)
{
}

ezResourceLoadDesc ezDecalResource::UnloadData(Unload WhatToUnload)
{
  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Unloaded;

  return res;
}

ezResourceLoadDesc ezDecalResource::UpdateContent(ezStreamReader* Stream)
{
  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  return res;
}

void ezDecalResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(ezDecalResource);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

EZ_RESOURCE_IMPLEMENT_CREATEABLE(ezDecalResource, ezDecalResourceDescriptor)
{
  ezResourceLoadDesc ret;
  ret.m_uiQualityLevelsDiscardable = 0;
  ret.m_uiQualityLevelsLoadable = 0;
  ret.m_State = ezResourceState::Loaded;

  return ret;
}

//////////////////////////////////////////////////////////////////////////

ezResourceLoadData ezDecalResourceLoader::OpenDataStream(const ezResource* pResource)
{
  // nothing to load, decals are solely identified by their id (name)
  // the rest of the information is in the decal atlas resource

  ezResourceLoadData res;
  return res;
}

void ezDecalResourceLoader::CloseDataStream(const ezResource* pResource, const ezResourceLoadData& LoaderData)
{
  // nothing to do
}

bool ezDecalResourceLoader::IsResourceOutdated(const ezResource* pResource) const
{
  // decals are never outdated
  return false;
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Decals_Implementation_DecalResource);

