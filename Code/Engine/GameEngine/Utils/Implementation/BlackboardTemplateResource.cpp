#include <GameEngine/GameEnginePCH.h>

#include <Foundation/Utilities/AssetFileHeader.h>
#include <GameEngine/Utils/BlackboardTemplateResource.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezBlackboardTemplateResource, 1, ezRTTIDefaultAllocator<ezBlackboardTemplateResource>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_RESOURCE_IMPLEMENT_COMMON_CODE(ezBlackboardTemplateResource);
// clang-format on

ezBlackboardTemplateResource::ezBlackboardTemplateResource()
  : ezResource(DoUpdate::OnAnyThread, 1)
{
}

ezBlackboardTemplateResource::~ezBlackboardTemplateResource() = default;

ezResourceLoadDesc ezBlackboardTemplateResource::UnloadData(Unload WhatToUnload)
{
  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Unloaded;

  return res;
}

ezResourceLoadDesc ezBlackboardTemplateResource::UpdateContent(ezStreamReader* Stream)
{
  EZ_LOG_BLOCK("ezBlackboardTemplateResource::UpdateContent", GetResourceIdOrDescription());

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  // skip the absolute file path data that the standard file reader writes into the stream
  ezStringBuilder sAbsFilePath;
  (*Stream) >> sAbsFilePath;

  // skip the asset file header at the start of the file
  ezAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  ezBlackboardTemplateResourceDescriptor desc;
  if (desc.Deserialize(*Stream).Failed())
  {
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  CreateResource(std::move(desc));

  res.m_State = ezResourceState::Loaded;
  return res;
}

void ezBlackboardTemplateResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(ezBlackboardTemplateResource);
  out_NewMemoryUsage.m_uiMemoryCPU += m_Descriptor.m_Entries.GetHeapMemoryUsage();
}

EZ_RESOURCE_IMPLEMENT_CREATEABLE(ezBlackboardTemplateResource, ezBlackboardTemplateResourceDescriptor)
{
  m_Descriptor = std::move(descriptor);

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  return res;
}

ezResult ezBlackboardTemplateResourceDescriptor::Serialize(ezStreamWriter& inout_stream) const
{
  EZ_SUCCEED_OR_RETURN(inout_stream.WriteArray(m_Entries));
  return EZ_SUCCESS;
}

ezResult ezBlackboardTemplateResourceDescriptor::Deserialize(ezStreamReader& inout_stream)
{
  EZ_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_Entries));
  return EZ_SUCCESS;
}


EZ_STATICLINK_FILE(GameEngine, GameEngine_Utils_Implementation_BlackboardTemplateResource);
