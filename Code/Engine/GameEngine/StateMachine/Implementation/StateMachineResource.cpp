#include <GameEngine/GameEnginePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <GameEngine/StateMachine/StateMachineResource.h>

// TESTING
#include <Core/Utils/Blackboard.h>
#include <GameEngine/StateMachine/Implementation/StateMachineBuiltins.h>
#include <GameEngine/StateMachine/StateMachineComponent.h>

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

ezUniquePtr<ezStateMachineInstance> ezStateMachineResource::CreateInstance(ezReflectedClass& owner)
{
  if (m_pDescription != nullptr)
  {
    auto pInstance = EZ_DEFAULT_NEW(ezStateMachineInstance, owner, m_pDescription);
    return pInstance;
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
  EZ_LOG_BLOCK("ezStateMachineResource::UpdateContent", GetResourceDescription().GetData());

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
