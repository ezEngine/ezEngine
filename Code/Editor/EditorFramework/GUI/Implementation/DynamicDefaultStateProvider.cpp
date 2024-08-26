#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/GUI/DynamicDefaultStateProvider.h>
#include <EditorFramework/Object/ObjectPropertyPath.h>
#include <Foundation/Reflection/Implementation/PropertyAttributes.h>
#include <Foundation/Reflection/PropertyPath.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

ezSharedPtr<ezDefaultStateProvider> ezDynamicDefaultStateProvider::CreateProvider(ezObjectAccessorBase* pAccessor, const ezDocumentObject* pObject, const ezAbstractProperty* pProp)
{
  if (pProp)
  {
    auto* pAttrib = pProp->GetAttributeByType<ezDynamicDefaultValueAttribute>();
    if (pAttrib && !ezStringUtils::IsNullOrEmpty(pAttrib->GetClassProperty()))
    {
      return EZ_DEFAULT_NEW(ezDynamicDefaultStateProvider, pAccessor, pObject, pObject, pObject, pProp, 0);
    }
  }

  ezInt32 iRootDepth = 0;
  if (pProp)
    iRootDepth += 1;

  const ezDocumentObject* pCurrentObject = pObject;
  while (pCurrentObject)
  {
    const ezAbstractProperty* pParentProp = pCurrentObject->GetParentPropertyType();
    if (!pParentProp)
      return nullptr;

    const auto* pAttrib = pParentProp->GetAttributeByType<ezDynamicDefaultValueAttribute>();
    if (pAttrib)
    {
      iRootDepth += 1;
      return EZ_DEFAULT_NEW(ezDynamicDefaultStateProvider, pAccessor, pObject, pCurrentObject, pCurrentObject->GetParent(), pParentProp, iRootDepth);
    }
    iRootDepth += 2;
    pCurrentObject = pCurrentObject->GetParent();
  }
  return nullptr;
}

ezDynamicDefaultStateProvider::ezDynamicDefaultStateProvider(ezObjectAccessorBase* pAccessor, const ezDocumentObject* pObject, const ezDocumentObject* pClassObject, const ezDocumentObject* pRootObject, const ezAbstractProperty* pRootProp, ezInt32 iRootDepth)
  : m_pObject(pObject)
  , m_pClassObject(pClassObject)
  , m_pRootObject(pRootObject)
  , m_pRootProp(pRootProp)
  , m_iRootDepth(iRootDepth)
{
  m_pAttrib = m_pRootProp->GetAttributeByType<ezDynamicDefaultValueAttribute>();
  EZ_ASSERT_DEBUG(m_pAttrib, "ezDynamicDefaultStateProvider was created for a property that does not have the ezDynamicDefaultValueAttribute.");

  m_pClassType = ezRTTI::FindTypeByName(m_pAttrib->GetClassType());
  EZ_ASSERT_DEBUG(m_pClassType, "The dynamic meta data class type '{0}' does not exist", m_pAttrib->GetClassType());

  m_pClassSourceProp = m_pRootObject->GetType()->FindPropertyByName(m_pAttrib->GetClassSource());
  EZ_ASSERT_DEBUG(m_pClassSourceProp, "The dynamic meta data class source '{0}' does not exist on type '{1}'", m_pAttrib->GetClassSource(), m_pRootObject->GetType()->GetTypeName());

  const bool bHasProperty = !ezStringUtils::IsNullOrEmpty(m_pAttrib->GetClassProperty());
  if (!bHasProperty)
  {
    EZ_ASSERT_DEBUG(m_pRootProp->GetCategory() == ezPropertyCategory::Member, "ezDynamicDefaultValueAttribute must be on a member property if no ClassProperty is given.");
  }
  else
  {
    m_pClassProperty = m_pClassType->FindPropertyByName(m_pAttrib->GetClassProperty());

    EZ_ASSERT_DEBUG(m_pClassProperty, "The dynamic meta data class type '{0}' does not have a property named '{1}'", m_pAttrib->GetClassType(), m_pAttrib->GetClassProperty());
  }
}

ezInt32 ezDynamicDefaultStateProvider::GetRootDepth() const
{
  return m_iRootDepth;
}

ezColorGammaUB ezDynamicDefaultStateProvider::GetBackgroundColor() const
{
  // Set alpha to 0 -> color will be ignored.
  return ezColorGammaUB(0, 0, 0, 0);
}

ezVariant ezDynamicDefaultStateProvider::GetDefaultValue(SuperArray superPtr, ezObjectAccessorBase* pAccessor, const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index)
{
  const bool bIsValueType = ezReflectionUtils::IsValueType(pProp) || pProp->GetFlags().IsAnySet(ezPropertyFlags::IsEnum | ezPropertyFlags::Bitflags);

  if (const ezReflectedClass* pMeta = GetMetaInfo(pAccessor))
  {
    ezPropertyPath propertyPath;
    if (CreatePath(pAccessor, pMeta, propertyPath, pObject, pProp, index).Failed())
    {
      return superPtr[0]->GetDefaultValue(superPtr.GetSubArray(1), pAccessor, pObject, pProp, index);
    }

    ezVariant defaultValue;
    ezResult res = propertyPath.ReadProperty(const_cast<ezReflectedClass*>(pMeta), *pMeta->GetDynamicRTTI(), [&](void* pLeaf, const ezRTTI& type, const ezAbstractProperty* pNativeProp, const ezVariant& index)
      {
      EZ_ASSERT_DEBUG(pProp->GetCategory() == pNativeProp->GetCategory(), "While properties don't need to match exactly, they need to be of the same category and type.");

      switch (pNativeProp->GetCategory())
      {
        case ezPropertyCategory::Member:
          defaultValue = ezReflectionUtils::GetMemberPropertyValue(static_cast<const ezAbstractMemberProperty*>(pNativeProp), pLeaf);
          break;
        case ezPropertyCategory::Array:
        {
          ezVariant currentValue;
          pAccessor->GetValue(pObject, pProp, currentValue).LogFailure();
          const ezVariantArray& currentArray = currentValue.Get<ezVariantArray>();

          auto* pArrayProp = static_cast<const ezAbstractArrayProperty*>(pNativeProp);
          if (!index.IsValid())
          {
            ezVariantArray varArray;
            varArray.SetCount(pArrayProp->GetCount(pLeaf));
            for (ezUInt32 i = 0; i < pArrayProp->GetCount(pLeaf); i++)
            {
              if (bIsValueType)
              {
                varArray[i] = ezReflectionUtils::GetArrayPropertyValue(pArrayProp, pLeaf, i);
              }
              else
              {
                // We don't have any guid on the native object. Thus we just match the count basically and fill everything we can't find on our current object with 'nullptr', i.e. an invalid guid.
                if (i < currentArray.GetCount())
                {
                  varArray[i] = currentArray[i];
                }
                else
                {
                  varArray[i] = ezUuid();
                }
              }
            }
            defaultValue = std::move(varArray);
          }
          else
          {
            if (bIsValueType)
            {
              defaultValue = ezReflectionUtils::GetArrayPropertyValue(pArrayProp, pLeaf, index.ConvertTo<ezInt32>());
            }
            else
            {
              ezUInt32 iIndex = index.ConvertTo<ezUInt32>();
              if (iIndex < currentArray.GetCount())
              {
                defaultValue = currentArray[iIndex];
              }
              else
              {
                defaultValue = ezUuid();
              }
            }
          }
        }
        break;
        case ezPropertyCategory::Map:
        {
          auto* pMapProp = static_cast<const ezAbstractMapProperty*>(pNativeProp);

          ezVariant currentValue;
          pAccessor->GetValue(pObject, pProp, currentValue).LogFailure();
          const ezVariantDictionary& currentDict = currentValue.Get<ezVariantDictionary>();

          if (!index.IsValid())
          {
            ezHybridArray<ezString, 16> keys;
            pMapProp->GetKeys(pLeaf, keys);

            ezVariantDictionary varDict;
            for (auto& key : keys)
            {
              if (bIsValueType)
              {
                varDict.Insert(key, ezReflectionUtils::GetMapPropertyValue(pMapProp, pLeaf, key));
              }
              else
              {
                if (auto* pValue = currentDict.GetValue(key))
                {
                  varDict.Insert(key, *pValue);
                }
                else
                {
                  varDict.Insert(key, ezUuid());
                }
              }
            }
            defaultValue = std::move(varDict);
          }
          else
          {
            if (bIsValueType)
            {
              defaultValue = ezReflectionUtils::GetMapPropertyValue(pMapProp, pLeaf, index.Get<ezString>());
            }
            else
            {
              if (auto* pValue = currentDict.GetValue(index.Get<ezString>()))
              {
                defaultValue = *pValue;
              }
              else
              {
                defaultValue = ezUuid();
              }
            }
          }
        }
        break;
        default:
          EZ_ASSERT_NOT_IMPLEMENTED;
          break;
      } });

    if (res.Succeeded())
    {
      if (!DoesVariantMatchProperty(defaultValue, pProp, index))
      {
        ezLog::Error("Default value '{}' does not match property '{}' at index '{}'", defaultValue, pProp->GetPropertyName(), index);
      }
      else
      {
        return defaultValue;
      }
    }
  }
  return superPtr[0]->GetDefaultValue(superPtr.GetSubArray(1), pAccessor, pObject, pProp, index);
}

const ezReflectedClass* ezDynamicDefaultStateProvider::GetMetaInfo(ezObjectAccessorBase* pAccessor) const
{
  ezVariant value;
  if (pAccessor->GetValue(m_pRootObject, m_pClassSourceProp, value).Succeeded())
  {
    if (value.IsA<ezString>())
    {
      const auto& sValue = value.Get<ezString>();
      if (const auto asset = ezAssetCurator::GetSingleton()->FindSubAsset(sValue.GetData()))
      {
        return asset->m_pAssetInfo->m_Info->GetMetaInfo(m_pClassType);
      }
    }
  }

  return nullptr;
}

const ezResult ezDynamicDefaultStateProvider::CreatePath(ezObjectAccessorBase* pAccessor, const ezReflectedClass* pMeta, ezPropertyPath& propertyPath, const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index)
{
  ezObjectPropertyPathContext pathContext = {m_pClassProperty ? m_pRootObject : m_pClassObject, pAccessor, "Children"};

  ezPropertyReference ref;
  ref.m_Object = pObject->GetGuid();
  ref.m_pProperty = pProp;
  ref.m_Index = index;

  ezStringBuilder sPropPath;
  ezObjectPropertyPath::CreatePropertyPath(pathContext, ref, sPropPath).LogFailure();
  if (m_pClassProperty)
  {
    sPropPath.ReplaceFirst(m_pRootProp->GetPropertyName(), m_pAttrib->GetClassProperty());
  }

  return propertyPath.InitializeFromPath(*pMeta->GetDynamicRTTI(), sPropPath);
}

ezStatus ezDynamicDefaultStateProvider::CreateRevertContainerDiff(SuperArray superPtr, ezObjectAccessorBase* pAccessor, const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezDeque<ezAbstractGraphDiffOperation>& out_diff)
{
  if (const ezReflectedClass* pMeta = GetMetaInfo(pAccessor))
  {
    ezPropertyPath propertyPath;
    if (CreatePath(pAccessor, pMeta, propertyPath, pObject, pProp).Failed())
    {
      return ezStatus(ezFmt("Failed to find root object in object graph"));
    }

    ezAbstractObjectGraph prefabSubGraph;
    ezAbstractObjectNode* pPrefabSubRoot = nullptr;
    {
      // Create a graph of the native object, skipping all other properties except for the container in question.
      ezRttiConverterContext context;
      ezString sRootPropertyName = pProp->GetPropertyName();
      // If we are dealing with an attributed container and pObject is its parent, then the root container property name can differ between the meta info and the target object so we have to rename it later to make the two graphs match.
      if (m_pClassProperty && pObject == m_pRootObject)
      {
        sRootPropertyName = m_pAttrib->GetClassProperty();
      }

      void* pNativeRootObject = nullptr;
      ezRttiConverterWriter rttiConverter(&prefabSubGraph, &context, [&](const void* pObject, const ezAbstractProperty* pCurrentProp)
        {
        if (pNativeRootObject == pObject && pCurrentProp->GetPropertyName() != sRootPropertyName)
          return false;
        return true; });

      auto WriteObject = [&](void* pLeafObject, const ezRTTI& leafType, const ezAbstractProperty* pLeafProp, const ezVariant& index)
      {
        pNativeRootObject = pLeafObject;
        context.RegisterObject(pObject->GetGuid(), &leafType, pLeafObject);
        pPrefabSubRoot = rttiConverter.AddObjectToGraph(&leafType, pLeafObject);
        pPrefabSubRoot->RenameProperty(sRootPropertyName, pProp->GetPropertyName());
      };

      ezVariant defaultValue;
      ezResult res = propertyPath.ReadProperty(const_cast<ezReflectedClass*>(pMeta), *pMeta->GetDynamicRTTI(), WriteObject);
      if (res.Failed())
      {
        return ezStatus(ezFmt("Failed to find root object in object graph"));
      }
    }

    // Create graph from current object with only the container to be reverted present.
    ezAbstractObjectGraph instanceSubGraph;
    ezAbstractObjectNode* pInstanceSubRoot = nullptr;
    {
      ezDocumentObjectConverterWriter writer(&instanceSubGraph, pObject->GetDocumentObjectManager(), [pRootObject = pObject, pRootProp = pProp](const ezDocumentObject* pObject, const ezAbstractProperty* pProp)
        {
        if (pObject == pRootObject && pProp != pRootProp)
          return false;
        return true; });
      pInstanceSubRoot = writer.AddObjectToGraph(pObject);
    }

    // Make the native graph match the guids of pObject graph.
    pPrefabSubRoot->SetType(pInstanceSubRoot->GetType());
    prefabSubGraph.ReMapNodeGuidsToMatchGraph(pPrefabSubRoot, instanceSubGraph, pInstanceSubRoot);
    prefabSubGraph.CreateDiffWithBaseGraph(instanceSubGraph, out_diff);
    return ezStatus(EZ_SUCCESS);
  }
  return superPtr[0]->CreateRevertContainerDiff(superPtr.GetSubArray(1), pAccessor, pObject, pProp, out_diff);
}
