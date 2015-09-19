#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

ezIReflectedTypeAccessor& ezDocumentObjectBase::GetTypeAccessor()
{
  const ezDocumentObjectBase* pMe = this;
  return const_cast<ezIReflectedTypeAccessor&>(pMe->GetTypeAccessor());
}

ezUInt32 ezDocumentObjectBase::GetChildIndex(ezDocumentObjectBase* pChild) const
{
  return m_Children.IndexOf(pChild);
}

void ezDocumentObjectBase::InsertSubObject(ezDocumentObjectBase* pObject, const char* szProperty, const ezVariant& index)
{
  EZ_ASSERT_DEV(pObject != nullptr, "");
  EZ_ASSERT_DEV(!ezStringUtils::IsNullOrEmpty(szProperty), "Child objects must have a parent property to insert into");
  ezIReflectedTypeAccessor& accessor = GetTypeAccessor();

  // Property patching
  const ezRTTI* pType = accessor.GetType();
  ezPropertyPath path(szProperty);
  auto* pProp = ezToolsReflectionUtils::GetPropertyByPath(pType, path);
  EZ_ASSERT_DEV(!pProp->GetFlags().IsSet(ezPropertyFlags::Pointer) || pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner), "");
  
  if (pProp->GetCategory() == ezPropertyCategory::Array || pProp->GetCategory() == ezPropertyCategory::Set)
  {
    if (index.CanConvertTo<ezInt32>() && index.ConvertTo<ezInt32>() == -1)
    {
      ezVariant newIndex = accessor.GetCount(path);
      bool bRes = accessor.InsertValue(path, newIndex, pObject->GetGuid());
      EZ_ASSERT_DEV(bRes, "");
    }
    else
    {
      bool bRes = accessor.InsertValue(path, index, pObject->GetGuid());
      EZ_ASSERT_DEV(bRes, "");
    }
  }
  else if (pProp->GetCategory() == ezPropertyCategory::Member)
  {
    bool bRes = accessor.SetValue(path, pObject->GetGuid());
    EZ_ASSERT_DEV(bRes, "");
  }

  // Object patching
  pObject->m_sParentProperty = szProperty;
  pObject->m_pParent = this;
  m_Children.PushBack(pObject);
}

void ezDocumentObjectBase::RemoveSubObject(ezDocumentObjectBase* pObject)
{
  EZ_ASSERT_DEV(pObject != nullptr, "");
  EZ_ASSERT_DEV(!pObject->m_sParentProperty.IsEmpty(), "");
  EZ_ASSERT_DEV(this == pObject->m_pParent, "");
  ezIReflectedTypeAccessor& accessor = GetTypeAccessor();

  // Property patching
  const ezRTTI* pType = accessor.GetType();
  ezPropertyPath path(pObject->m_sParentProperty);
  auto* pProp = ezToolsReflectionUtils::GetPropertyByPath(pType, path);
  if (pProp->GetCategory() == ezPropertyCategory::Array || pProp->GetCategory() == ezPropertyCategory::Set)
  {
    ezVariant index = accessor.GetPropertyChildIndex(path, pObject->GetGuid());
    bool bRes = accessor.RemoveValue(path, index);
    EZ_ASSERT_DEV(bRes, "");
  }
  else if (pProp->GetCategory() == ezPropertyCategory::Member)
  {
    bool bRes = accessor.SetValue(path, ezUuid());
    EZ_ASSERT_DEV(bRes, "");
  }

  m_Children.Remove(pObject);
  pObject->m_pParent = nullptr;
}

void ezDocumentObjectBase::ComputeObjectHash(ezUInt64& uiHash) const
{
  const ezIReflectedTypeAccessor& acc = GetTypeAccessor();
  auto pType = acc.GetType();
  ezPropertyPath path;
  HashPropertiesRecursive(acc, uiHash, pType, path);
}

ezVariant ezDocumentObjectBase::GetPropertyIndex() const
{
  if (m_pParent == nullptr)
    return ezVariant();
  const ezIReflectedTypeAccessor& accessor = m_pParent->GetTypeAccessor();
  return accessor.GetPropertyChildIndex(m_sParentProperty.GetData(), GetGuid());
}

bool ezDocumentObjectBase::IsOnHeap() const
{
  if (GetParent() == GetDocumentObjectManager()->GetRootObject())
    return true;

  const ezRTTI* pRtti = GetParent()->GetTypeAccessor().GetType();

  ezPropertyPath path(m_sParentProperty);
  auto* pProp = ezToolsReflectionUtils::GetPropertyByPath(pRtti, path);
  return pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner);
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
      HashPropertiesRecursive(acc, uiHash, pMember->GetSpecificType(), path);
    }

    path.PopBack();
  }
}


ezDocumentSubObject::ezDocumentSubObject(const ezRTTI* pRtti)
  : m_Accessor(pRtti, this)
{
}

void ezDocumentSubObject::SetObject(ezDocumentObjectBase* pOwnerObject, const ezPropertyPath& subPath, ezUuid guid)
{
  ezAbstractProperty* pProp = ezToolsReflectionUtils::GetPropertyByPath(pOwnerObject->GetTypeAccessor().GetType(), subPath);
  EZ_ASSERT_DEV(pProp != nullptr && pProp->GetSpecificType() == m_Accessor.GetType(), "ezDocumentSubObject was created for a different type it is mapped to!");

  m_Guid = guid;
  m_pParent = pOwnerObject;
  m_SubPath = subPath;
  m_pDocumentObjectManager = pOwnerObject->GetDocumentObjectManager();
  m_Accessor.SetSubAccessor(&pOwnerObject->GetTypeAccessor(), subPath);
}
