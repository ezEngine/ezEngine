#include <Core/CorePCH.h>

#include <Core/Prefabs/PrefabResource.h>
#include <Foundation/Reflection/PropertyPath.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Utilities/AssetFileHeader.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPrefabResource, 1, ezRTTIDefaultAllocator<ezPrefabResource>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_RESOURCE_IMPLEMENT_COMMON_CODE(ezPrefabResource);
// clang-format on

ezPrefabResource::ezPrefabResource()
  : ezResource(DoUpdate::OnAnyThread, 1)
{
}

void ezPrefabResource::InstantiatePrefab(ezWorld& ref_world, const ezTransform& rootTransform, ezPrefabInstantiationOptions options, const ezArrayMap<ezHashedString, ezVariant>* pExposedParamValues)
{
  if (GetLoadingState() != ezResourceState::Loaded)
    return;

  if (pExposedParamValues != nullptr && !pExposedParamValues->IsEmpty())
  {
    ezHybridArray<ezGameObject*, 8> createdRootObjects;
    ezHybridArray<ezGameObject*, 8> createdChildObjects;

    if (options.m_pCreatedRootObjectsOut == nullptr)
    {
      options.m_pCreatedRootObjectsOut = &createdRootObjects;
    }

    if (options.m_pCreatedChildObjectsOut == nullptr)
    {
      options.m_pCreatedChildObjectsOut = &createdChildObjects;
    }

    m_WorldReader.InstantiatePrefab(ref_world, rootTransform, options);

    EZ_ASSERT_DEBUG(options.m_pCreatedRootObjectsOut != options.m_pCreatedChildObjectsOut, "These pointers must point to different arrays, otherwise applying exposed properties doesn't work correctly.");
    ApplyExposedParameterValues(pExposedParamValues, *options.m_pCreatedChildObjectsOut, *options.m_pCreatedRootObjectsOut);
  }
  else
  {
    m_WorldReader.InstantiatePrefab(ref_world, rootTransform, options);
  }
}

ezPrefabResource::InstantiateResult ezPrefabResource::InstantiatePrefab(const ezPrefabResourceHandle& hPrefab, bool bBlockTillLoaded, ezWorld& ref_world, const ezTransform& rootTransform, ezPrefabInstantiationOptions options, const ezArrayMap<ezHashedString, ezVariant>* pExposedParamValues /*= nullptr*/)
{
  ezResourceLock<ezPrefabResource> pPrefab(hPrefab, bBlockTillLoaded ? ezResourceAcquireMode::BlockTillLoaded_NeverFail : ezResourceAcquireMode::AllowLoadingFallback_NeverFail);

  switch (pPrefab.GetAcquireResult())
  {
    case ezResourceAcquireResult::Final:
      pPrefab->InstantiatePrefab(ref_world, rootTransform, options, pExposedParamValues);
      return InstantiateResult::Success;

    case ezResourceAcquireResult::LoadingFallback:
      return InstantiateResult::NotYetLoaded;

    default:
      return InstantiateResult::Error;
  }
}

void ezPrefabResource::ApplyExposedParameterValues(const ezArrayMap<ezHashedString, ezVariant>* pExposedParamValues, const ezDynamicArray<ezGameObject*>& createdChildObjects, const ezDynamicArray<ezGameObject*>& createdRootObjects) const
{
  const ezUInt32 uiNumParamDescs = m_PrefabParamDescs.GetCount();

  for (ezUInt32 i = 0; i < pExposedParamValues->GetCount(); ++i)
  {
    const ezHashedString& name = pExposedParamValues->GetKey(i);
    const ezUInt64 uiNameHash = name.GetHash();

    for (ezUInt32 uiCurParam = FindFirstParamWithName(uiNameHash); uiCurParam < uiNumParamDescs; ++uiCurParam)
    {
      const auto& ppd = m_PrefabParamDescs[uiCurParam];

      if (ppd.m_sExposeName.GetHash() != uiNameHash)
        break;

      ezGameObject* pTarget = ppd.m_uiWorldReaderChildObject ? createdChildObjects[ppd.m_uiWorldReaderObjectIndex] : createdRootObjects[ppd.m_uiWorldReaderObjectIndex];

      if (ppd.m_CachedPropertyPath.IsValid())
      {
        if (ppd.m_sComponentType.IsEmpty())
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
            if (pRtti->GetTypeNameHash() == ppd.m_sComponentType.GetHash())
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
  EZ_LOG_BLOCK("ezPrefabResource::UpdateContent", GetResourceIdOrDescription());

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

  ezAssetFileHeader assetHeader;
  assetHeader.Read(s).IgnoreResult();

  char szSceneTag[16];
  s.ReadBytes(szSceneTag, sizeof(char) * 16);
  EZ_ASSERT_DEV(ezStringUtils::IsEqualN(szSceneTag, "[ezBinaryScene]", 16), "The given file is not a valid prefab file");

  if (!ezStringUtils::IsEqualN(szSceneTag, "[ezBinaryScene]", 16))
  {
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  m_WorldReader.ReadWorldDescription(s).IgnoreResult();

  if (assetHeader.GetFileVersion() >= 4)
  {
    ezUInt32 uiExposedParams = 0;

    s >> uiExposedParams;

    m_PrefabParamDescs.SetCount(uiExposedParams);

    for (ezUInt32 i = 0; i < uiExposedParams; ++i)
    {
      auto& ppd = m_PrefabParamDescs[i];

      EZ_ASSERT_DEV(assetHeader.GetFileVersion() >= 6, "Old resource version not supported anymore");
      ppd.Load(s);

      // initialize the cached property path here once
      // so we can only apply it later as often as needed
      {
        if (ppd.m_sComponentType.IsEmpty())
        {
          ppd.m_CachedPropertyPath.InitializeFromPath(*ezGetStaticRTTI<ezGameObject>(), ppd.m_sProperty).IgnoreResult();
        }
        else
        {
          if (const ezRTTI* pRtti = ezRTTI::FindTypeByNameHash(ppd.m_sComponentType.GetHash()))
          {
            ppd.m_CachedPropertyPath.InitializeFromPath(*pRtti, ppd.m_sProperty).IgnoreResult();
          }
        }
      }
    }

    // sort exposed parameter descriptions by name hash for quicker access
    m_PrefabParamDescs.Sort([](const ezExposedPrefabParameterDesc& lhs, const ezExposedPrefabParameterDesc& rhs) -> bool
      { return lhs.m_sExposeName.GetHash() < rhs.m_sExposeName.GetHash(); });
  }

  res.m_State = ezResourceState::Loaded;
  return res;
}

void ezPrefabResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
  out_NewMemoryUsage.m_uiMemoryCPU = m_WorldReader.GetHeapMemoryUsage() + sizeof(this);
}

EZ_RESOURCE_IMPLEMENT_CREATEABLE(ezPrefabResource, ezPrefabResourceDescriptor)
{
  EZ_IGNORE_UNUSED(descriptor);

  ezResourceLoadDesc desc;
  desc.m_State = ezResourceState::Loaded;
  desc.m_uiQualityLevelsDiscardable = 0;
  desc.m_uiQualityLevelsLoadable = 0;
  return desc;
}

ezUInt32 ezPrefabResource::FindFirstParamWithName(ezUInt64 uiNameHash) const
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

void ezExposedPrefabParameterDesc::Save(ezStreamWriter& inout_stream) const
{
  ezUInt32 comb = m_uiWorldReaderObjectIndex | (m_uiWorldReaderChildObject << 31);

  inout_stream << m_sExposeName;
  inout_stream << comb;
  inout_stream << m_sComponentType;
  inout_stream << m_sProperty;
}

void ezExposedPrefabParameterDesc::Load(ezStreamReader& inout_stream)
{
  ezUInt32 comb = 0;

  inout_stream >> m_sExposeName;
  inout_stream >> comb;
  inout_stream >> m_sComponentType;
  inout_stream >> m_sProperty;

  m_uiWorldReaderObjectIndex = comb & 0x7FFFFFFF;
  m_uiWorldReaderChildObject = (comb >> 31);
}

EZ_STATICLINK_FILE(Core, Core_Prefabs_Implementation_PrefabResource);
