#include <PCH.h>
#include <RendererCore/Decals/DecalResource.h>
#include <Foundation/Configuration/Startup.h>

static ezDecalResourceLoader s_DecalResourceLoader;

EZ_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, DecalResource)

BEGIN_SUBSYSTEM_DEPENDENCIES
"Foundation",
"Core",
"TextureResource"
END_SUBSYSTEM_DEPENDENCIES

ON_CORE_STARTUP
{
  ezResourceManager::SetResourceTypeLoader<ezDecalResource>(&s_DecalResourceLoader);

  ezDecalResourceDescriptor desc;
  ezDecalResourceHandle hFallback = ezResourceManager::CreateResource<ezDecalResource>("Fallback Decal", desc, "Empty Decal for loading and missing decals");

  ezDecalResource::SetTypeFallbackResource(hFallback);
  ezDecalResource::SetTypeMissingResource(hFallback);
}

ON_CORE_SHUTDOWN
{
  ezResourceManager::SetResourceTypeLoader<ezDecalResource>(nullptr);

  ezDecalResource::SetTypeFallbackResource(ezDecalResourceHandle());
  ezDecalResource::SetTypeMissingResource(ezDecalResourceHandle());
}

ON_ENGINE_STARTUP
{
}

ON_ENGINE_SHUTDOWN
{
}

EZ_END_SUBSYSTEM_DECLARATION


//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDecalResource, 1, ezRTTIDefaultAllocator<ezDecalResource>);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezDecalResource::ezDecalResource() : ezResource<ezDecalResource, ezDecalResourceDescriptor>(DoUpdate::OnAnyThread, 1)
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

ezResourceLoadDesc ezDecalResource::CreateResource(const ezDecalResourceDescriptor& descriptor)
{
  ezResourceLoadDesc ret;
  ret.m_uiQualityLevelsDiscardable = 0;
  ret.m_uiQualityLevelsLoadable = 0;
  ret.m_State = ezResourceState::Loaded;

  return ret;
}

//////////////////////////////////////////////////////////////////////////

ezResourceLoadData ezDecalResourceLoader::OpenDataStream(const ezResourceBase* pResource)
{
  // nothing to load, decals are solely identified by their id (name)
  // the rest of the information is in the decal atlas resource

  ezResourceLoadData res;
  return res;
}

void ezDecalResourceLoader::CloseDataStream(const ezResourceBase* pResource, const ezResourceLoadData& LoaderData)
{
  // nothing to do
}

bool ezDecalResourceLoader::IsResourceOutdated(const ezResourceBase* pResource) const
{
  // decals are never outdated
  return false;
}



