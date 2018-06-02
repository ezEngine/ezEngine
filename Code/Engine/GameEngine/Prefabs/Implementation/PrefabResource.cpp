#include <PCH.h>
#include <GameEngine/Prefabs/PrefabResource.h>
#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/Reflection/ReflectionUtils.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPrefabResource, 1, ezRTTIDefaultAllocator<ezPrefabResource>);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezPrefabResource::ezPrefabResource()
  : ezResource<ezPrefabResource, ezPrefabResourceDescriptor>(DoUpdate::OnAnyThread, 1)
{

}

void ezPrefabResource::InstantiatePrefab(ezWorld& world, const ezTransform& rootTransform, ezGameObjectHandle hParent, ezHybridArray<ezGameObject*, 8>* out_CreatedRootObjects, const ezUInt16* pOverrideTeamID, const ezArrayMap<ezHashedString, ezVariant>* pExposedParamValues)
{
  if (GetLoadingState() != ezResourceState::Loaded)
    return;

  if (pExposedParamValues != nullptr && !pExposedParamValues->IsEmpty())
  {
    ezHybridArray<ezGameObject*, 8> createdRootObjects;
    ezHybridArray<ezGameObject*, 8> createdChildObjects;

    if (out_CreatedRootObjects == nullptr)
      out_CreatedRootObjects = &createdRootObjects;

    m_WorldReader.InstantiatePrefab(world, rootTransform, hParent, out_CreatedRootObjects, &createdChildObjects, pOverrideTeamID);

    for (ezUInt32 i = 0; i < pExposedParamValues->GetCount(); ++i)
    {
      const ezHashedString& name = pExposedParamValues->GetKey(i);

      // TODO: use a sorted array lookup (ArrayMap)
      for (const auto& ppd : m_PrefabParamDescs)
      {
        if (ppd.m_sExposeName == name)
        {
          ezGameObject* pTarget = ppd.m_uiWorldReaderChildObject ? createdChildObjects[ppd.m_uiWorldReaderObjectIndex] : (*out_CreatedRootObjects)[ppd.m_uiWorldReaderObjectIndex];

          for (ezComponent* pComp : pTarget->GetComponents())
          {
            const ezRTTI* pRtti = pComp->GetDynamicRTTI();

            // TODO: use component index instead ?
            if (pRtti->GetTypeNameHash() == ppd.m_uiComponentTypeHash)
            {
              ezAbstractProperty* pAbstract = pRtti->FindPropertyByName(ppd.m_sProperty);

              if (pAbstract && pAbstract->GetCategory() == ezPropertyCategory::Member)
              {
                ezReflectionUtils::SetMemberPropertyValue(static_cast<ezAbstractMemberProperty*>(pAbstract), pComp, pExposedParamValues->GetValue(i));
              }
            }
          }
          //#TODO: Should we remove the break so multiple properties can bind to the same name?
          break;
        }
      }
    }
  }
  else
  {
    m_WorldReader.InstantiatePrefab(world, rootTransform, hParent, out_CreatedRootObjects, nullptr, pOverrideTeamID);
  }
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

  ezStreamReader& s = *Stream;

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    ezString sAbsFilePath;
    s >> sAbsFilePath;
  }

  ezAssetFileHeader AssetHash;
  AssetHash.Read(s);

  char szSceneTag[16];
  s.ReadBytes(szSceneTag, sizeof(char) * 16);
  EZ_ASSERT_DEV(ezStringUtils::IsEqualN(szSceneTag, "[ezBinaryScene]", 16), "The given file is not a valid prefab file");

  if (!ezStringUtils::IsEqualN(szSceneTag, "[ezBinaryScene]", 16))
  {
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  m_WorldReader.ReadWorldDescription(s);

  if (AssetHash.GetFileVersion() >= 4)
  {
    ezUInt32 uiExposedParams = 0;

    s >> uiExposedParams;

    m_PrefabParamDescs.SetCount(uiExposedParams);

    for (ezUInt32 i = 0; i < uiExposedParams; ++i)
    {
      m_PrefabParamDescs[i].Load(s);
    }
  }

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

void ezExposedPrefabParameterDesc::Save(ezStreamWriter& stream) const
{
  ezUInt32 comb = m_uiWorldReaderObjectIndex | (m_uiWorldReaderChildObject << 31);

  stream << m_sExposeName;
  stream << comb;
  stream << m_uiComponentTypeHash;
  stream << m_sProperty;
}

void ezExposedPrefabParameterDesc::Load(ezStreamReader& stream)
{
  ezUInt32 comb = 0;

  stream >> m_sExposeName;
  stream >> comb;
  stream >> m_uiComponentTypeHash;
  stream >> m_sProperty;

  m_uiWorldReaderObjectIndex = comb & 0x7FFFFFFF;
  m_uiWorldReaderChildObject = (comb >> 31);
}

EZ_STATICLINK_FILE(GameEngine, GameEngine_Prefabs_Implementation_PrefabResource);

