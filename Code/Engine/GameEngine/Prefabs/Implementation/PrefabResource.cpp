#include <GameEnginePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/Reflection/PropertyPath.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <GameEngine/Prefabs/PrefabResource.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPrefabResource, 1, ezRTTIDefaultAllocator<ezPrefabResource>);
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_RESOURCE_IMPLEMENT_COMMON_CODE(ezPrefabResource);
// clang-format on

ezPrefabResource::ezPrefabResource()
    : ezResource(DoUpdate::OnAnyThread, 1)
{
}

void ezPrefabResource::InstantiatePrefab(ezWorld& world, const ezTransform& rootTransform, ezGameObjectHandle hParent,
                                         ezHybridArray<ezGameObject*, 8>* out_CreatedRootObjects, const ezUInt16* pOverrideTeamID,
                                         const ezArrayMap<ezHashedString, ezVariant>* pExposedParamValues)
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

    ApplyExposedParameterValues(pExposedParamValues, createdChildObjects, *out_CreatedRootObjects);
  }
  else
  {
    m_WorldReader.InstantiatePrefab(world, rootTransform, hParent, out_CreatedRootObjects, nullptr, pOverrideTeamID);
  }
}

void ezPrefabResource::ApplyExposedParameterValues(const ezArrayMap<ezHashedString, ezVariant>* pExposedParamValues,
                                                   const ezHybridArray<ezGameObject*, 8>& createdChildObjects,
                                                   const ezHybridArray<ezGameObject*, 8>& createdRootObjects) const
{
  const ezUInt32 uiNumParamDescs = m_PrefabParamDescs.GetCount();

  for (ezUInt32 i = 0; i < pExposedParamValues->GetCount(); ++i)
  {
    const ezHashedString& name = pExposedParamValues->GetKey(i);
    const ezUInt32 uiNameHash = name.GetHash();

    for (ezUInt32 uiCurParam = FindFirstParamWithName(uiNameHash); uiCurParam < uiNumParamDescs; ++uiCurParam)
    {
      const auto& ppd = m_PrefabParamDescs[uiCurParam];

      if (ppd.m_sExposeName.GetHash() != uiNameHash)
        break;

      ezGameObject* pTarget = ppd.m_uiWorldReaderChildObject ? createdChildObjects[ppd.m_uiWorldReaderObjectIndex]
                                                             : createdRootObjects[ppd.m_uiWorldReaderObjectIndex];

      if (ppd.m_CachedPropertyPath.IsValid())
      {
        if (ppd.m_uiComponentTypeHash == 0)
        {
          ppd.m_CachedPropertyPath.SetValue(pTarget, pExposedParamValues->GetValue(i));
        }
        else
        {
          for (ezComponent* pComp : pTarget->GetComponents())
          {
            const ezRTTI* pRtti = pComp->GetDynamicRTTI();

            // TODO: use component index instead
            // atm if the same component type is attached multiple times, they will all get the value applied
            if (pRtti->GetTypeNameHash() == ppd.m_uiComponentTypeHash)
            {
              ppd.m_CachedPropertyPath.SetValue(pComp, pExposedParamValues->GetValue(i));
            }
          }
        }
      }

      // Allow to bind multiple properties to the same exposed parameter name
      // Therefore, do not break here, but continue iterating
    }
  }
}

ezResourceLoadDesc ezPrefabResource::UnloadData(Unload WhatToUnload)
{
  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Unloaded;

  if (WhatToUnload == ezResource::Unload::AllQualityLevels)
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
      auto& ppd = m_PrefabParamDescs[i];

      ppd.Load(s);

      // initialize the cached property path here once
      // so we can only apply it later as often as needed
      {
        if (ppd.m_uiComponentTypeHash == 0)
          ppd.m_CachedPropertyPath.InitializeFromPath(*ezGetStaticRTTI<ezGameObject>(), ppd.m_sProperty);
        else
        {
          for (const ezRTTI* pRtti = ezRTTI::GetFirstInstance(); pRtti != nullptr; pRtti = pRtti->GetNextInstance())
          {
            if (pRtti->GetTypeNameHash() == ppd.m_uiComponentTypeHash)
            {
              ppd.m_CachedPropertyPath.InitializeFromPath(*pRtti, ppd.m_sProperty);
              break;
            }
          }
        }
      }
    }

    // sort exposed parameter descriptions by name hash for quicker access
    m_PrefabParamDescs.Sort([](const ezExposedPrefabParameterDesc& lhs, const ezExposedPrefabParameterDesc& rhs) -> bool {
      return lhs.m_sExposeName.GetHash() < rhs.m_sExposeName.GetHash();
    });
  }

  res.m_State = ezResourceState::Loaded;
  return res;
}

void ezPrefabResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
  out_NewMemoryUsage.m_uiMemoryCPU = (ezUInt32)(m_WorldReader.GetHeapMemoryUsage() + sizeof(this));
}

EZ_RESOURCE_IMPLEMENT_CREATEABLE(ezPrefabResource, ezPrefabResourceDescriptor)
{
  ezResourceLoadDesc desc;
  desc.m_State = ezResourceState::Loaded;
  desc.m_uiQualityLevelsDiscardable = 0;
  desc.m_uiQualityLevelsLoadable = 0;
  return desc;
}

ezUInt32 ezPrefabResource::FindFirstParamWithName(ezUInt32 uiNameHash) const
{
  ezUInt32 lb = 0;
  ezUInt32 ub = m_PrefabParamDescs.GetCount();

  while (lb < ub)
  {
    const ezUInt32 middle = lb + ((ub - lb) >> 1);

    if (m_PrefabParamDescs[middle].m_sExposeName.GetHash() < uiNameHash)
    {
      lb = middle + 1;
    }
    else
    {
      ub = middle;
    }
  }

  return lb;
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

