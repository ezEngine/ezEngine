#include <GameEngine/GameEnginePCH.h>

#include <Foundation/Utilities/AssetFileHeader.h>
#include <GameEngine/StateMachine/StateMachineResource.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezStateMachineResource, 1, ezRTTIDefaultAllocator<ezStateMachineResource>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_RESOURCE_IMPLEMENT_COMMON_CODE(ezStateMachineResource);
// clang-format on

ezStateMachineResource::ezStateMachineResource()
  : ezResource(DoUpdate::OnAnyThread, 1)
{
}

ezStateMachineResource::~ezStateMachineResource() = default;

ezUniquePtr<ezStateMachineInstance> ezStateMachineResource::CreateInstance(ezReflectedClass& ref_owner)
{
  if (m_pDescription != nullptr)
  {
    return EZ_DEFAULT_NEW(ezStateMachineInstance, ref_owner, m_pDescription);
  }

  return nullptr;
}

ezResourceLoadDesc ezStateMachineResource::UnloadData(Unload WhatToUnload)
{
  m_pDescription = nullptr;

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Unloaded;

  return res;
}

ezResourceLoadDesc ezStateMachineResource::UpdateContent(ezStreamReader* Stream)
{
  EZ_LOG_BLOCK("ezStateMachineResource::UpdateContent", GetResourceIdOrDescription());

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    ezStringBuilder sAbsFilePath;
    (*Stream) >> sAbsFilePath;
  }

  ezAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  ezUniquePtr<ezStateMachineDescription> pDescription = EZ_DEFAULT_NEW(ezStateMachineDescription);
  if (pDescription->Deserialize(*Stream).Failed())
  {
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  m_pDescription = std::move(pDescription);

  res.m_State = ezResourceState::Loaded;
  return res;
}

void ezStateMachineResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = 0;
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}


EZ_STATICLINK_FILE(GameEngine, GameEngine_StateMachine_Implementation_StateMachineResource);
