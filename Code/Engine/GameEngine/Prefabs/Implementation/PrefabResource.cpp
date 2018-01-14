#include <PCH.h>
#include <GameEngine/Prefabs/PrefabResource.h>
#include <Core/Assets/AssetFileHeader.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPrefabResource, 1, ezRTTIDefaultAllocator<ezPrefabResource>);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezPrefabResource::ezPrefabResource()
  : ezResource<ezPrefabResource, ezPrefabResourceDescriptor>(DoUpdate::OnAnyThread, 1)
{

}

void ezPrefabResource::InstantiatePrefab(ezWorld& world, const ezTransform& rootTransform, ezGameObjectHandle hParent, ezHybridArray<ezGameObject*, 8>* out_CreatedRootObjects, const ezUInt16* pOverrideTeamID)
{
  if (GetLoadingState() != ezResourceState::Loaded)
    return;

  m_WorldReader.InstantiatePrefab(world, rootTransform, hParent, out_CreatedRootObjects, nullptr, pOverrideTeamID);
}

ezResourceLoadDesc ezPrefabResource::UnloadData(Unload WhatToUnload)
{
  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Unloaded;

  if (WhatToUnload == ezResourceBase::Unload::AllQualityLevels)
  {
    m_WorldReader.ClearAndCompact();
  }

  return res;
}

ezResourceLoadDesc ezPrefabResource::UpdateContent(ezStreamReader* Stream)
{
  EZ_LOG_BLOCK("ezPrefabResource::UpdateContent", GetResourceDescription().GetData());

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
    ezString sAbsFilePath;
    (*Stream) >> sAbsFilePath;
  }

  ezAssetFileHeader AssetHash;
  AssetHash.Read(*Stream);

  char szSceneTag[16];
  Stream->ReadBytes(szSceneTag, sizeof(char) * 16);
  EZ_ASSERT_DEV(ezStringUtils::IsEqualN(szSceneTag, "[ezBinaryScene]", 16), "The given file is not a valid prefab file");

  if (!ezStringUtils::IsEqualN(szSceneTag, "[ezBinaryScene]", 16))
  {
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  m_WorldReader.ReadWorldDescription(*Stream);

  res.m_State = ezResourceState::Loaded;
  return res;
}

void ezPrefabResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
  out_NewMemoryUsage.m_uiMemoryCPU = (ezUInt32) (m_WorldReader.GetHeapMemoryUsage() + sizeof(this));
}

ezResourceLoadDesc ezPrefabResource::CreateResource(const ezPrefabResourceDescriptor& descriptor)
{
  ezResourceLoadDesc desc;
  desc.m_State = ezResourceState::Loaded;
  desc.m_uiQualityLevelsDiscardable = 0;
  desc.m_uiQualityLevelsLoadable = 0;
  return desc;
}

