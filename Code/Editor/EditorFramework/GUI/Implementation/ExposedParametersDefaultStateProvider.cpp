#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/GUI/ExposedParametersDefaultStateProvider.h>
#include <EditorFramework/PropertyGrid/ExposedParametersPropertyWidget.moc.h>
#include <Foundation/Reflection/Implementation/PropertyAttributes.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Reflection/VariantStorageAccessor.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

ezSharedPtr<ezDefaultStateProvider> ezExposedParametersDefaultStateProvider::CreateProvider(ezObjectAccessorBase* pAccessor, const ezDocumentObject* pObject, const ezAbstractProperty* pProp)
{
  if (pProp)
  {
    const auto* pAttrib = pProp->GetAttributeByType<ezExposedParametersAttribute>();
    if (pAttrib)
    {
      return EZ_DEFAULT_NEW(ezExposedParametersDefaultStateProvider, pAccessor, pObject, pProp);
    }
  }
  return nullptr;
}

ezExposedParametersDefaultStateProvider::ezExposedParametersDefaultStateProvider(ezObjectAccessorBase* pAccessor, const ezDocumentObject* pObject, const ezAbstractProperty* pProp)
  : m_pObject(pObject)
  , m_pProp(pProp)
{
  EZ_ASSERT_DEBUG(pProp->GetCategory() == ezPropertyCategory::Map, "ezExposedParametersAttribute must be on a map property");
  m_pAttrib = pProp->GetAttributeByType<ezExposedParametersAttribute>();
  EZ_ASSERT_DEBUG(m_pAttrib, "ezExposedParametersDefaultStateProvider was created for a property that does not have the ezExposedParametersAttribute.");
  m_pParameterSourceProp = pObject->GetType()->FindPropertyByName(m_pAttrib->GetParametersSource());
  EZ_ASSERT_DEBUG(
    m_pParameterSourceProp, "The exposed parameter source '{0}' does not exist on type '{1}'", m_pAttrib->GetParametersSource(), pObject->GetType()->GetTypeName());
}

ezInt32 ezExposedParametersDefaultStateProvider::GetRootDepth() const
{
  return 0;
}

ezColorGammaUB ezExposedParametersDefaultStateProvider::GetBackgroundColor() const
{
  // Set alpha to 0 -> color will be ignored.
  return ezColorGammaUB(0, 0, 0, 0);
}

ezVariant ezExposedParametersDefaultStateProvider::GetDefaultValue(SuperArray superPtr, ezObjectAccessorBase* pAccessor, const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index)
{
  EZ_ASSERT_DEBUG(pObject == m_pObject && pProp == m_pProp, "ezDefaultContainerState is only valid on the object and container it was created on.");
  ezExposedParameterCommandAccessor accessor(pAccessor, pProp, m_pParameterSourceProp);
  if (index.IsValid())
  {
    if (index.IsA<ezString>())
    {
      const ezExposedParameter* pParam = accessor.GetExposedParam(pObject, index.Get<ezString>());
      if (pParam)
      {
        return pParam->m_DefaultValue;
      }
    }
    return superPtr[0]->GetDefaultValue(superPtr.GetSubArray(1), pAccessor, pObject, pProp, index);
  }
  else
  {
    ezVariantDictionary defaultDict;
    if (const ezExposedParameters* pParams = accessor.GetExposedParams(pObject))
    {
      for (ezExposedParameter* pParam : pParams->m_Parameters)
      {
        defaultDict.Insert(pParam->m_sName, pParam->m_DefaultValue);
      }
    }
    return defaultDict;
  }
}

ezStatus ezExposedParametersDefaultStateProvider::CreateRevertContainerDiff(SuperArray superPtr, ezObjectAccessorBase* pAccessor, const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezDeque<ezAbstractGraphDiffOperation>& out_diff)
{
  EZ_REPORT_FAILURE("Unreachable code");
  return ezStatus(EZ_SUCCESS);
}

bool ezExposedParametersDefaultStateProvider::IsDefaultValue(SuperArray superPtr, ezObjectAccessorBase* pAccessor, const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index)
{
  EZ_ASSERT_DEBUG(pObject == m_pObject && pProp == m_pProp, "ezDefaultContainerState is only valid on the object and container it was created on.");
  ezExposedParameterCommandAccessor accessor(pAccessor, pProp, m_pParameterSourceProp);

  const ezVariant def = GetDefaultValue(superPtr, pAccessor, pObject, pProp, index);
  if (index.IsValid())
  {
    ezVariant value;
    ezStatus res = accessor.GetValue(pObject, pProp, value, index);
    // If the key is not valid, the exposed parameter is not overwritten and thus remains at the default value.
    return res.Failed() || def == value;
  }
  else
  {
    // We consider an exposed params map to be the default if it is empty.
    // We deliberately do not use the accessor here and go directly to the object storage as the passed in pAccessor could already be an ezExposedParameterCommandAccessor in which case we wouldn't truly know if anything was overwritten.
    ezVariant value = pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName(), index);
    return value.Get<ezVariantDictionary>().GetCount() == 0;
  }
}

ezStatus ezExposedParametersDefaultStateProvider::RevertProperty(SuperArray superPtr, ezObjectAccessorBase* pAccessor, const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index)
{
  if (!index.IsValid())
  {
    // We override the standard implementation here to just clear the array on revert to default. This is because the exposed params work as an override of the default behavior and we can safe space and time by simply not overriding anything.
    // The GUI will take care of pretending that the values are present with their default value.
    ezDeque<ezAbstractGraphDiffOperation> diff;
    auto& op = diff.ExpandAndGetRef();
    op.m_Node = pObject->GetGuid();
    op.m_Operation = ezAbstractGraphDiffOperation::Op::PropertyChanged;
    op.m_sProperty = pProp->GetPropertyName();
    op.m_uiTypeVersion = 0;
    op.m_Value = ezVariantDictionary();
    ezDocumentObjectConverterReader::ApplyDiffToObject(pAccessor, pObject, diff);
    return ezStatus(EZ_SUCCESS);
  }
  return ezDefaultStateProvider::RevertProperty(superPtr, pAccessor, pObject, pProp, index);
}

///////////////////////////////////////////////////////////////////////////////

ezSharedPtr<ezDefaultStateProvider> ezExposedParametersAsTypeDefaultStateProvider::CreateProvider(ezObjectAccessorBase* pAccessor, const ezDocumentObject* pObject, const ezAbstractProperty* pProp)
{
  if (auto pExposedParameterCommandAccessor = ezDynamicCast<ezExposedParametersAsTypeCommandAccessor*>(pAccessor))
  {
    return EZ_DEFAULT_NEW(ezExposedParametersAsTypeDefaultStateProvider, pExposedParameterCommandAccessor, pObject, pProp);
  }
  return nullptr;
}

ezExposedParametersAsTypeDefaultStateProvider::ezExposedParametersAsTypeDefaultStateProvider(ezExposedParametersAsTypeCommandAccessor* pAccessor, const ezDocumentObject* pObject, const ezAbstractProperty* pProp)
  : ezExposedParametersDefaultStateProvider(pAccessor->GetSourceAccessor(), pObject, pAccessor->GetSourceAccessor()->m_pParameterProp)
  , m_pAccessor(pAccessor)
{
}

ezVariant ezExposedParametersAsTypeDefaultStateProvider::GetDefaultValue(SuperArray superPtr, ezObjectAccessorBase* pAccessor, const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index)
{
  ezVariant defaultValue;
  if (GetDefaultValueInternal(superPtr, pAccessor, pObject, pProp, index, defaultValue).Succeeded())
    return defaultValue;

  return {};
}

ezStatus ezExposedParametersAsTypeDefaultStateProvider::CreateRevertContainerDiff(SuperArray superPtr, ezObjectAccessorBase* pAccessor, const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezDeque<ezAbstractGraphDiffOperation>& out_diff)
{
  EZ_REPORT_FAILURE("Unreachable code");
  return ezStatus(EZ_SUCCESS);
}

bool ezExposedParametersAsTypeDefaultStateProvider::IsDefaultValue(ezDefaultStateProvider::SuperArray superPtr, ezObjectAccessorBase* pAccessor, const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index)
{
  ezVariant defaultValue;
  if (GetDefaultValueInternal(superPtr, pAccessor, pObject, pProp, index, defaultValue).Failed())
    return true;

  ezVariant value;
  pAccessor->GetValue(pObject, pProp, value, index).LogFailure();
  return defaultValue == value;
}

ezStatus ezExposedParametersAsTypeDefaultStateProvider::RevertProperty(ezDefaultStateProvider::SuperArray superPtr, ezObjectAccessorBase* pAccessor, const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index)
{
  ezVariant defaultValue;
  if (GetDefaultValueInternal(superPtr, pAccessor, pObject, pProp, index, defaultValue).Failed())
    return ezStatus(ezFmt("Failed to retrieve default value for exposed parameter."));

  return pAccessor->SetValue(pObject, pProp, defaultValue, index);
}

ezResult ezExposedParametersAsTypeDefaultStateProvider::GetDefaultValueInternal(ezDefaultStateProvider::SuperArray superPtr, ezObjectAccessorBase* pAccessor, const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index, ezVariant& out_DefaultValue)
{
  // As we derive from ezExposedParametersDefaultStateProvider, we first need to convert the exposed parameter type, prop, accessor into the actual underlying data structure that the base class expects before calling it.
  // * The ezExposedParametersAsTypeCommandAccessor proxies the ezExposedParameterCommandAccessor so the proxy source is the correct accessor.
  // * The object stays the same.
  // * The property from the exposed parameter type is replaced by the actual property that stores the exposed parameters in the real object.
  // * The index is the name of the property as that is how the parameter map is generated (keyed by parameter name).
  // With these changes made, we can rely on the base class to compute the default value.
  out_DefaultValue = ezExposedParametersDefaultStateProvider::GetDefaultValue(superPtr.GetSubArray(1), m_pAccessor->GetSourceAccessor(), pObject, m_pAccessor->GetSourceAccessor()->m_pParameterProp, pProp->GetPropertyName());

  ezStatus res(EZ_SUCCESS);
  // We now have the value of the exposed parameter. If this is a container, we need to dive into the index. If index is invalid, this is a no-op.
  out_DefaultValue = ezVariantStorageAccessor(pProp->GetPropertyName(), out_DefaultValue).GetValue(index, &res);
  if (res.Failed())
    return EZ_FAILURE;

  return EZ_SUCCESS;
}
