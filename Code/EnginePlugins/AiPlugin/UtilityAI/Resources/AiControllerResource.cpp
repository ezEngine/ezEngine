#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/UtilityAI/Resources/AiControllerResource.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Utilities/AssetFileHeader.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAiControllerResource, 1, ezRTTIDefaultAllocator<ezAiControllerResource>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_RESOURCE_IMPLEMENT_COMMON_CODE(ezAiControllerResource);

EZ_BEGIN_SUBSYSTEM_DECLARATION(AiPlugin, AiControllerResource)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ResourceManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    ezResourceManager::RegisterResourceForAssetType("AI Controller", ezGetStaticRTTI<ezAiControllerResource>());
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

ezAiControllerResource::ezAiControllerResource()
  : ezResource(DoUpdate::OnAnyThread, 1)
{
}

ezAiControllerResource::~ezAiControllerResource() = default;

ezResourceLoadDesc ezAiControllerResource::UnloadData(Unload WhatToUnload)
{
  m_pDesc.Clear();

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Unloaded;
  return res;
}

ezResourceLoadDesc ezAiControllerResource::UpdateContent(ezStreamReader* pStream)
{
  EZ_LOG_BLOCK("ezAiControllerResource::UpdateContent", GetResourceIdOrDescription());

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (pStream == nullptr)
  {
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  // the standard file reader writes the absolute file path into the stream
  ezStringBuilder sAbsFilePath;
  (*pStream) >> sAbsFilePath;

  ezAssetFileHeader assetHeader;
  assetHeader.Read(*pStream).IgnoreResult();

  ezSharedPtr<ezAiControllerDesc> pDesc = EZ_DEFAULT_NEW(ezAiControllerDesc);
  if (pDesc->Deserialize(*pStream).Failed())
  {
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  pDesc->Finalize();
  m_pDesc = std::move(pDesc);

  res.m_State = ezResourceState::Loaded;
  return res;
}

void ezAiControllerResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(*this);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}
