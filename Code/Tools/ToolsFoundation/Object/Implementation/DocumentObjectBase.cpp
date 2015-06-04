#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>

ezIReflectedTypeAccessor& ezDocumentObjectBase::GetTypeAccessor()
{
  const ezDocumentObjectBase* pMe = this;
  return const_cast<ezIReflectedTypeAccessor&>(pMe->GetTypeAccessor());
}

ezIReflectedTypeAccessor& ezDocumentObjectBase::GetEditorTypeAccessor()
{
  const ezDocumentObjectBase* pMe = this;
  return const_cast<ezIReflectedTypeAccessor&>(pMe->GetEditorTypeAccessor());
}

ezUInt32 ezDocumentObjectBase::GetChildIndex(ezDocumentObjectBase* pChild) const
{
  return m_Children.IndexOf(pChild);
}

void ezDocumentObjectBase::ComputeObjectHash(ezUInt64& uiHash) const
{
  {
    const ezIReflectedTypeAccessor& acc = GetEditorTypeAccessor();
    auto pType = acc.GetType();
    ezPropertyPath path;
    HashPropertiesRecursive(acc, uiHash, pType, path);
  }

  {
    const ezIReflectedTypeAccessor& acc = GetTypeAccessor();
    auto pType = acc.GetType();
    ezPropertyPath path;
    HashPropertiesRecursive(acc, uiHash, pType, path);
  }
}


void ezDocumentObjectBase::HashPropertiesRecursive(const ezIReflectedTypeAccessor& acc, ezUInt64& uiHash, const ezRTTI* pType, ezPropertyPath& path) const
{
  // Parse parent class
  const ezRTTI* pParentType = pType->GetParentType();
  if (pParentType != nullptr)
    HashPropertiesRecursive(acc, uiHash, pParentType, path);

  // Parse properties
  ezUInt32 uiPropertyCount = pType->GetProperties().GetCount();
  for (ezUInt32 i = 0; i < uiPropertyCount; ++i)
  {
    const ezAbstractProperty* pProperty = pType->GetProperties()[i];

    if (pProperty->GetFlags().IsSet(ezPropertyFlags::ReadOnly))
      continue;

    // Build property path
    path.PushBack(pProperty->GetPropertyName());

    if (pProperty->GetFlags().IsSet(ezPropertyFlags::StandardType))
    {
      ezVariant var = acc.GetValue(path);
      uiHash = var.ComputeHash(uiHash);
    }
    else if (pProperty->GetFlags().IsAnySet(ezPropertyFlags::IsEnum | ezPropertyFlags::Bitflags))
    {
      ezVariant var = acc.GetValue(path);
      uiHash = var.ComputeHash(uiHash);
    }
    else if (pProperty->GetCategory() == ezPropertyCategory::Member)
    {
      const ezAbstractMemberProperty* pMember = static_cast<const ezAbstractMemberProperty*>(pProperty);

      // Not POD type, recurse further
      HashPropertiesRecursive(acc, uiHash, pMember->GetPropertyType(), path);
    }

    path.PopBack();
  }
}

ezDocumentSubObject::ezDocumentSubObject(const ezRTTI* pRtti)
  : m_TypeAccessor(pRtti)
  , m_EditorTypeAccessor(pRtti)
{
  m_pOwnerObject = nullptr;
}

void ezDocumentSubObject::SetObject(ezDocumentObjectBase* pOwnerObject, const ezPropertyPath& subPath)
{
  m_pOwnerObject = pOwnerObject;
  m_SubPath = subPath;

  m_TypeAccessor.SetSubAccessor(&pOwnerObject->GetTypeAccessor(), subPath);
  m_EditorTypeAccessor.SetSubAccessor(&pOwnerObject->GetEditorTypeAccessor(), subPath);
}
