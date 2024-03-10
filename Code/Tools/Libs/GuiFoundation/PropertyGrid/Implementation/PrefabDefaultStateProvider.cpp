#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/PropertyGrid/PrefabDefaultStateProvider.h>
#include <ToolsFoundation/Document/PrefabCache.h>
#include <ToolsFoundation/Document/PrefabUtils.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

ezSharedPtr<ezDefaultStateProvider> ezPrefabDefaultStateProvider::CreateProvider(ezObjectAccessorBase* pAccessor, const ezDocumentObject* pObject, const ezAbstractProperty* pProp)
{
  const auto* pMetaData = pObject->GetDocumentObjectManager()->GetDocument()->m_DocumentObjectMetaData.Borrow();
  ezInt32 iRootDepth = 0;
  ezUuid rootObjectGuid = ezPrefabUtils::GetPrefabRoot(pObject, *pMetaData, &iRootDepth);
  // The root depth is taken x2 because GetPrefabRoot counts the number of parent objects while ezDefaultStateProvider expects to count the properties as well.
  iRootDepth *= 2;
  // If we construct this from a property scope, the root is an additional hop away as GetPrefabRoot counts from the parent object.
  if (pProp)
    iRootDepth += 1;

  if (rootObjectGuid.IsValid())
  {
    auto pMeta = pMetaData->BeginReadMetaData(rootObjectGuid);
    EZ_SCOPE_EXIT(pMetaData->EndReadMetaData(););
    ezUuid objectPrefabGuid = pObject->GetGuid();
    objectPrefabGuid.RevertCombinationWithSeed(pMeta->m_PrefabSeedGuid);
    const ezAbstractObjectGraph* pGraph = ezPrefabCache::GetSingleton()->GetCachedPrefabGraph(pMeta->m_CreateFromPrefab);
    if (pGraph)
    {
      if (pGraph->GetNode(objectPrefabGuid) != nullptr)
      {
        // The object was found in the prefab, we can thus use its prefab counterpart to provide a default state.
        return EZ_DEFAULT_NEW(ezPrefabDefaultStateProvider, rootObjectGuid, pMeta->m_CreateFromPrefab, pMeta->m_PrefabSeedGuid, iRootDepth);
      }
    }
  }
  return nullptr;
}

ezPrefabDefaultStateProvider::ezPrefabDefaultStateProvider(const ezUuid& rootObjectGuid, const ezUuid& createFromPrefab, const ezUuid& prefabSeedGuid, ezInt32 iRootDepth)
  : m_RootObjectGuid(rootObjectGuid)
  , m_CreateFromPrefab(createFromPrefab)
  , m_PrefabSeedGuid(prefabSeedGuid)
  , m_iRootDepth(iRootDepth)
{
}

ezInt32 ezPrefabDefaultStateProvider::GetRootDepth() const
{
  return m_iRootDepth;
}

ezColorGammaUB ezPrefabDefaultStateProvider::GetBackgroundColor() const
{
  return ezColorScheme::DarkUI(ezColorScheme::Blue).WithAlpha(0.25f);
}

ezVariant ezPrefabDefaultStateProvider::GetDefaultValue(SuperArray superPtr, ezObjectAccessorBase* pAccessor, const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index)
{
  const bool bIsValueType = ezReflectionUtils::IsValueType(pProp) || pProp->GetFlags().IsAnySet(ezPropertyFlags::IsEnum | ezPropertyFlags::Bitflags);

  const ezAbstractObjectGraph* pGraph = ezPrefabCache::GetSingleton()->GetCachedPrefabGraph(m_CreateFromPrefab);
  ezUuid objectPrefabGuid = pObject->GetGuid();
  objectPrefabGuid.RevertCombinationWithSeed(m_PrefabSeedGuid);
  if (pGraph)
  {
    bool bValueFound = true;
    ezVariant defaultValue = ezPrefabUtils::GetDefaultValue(*pGraph, objectPrefabGuid, pProp->GetPropertyName(), index, &bValueFound);
    if (!bValueFound)
    {
      return superPtr[0]->GetDefaultValue(superPtr.GetSubArray(1), pAccessor, pObject, pProp, index);
    }

    if (pProp->GetFlags().IsAnySet(ezPropertyFlags::IsEnum | ezPropertyFlags::Bitflags) && defaultValue.IsA<ezString>())
    {
      ezInt64 iValue = 0;
      if (ezReflectionUtils::StringToEnumeration(pProp->GetSpecificType(), defaultValue.Get<ezString>(), iValue))
      {
        defaultValue = iValue;
      }
      else
      {
        defaultValue = superPtr[0]->GetDefaultValue(superPtr.GetSubArray(1), pAccessor, pObject, pProp, index);
      }
    }
    else if (!bIsValueType)
    {
      // For object references we need to reverse the object GUID mapping from prefab -> instance.
      switch (pProp->GetCategory())
      {
        case ezPropertyCategory::Member:
        {
          ezUuid& targetGuid = defaultValue.GetWritable<ezUuid>();
          targetGuid.CombineWithSeed(m_PrefabSeedGuid);
        }
        break;
        case ezPropertyCategory::Array:
        case ezPropertyCategory::Set:
        {
          if (index.IsValid())
          {
            ezUuid& targetGuid = defaultValue.GetWritable<ezUuid>();
            targetGuid.CombineWithSeed(m_PrefabSeedGuid);
          }
          else
          {
            ezVariantArray& defaultValueArray = defaultValue.GetWritable<ezVariantArray>();
            for (ezVariant& value : defaultValueArray)
            {
              ezUuid& targetGuid = value.GetWritable<ezUuid>();
              targetGuid.CombineWithSeed(m_PrefabSeedGuid);
            }
          }
        }
        break;
        case ezPropertyCategory::Map:
        {
          if (index.IsValid())
          {
            ezUuid& targetGuid = defaultValue.GetWritable<ezUuid>();
            targetGuid.CombineWithSeed(m_PrefabSeedGuid);
          }
          else
          {
            ezVariantDictionary& defaultValueDict = defaultValue.GetWritable<ezVariantDictionary>();
            for (auto it : defaultValueDict)
            {
              ezUuid& targetGuid = it.Value().GetWritable<ezUuid>();
              targetGuid.CombineWithSeed(m_PrefabSeedGuid);
            }
          }
        }
        break;
        default:
          break;
      }
    }

    if (defaultValue.IsValid())
    {
      if (defaultValue.IsString() && pProp->GetAttributeByType<ezGameObjectReferenceAttribute>())
      {
        // While pretty expensive this restores the default state of game object references which are stored as strings.
        ezStringView sValue = defaultValue.GetType() == ezVariantType::StringView ? defaultValue.Get<ezStringView>() : ezStringView(defaultValue.Get<ezString>().GetData());
        if (ezConversionUtils::IsStringUuid(sValue))
        {
          ezUuid guid = ezConversionUtils::ConvertStringToUuid(sValue);
          guid.CombineWithSeed(m_PrefabSeedGuid);
          ezStringBuilder sTemp;
          defaultValue = ezConversionUtils::ToString(guid, sTemp).GetData();
        }
      }

      return defaultValue;
    }
  }
  return superPtr[0]->GetDefaultValue(superPtr.GetSubArray(1), pAccessor, pObject, pProp);
}

ezStatus ezPrefabDefaultStateProvider::CreateRevertContainerDiff(SuperArray superPtr, ezObjectAccessorBase* pAccessor, const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezDeque<ezAbstractGraphDiffOperation>& out_diff)
{
  ezVariant defaultValue = GetDefaultValue(superPtr, pAccessor, pObject, pProp);
  ezVariant currentValue;
  EZ_SUCCEED_OR_RETURN(pAccessor->GetValue(pObject, pProp, currentValue));

  const ezAbstractObjectGraph* pGraph = ezPrefabCache::GetSingleton()->GetCachedPrefabGraph(m_CreateFromPrefab);
  ezUuid objectPrefabGuid = pObject->GetGuid();
  objectPrefabGuid.RevertCombinationWithSeed(m_PrefabSeedGuid);
  if (pGraph)
  {
    // We create a sub-graph of only the parent node in both re-mapped prefab as well as from the actually object. We limit the graph to only the container property.
    auto pNode = pGraph->GetNode(objectPrefabGuid);
    ezAbstractObjectGraph prefabSubGraph;
    pGraph->Clone(prefabSubGraph, pNode, [pRootNode = pNode, pRootProp = pProp](const ezAbstractObjectNode* pNode, const ezAbstractObjectNode::Property* pProp)
      {
        if (pNode == pRootNode && pProp->m_sPropertyName != pRootProp->GetPropertyName())
          return false;

        return true; //
      });

    prefabSubGraph.ReMapNodeGuids(m_PrefabSeedGuid);

    ezAbstractObjectGraph instanceSubGraph;
    ezDocumentObjectConverterWriter writer(&instanceSubGraph, pObject->GetDocumentObjectManager(), [pRootObject = pObject, pRootProp = pProp](const ezDocumentObject* pObject, const ezAbstractProperty* pProp)
      {
        if (pObject == pRootObject && pProp != pRootProp)
          return false;

        return true; //
      });

    writer.AddObjectToGraph(pObject);

    prefabSubGraph.CreateDiffWithBaseGraph(instanceSubGraph, out_diff);

    return ezStatus(EZ_SUCCESS);
  }

  return ezStatus(ezFmt("The object was not found in the base prefab graph."));
}
