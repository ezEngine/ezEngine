#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <RendererCore/RendererCoreDLL.h>

using ezDecalResourceHandle = ezTypedResourceHandle<class ezDecalResource>;

/// Descriptor for creating a decal resource.
///
/// Currently empty as decals are typically loaded from asset files.
struct ezDecalResourceDescriptor
{
};

/// Resource representing a single decal that references regions in a decal atlas.
///
/// Decal resources are lightweight and reference textures stored in ezDecalAtlasResource.
/// The actual texture data and UV mapping is managed by the atlas.
class EZ_RENDERERCORE_DLL ezDecalResource : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDecalResource, ezResource);
  EZ_RESOURCE_DECLARE_COMMON_CODE(ezDecalResource);
  EZ_RESOURCE_DECLARE_CREATEABLE(ezDecalResource, ezDecalResourceDescriptor);

public:
  ezDecalResource();

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;
};

/// Resource loader for decal resources.
///
/// Loads decal metadata from asset files. The actual texture data is loaded separately
/// through the decal atlas system.
class EZ_RENDERERCORE_DLL ezDecalResourceLoader : public ezResourceTypeLoader
{
public:
  struct LoadedData
  {
    LoadedData()
      : m_Reader(&m_Storage)
    {
    }

    ezContiguousMemoryStreamStorage m_Storage;
    ezMemoryStreamReader m_Reader;
  };

  virtual ezResourceLoadData OpenDataStream(const ezResource* pResource) override;
  virtual void CloseDataStream(const ezResource* pResource, const ezResourceLoadData& loaderData) override;
  virtual bool IsResourceOutdated(const ezResource* pResource) const override;
};
