#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/IO/DependencyFile.h>
#include <RmlUiPlugin/RmlUiPluginDLL.h>

struct EZ_RMLUIPLUGIN_DLL ezRmlUiScaleMode
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Fixed,
    WithScreenSize,

    Default = Fixed
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_RMLUIPLUGIN_DLL, ezRmlUiScaleMode);

struct EZ_RMLUIPLUGIN_DLL ezRmlUiResourceDescriptor
{
  ezResult Save(ezStreamWriter& stream);
  ezResult Load(ezStreamReader& stream);

  ezDependencyFile m_DependencyFile;

  ezString m_sRmlFile;
  ezEnum<ezRmlUiScaleMode> m_ScaleMode;
  ezVec2U32 m_ReferenceResolution;
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
  const ezEnum<ezRmlUiScaleMode>& GetScaleMode() const { return m_ScaleMode; }
  const ezVec2U32& GetReferenceResolution() const { return m_vReferenceResolution; }

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  ezString m_sRmlFile;
  ezEnum<ezRmlUiScaleMode> m_ScaleMode;
  ezVec2U32 m_vReferenceResolution = ezVec2U32::ZeroVector();
};

class ezRmlUiResourceLoader : public ezResourceLoaderFromFile
{
public:
  virtual bool IsResourceOutdated(const ezResource* pResource) const override;
};
