#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCheckRule.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAssetCheckRule, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezObjectAccessorBase* ezAssetCheckContext::GetObjectAccessor() const
{
  return m_pDocument ? m_pDocument->GetObjectAccessor() : nullptr;
}

void ezAssetCheckContext::ReportIssue(ezAssetCheckSeverity::Enum severity, ezStringView sMessage, const ezDocumentObject* pObject /*= nullptr*/, bool bFixed /*= false*/)
{
  ezAssetCheckNote& note = m_pNotes->ExpandAndGetRef();
  note.m_Severity = severity;
  note.m_sMessage = sMessage;
  note.m_bFixed = bFixed;

  if (pObject != nullptr)
  {
    note.m_ObjectGuid = pObject->GetGuid();
  }

  if (bFixed)
  {
    ++m_uiFixCount;
  }
}

ezString ezAssetCheckContext::GetObjectDisplayName(const ezDocumentObject* pObject)
{
  if (pObject == nullptr)
    return ezString();

  const ezVariant name = pObject->GetTypeAccessor().GetValue("Name");
  if (name.IsValid() && name.CanConvertTo<ezString>())
  {
    const ezString sName = name.ConvertTo<ezString>();
    if (!sName.IsEmpty())
      return sName;
  }

  return pObject->GetType()->GetTypeName();
}

void ezAssetCheckRule::CheckDocument(ezAssetCheckContext& ref_ctx)
{
  const ezDocumentObject* pRoot = ref_ctx.GetDocument()->GetObjectManager()->GetRootObject();
  VisitObjectRecursive(ref_ctx, pRoot);
}

void ezAssetCheckRule::VisitObjectRecursive(ezAssetCheckContext& ref_ctx, const ezDocumentObject* pObject)
{
  CheckObject(ref_ctx, pObject);

  for (const ezDocumentObject* pChild : pObject->GetChildren())
  {
    if (pChild->GetParentPropertyType() != nullptr &&
        pChild->GetParentPropertyType()->GetAttributeByType<ezTemporaryAttribute>() != nullptr)
      continue;

    VisitObjectRecursive(ref_ctx, pChild);
  }
}

void ezAssetCheckRule::CheckObject(ezAssetCheckContext& ref_ctx, const ezDocumentObject* pObject)
{
  ezHybridArray<const ezAbstractProperty*, 32> properties;
  pObject->GetType()->GetAllProperties(properties);

  for (const ezAbstractProperty* pProp : properties)
  {
    if (pProp->GetAttributeByType<ezTemporaryAttribute>() != nullptr)
      continue;

    CheckProperty(ref_ctx, pObject, pProp);
  }
}

void ezAssetCheckRule::CheckProperty(ezAssetCheckContext& ref_ctx, const ezDocumentObject* pObject, const ezAbstractProperty* pProp)
{
}

ezResult ezAssetCheckRule::GetPropertyValues(ezObjectAccessorBase* pAcc, const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezDynamicArray<ezVariant>& out_indices, ezDynamicArray<ezVariant>& out_values)
{
  out_indices.Clear();
  out_values.Clear();

  switch (pProp->GetCategory())
  {
    case ezPropertyCategory::Member:
    {
      ezVariant value;
      if (pAcc->GetValue(pObject, pProp, value).Failed())
        return EZ_FAILURE;

      out_indices.PushBack(ezVariant());
      out_values.PushBack(value);
      return EZ_SUCCESS;
    }

    case ezPropertyCategory::Array:
    case ezPropertyCategory::Set:
    {
      if (pAcc->GetValues(pObject, pProp, out_values).Failed())
        return EZ_FAILURE;

      out_indices.Reserve(out_values.GetCount());
      for (ezUInt32 i = 0; i < out_values.GetCount(); ++i)
        out_indices.PushBack(i);
      return EZ_SUCCESS;
    }

    case ezPropertyCategory::Map:
    {
      if (pAcc->GetKeys(pObject, pProp, out_indices).Failed())
        return EZ_FAILURE;

      out_values.Reserve(out_indices.GetCount());
      for (const ezVariant& key : out_indices)
      {
        ezVariant value;
        if (pAcc->GetValue(pObject, pProp, value, key).Failed())
          return EZ_FAILURE;

        out_values.PushBack(value);
      }
      return EZ_SUCCESS;
    }

    default:
      return EZ_FAILURE;
  }
}

void ezAssetCheckRule::CreateRules(ezDynamicArray<ezAssetCheckRule*>& out_rules)
{
  ezRTTI::ForEachDerivedType<ezAssetCheckRule>(
    [&](const ezRTTI* pRtti)
    {
      out_rules.PushBack(pRtti->GetAllocator()->Allocate<ezAssetCheckRule>());
    },
    ezRTTI::ForEachOptions::ExcludeNonAllocatable);

  out_rules.Sort([](ezAssetCheckRule* lhs, ezAssetCheckRule* rhs) -> bool
    {
      return lhs->GetDisplayName().Compare_NoCase(rhs->GetDisplayName()) < 0; //
    });
}

void ezAssetCheckRule::DestroyRules(ezDynamicArray<ezAssetCheckRule*>& ref_rules)
{
  for (ezAssetCheckRule* pRule : ref_rules)
  {
    pRule->GetDynamicRTTI()->GetAllocator()->Deallocate(pRule);
  }

  ref_rules.Clear();
}
