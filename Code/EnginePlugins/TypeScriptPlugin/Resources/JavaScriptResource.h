#pragma once

#include <TypeScriptPlugin/TypeScriptPluginDLL.h>

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/DynamicArray.h>

class ezStreamWriter;
class ezStreamReader;

struct EZ_TYPESCRIPTPLUGIN_DLL ezJavaScriptResourceDesc
{
  ezString m_sComponentName;

  ezResult Serialize(ezStreamWriter& stream) const;
  ezResult Deserialize(ezStreamReader& stream);
};

class EZ_TYPESCRIPTPLUGIN_DLL ezJavaScriptResource : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezJavaScriptResource, ezResource);
  EZ_RESOURCE_DECLARE_COMMON_CODE(ezJavaScriptResource);

public:
  ezJavaScriptResource();
  ~ezJavaScriptResource();

  const ezJavaScriptResourceDesc& GetDescriptor() const { return m_Desc; }

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* pStream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  ezJavaScriptResourceDesc m_Desc;
};

using ezJavaScriptResourceHandle = ezTypedResourceHandle<class ezJavaScriptResource>;
