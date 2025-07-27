#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/PropertyGrid/VariantSubDefaultStateProvider.h>

#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Object/VariantSubAccessor.h>
#include <ToolsFoundation/Reflection/VariantStorageAccessor.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

ezSharedPtr<ezDefaultStateProvider> ezVariantSubDefaultStateProvider::CreateProvider(ezObjectAccessorBase* pAccessor, const ezDocumentObject* pObject, const ezAbstractProperty* pProp)
{
  if (auto variantSubAccessor = ezDynamicCast<ezVariantSubAccessor*>(pAccessor))
  {
    if (variantSubAccessor->GetRootProperty() == pProp)
      return EZ_DEFAULT_NEW(ezVariantSubDefaultStateProvider, variantSubAccessor, pObject, pProp);
  }
  return nullptr;
}

ezVariantSubDefaultStateProvider::ezVariantSubDefaultStateProvider(ezVariantSubAccessor* pAccessor, const ezDocumentObject* pObject, const ezAbstractProperty* pProp)
  : m_pAccessor(pAccessor)
  , m_pObject(pObject)
  , m_pProp(pProp)
{
  m_pRootAccessor = m_pAccessor->GetSourceAccessor();
  while (auto variantSubAccessor = ezDynamicCast<ezVariantSubAccessor*>(m_pRootAccessor))
  {
    m_pRootAccessor = variantSubAccessor->GetSourceAccessor();
  }
}

ezInt32 ezVariantSubDefaultStateProvider::GetRootDepth() const
{
  // As this default provider dives into the contents of a variable it has to always be executed first as all the other providers work on property granularity.
  return 1000;
}

ezColorGammaUB ezVariantSubDefaultStateProvider::GetBackgroundColor() const
{
  // Set alpha to 0 -> color will be ignored.
  return ezColorGammaUB(0, 0, 0, 0);
}

ezVariant ezVariantSubDefaultStateProvider::GetDefaultValue(SuperArray superPtr, ezObjectAccessorBase* pAccessor, const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index)
{
  ezVariant defaultValue;
  if (GetDefaultValueInternal(superPtr, pAccessor, pObject, pProp, index, defaultValue).Succeeded())
    return defaultValue;

  return {};
}

ezStatus ezVariantSubDefaultStateProvider::CreateRevertContainerDiff(SuperArray superPtr, ezObjectAccessorBase* pAccessor, const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezDeque<ezAbstractGraphDiffOperation>& out_diff)
{
  EZ_REPORT_FAILURE("Unreachable code");
  return ezStatus(EZ_SUCCESS);
}

bool ezVariantSubDefaultStateProvider::IsDefaultValue(ezDefaultStateProvider::SuperArray superPtr, ezObjectAccessorBase* pAccessor, const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index)
{
  ezVariant defaultValue;
  if (GetDefaultValueInternal(superPtr, pAccessor, pObject, pProp, index, defaultValue).Failed())
    return true;

  ezVariant value;
  pAccessor->GetValue(pObject, pProp, value, index).LogFailure();
  return defaultValue == value;
}

ezStatus ezVariantSubDefaultStateProvider::RevertProperty(ezDefaultStateProvider::SuperArray superPtr, ezObjectAccessorBase* pAccessor, const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index)
{
  ezVariant defaultValue;
  if (GetDefaultValueInternal(superPtr, pAccessor, pObject, pProp, index, defaultValue).Failed())
    return ezStatus(ezFmt("Failed to retrieve default value for variant sub tree."));

  return pAccessor->SetValue(pObject, pProp, defaultValue, index);
}

ezResult ezVariantSubDefaultStateProvider::GetDefaultValueInternal(ezDefaultStateProvider::SuperArray superPtr, ezObjectAccessorBase* pAccessor, const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index, ezVariant& out_DefaultValue)
{
  EZ_ASSERT_DEBUG(pObject == m_pObject && pProp == m_pProp, "ezVariantSubDefaultStateProvider is only valid on the object and variant property it was created on.");
  // As m_pAccessor is a view into an ezVariant we first need to take the same steps into the defaultValue retrieved from the root accessor to have the same view so we can compare the same subset of both ezVariants.
  out_DefaultValue = superPtr[0]->GetDefaultValue(superPtr.GetSubArray(1), m_pRootAccessor, pObject, pProp);

  ezHybridArray<ezVariant, 4> path;
  EZ_SUCCEED_OR_RETURN(m_pAccessor->GetPath(pObject, path));
  if (index.IsValid())
    path.PushBack(index);

  for (const ezVariant& step : path)
  {
    ezStatus res(EZ_SUCCESS);
    out_DefaultValue = ezVariantStorageAccessor(pProp->GetPropertyName(), out_DefaultValue).GetValue(step, &res);
    if (res.Failed())
      return EZ_FAILURE;
  }
  return EZ_SUCCESS;
}
