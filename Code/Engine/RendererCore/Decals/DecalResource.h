#pragma once

#include <RendererCore/RendererCoreDLL.h>
#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>

typedef ezTypedResourceHandle<class ezDecalResource> ezDecalResourceHandle;

struct ezDecalResourceDescriptor
{
};

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

class EZ_RENDERERCORE_DLL ezDecalResourceLoader : public ezResourceTypeLoader
{
public:

  struct LoadedData
  {
    LoadedData() : m_Reader(&m_Storage) { }

    ezMemoryStreamStorage m_Storage;
    ezMemoryStreamReader m_Reader;
  };

  virtual ezResourceLoadData OpenDataStream(const ezResource* pResource) override;
  virtual void CloseDataStream(const ezResource* pResource, const ezResourceLoadData& LoaderData) override;
  virtual bool IsResourceOutdated(const ezResource* pResource) const override;
};

