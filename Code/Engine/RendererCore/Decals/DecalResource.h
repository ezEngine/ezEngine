#pragma once

#include <RendererCore/Basics.h>
#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>

typedef ezTypedResourceHandle<class ezDecalResource> ezDecalResourceHandle;

struct ezDecalResourceDescriptor
{
};

class EZ_RENDERERCORE_DLL ezDecalResource : public ezResource<ezDecalResource, ezDecalResourceDescriptor>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDecalResource, ezResourceBase);

public:
  ezDecalResource();

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;
  virtual ezResourceLoadDesc CreateResource(const ezDecalResourceDescriptor& descriptor) override;

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

  virtual ezResourceLoadData OpenDataStream(const ezResourceBase* pResource) override;
  virtual void CloseDataStream(const ezResourceBase* pResource, const ezResourceLoadData& LoaderData) override;
  virtual bool IsResourceOutdated(const ezResourceBase* pResource) const override;
};

