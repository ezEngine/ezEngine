#pragma once

#include <AiPlugin/AiPluginDLL.h>
#include <AiPlugin/UtilityAI/Framework/AiControllerDesc.h>
#include <Core/ResourceManager/Resource.h>

using ezAiControllerResourceHandle = ezTypedResourceHandle<class ezAiControllerResource>;

/// Holds a finalized ezAiControllerDesc, loaded from an AI controller asset (see ezAiControllerDesc::Deserialize()).
///
/// Agents keep a reference to the desc, so a reloaded resource only affects agents that start afterwards.
class EZ_AIPLUGIN_DLL ezAiControllerResource : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAiControllerResource, ezResource);
  EZ_RESOURCE_DECLARE_COMMON_CODE(ezAiControllerResource);

public:
  ezAiControllerResource();
  ~ezAiControllerResource();

  /// Null, if the resource could not be loaded.
  const ezSharedPtr<ezAiControllerDesc>& GetDesc() const { return m_pDesc; }

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* pStream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  ezSharedPtr<ezAiControllerDesc> m_pDesc;
};
