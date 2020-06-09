#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/IO/DependencyFile.h>
#include <RmlUiPlugin/RmlUiPluginDLL.h>

struct EZ_RMLUIPLUGIN_DLL ezRmlUiResourceDescriptor
{
  ezResult Save(ezStreamWriter& stream);
  ezResult Load(ezStreamReader& stream);

  ezDependencyFile m_DependencyFile;

  ezString m_sRmlFile;
};

using ezRmlUiResourceHandle = ezTypedResourceHandle<class ezRmlUiResource>;

class EZ_RMLUIPLUGIN_DLL ezRmlUiResource : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRmlUiResource, ezResource);
  EZ_RESOURCE_DECLARE_COMMON_CODE(ezRmlUiResource);
  EZ_RESOURCE_DECLARE_CREATEABLE(ezRmlUiResource, ezRmlUiResourceDescriptor);

public:
  ezRmlUiResource();

  const ezString& GetRmlFile() const { return m_sRmlFile; }

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  ezString m_sRmlFile;
};

class ezRmlUiResourceLoader : public ezResourceLoaderFromFile
{
public:
  virtual bool IsResourceOutdated(const ezResource* pResource) const override;
};
