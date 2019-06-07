#include <ToolsFoundationPCH.h>

#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

ezIReflectedTypeAccessor& ezDocumentObject::GetTypeAccessor()
{
  const ezDocumentObject* pMe = this;
  return const_cast<ezIReflectedTypeAccessor&>(pMe->GetTypeAccessor());
}

ezUInt32 ezDocumentObject::GetChildIndex(const ezDocumentObject* pChild) const
{
  return m_Children.IndexOf(const_cast<ezDocumentObject*>(pChild));
}

void ezDocumentObject::InsertSubObject(ezDocumentObject* pObject, const char* szProperty, const ezVariant& index)
{
  EZ_ASSERT_DEV(pObject != nullptr, "");
  EZ_ASSERT_DEV(!ezStringUtils::IsNullOrEmpty(szProperty), "Child objects must have a parent property to insert into");
  ezIReflectedTypeAccessor& accessor = GetTypeAccessor();

  const ezRTTI* pType = accessor.GetType();
  auto* pProp = pType->FindPropertyByName(szProperty);
  EZ_ASSERT_DEV(pProp && pProp->GetFlags().IsSet(ezPropertyFlags::Class) &&
                    (!pProp->GetFlags().IsSet(ezPropertyFlags::Pointer) || pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner)),
                "Only class type or pointer to class type that own the object can be inserted, everything else is handled by value.");

  if (pProp->GetCategory() == ezPropertyCategory::Array || pProp->GetCategory() == ezPropertyCategory::Set)
  {
    if (!index.IsValid() || (index.CanConvertTo<ezInt32>() && index.ConvertTo<ezInt32>() == -1))
    {
      ezVariant newIndex = accessor.GetCount(szProperty);
      bool bRes = accessor.InsertValue(szProperty, newIndex, pObject->GetGuid());
      EZ_ASSERT_DEV(bRes, "");
    }
    else
    {
      bool bRes = accessor.InsertValue(szProperty, index, pObject->GetGuid());
      EZ_ASSERT_DEV(bRes, "");
    }
  }
  else if (pProp->GetCategory() == ezPropertyCategory::Map)
  {
    EZ_ASSERT_DEV(index.IsA<ezString>(), "Map key must be a string.");
    bool bRes = accessor.InsertValue(szProperty, index, pObject->GetGuid());
    EZ_ASSERT_DEV(bRes, "");
  }
  else if (pProp->GetCategory() == ezPropertyCategory::Member)
  {
    bool bRes = accessor.SetValue(szProperty, pObject->GetGuid());
    EZ_ASSERT_DEV(bRes, "");
  }

  // Object patching
  pObject->m_sParentProperty = szProperty;
  pObject->m_pParent = this;
  m_Children.PushBack(pObject);
}

void ezDocumentObject::RemoveSubObject(ezDocumentObject* pObject)
{
  EZ_ASSERT_DEV(pObject != nullptr, "");
  EZ_ASSERT_DEV(!pObject->m_sParentProperty.IsEmpty(), "");
  EZ_ASSERT_DEV(this == pObject->m_pParent, "");
  ezIReflectedTypeAccessor& accessor = GetTypeAccessor();

  // Property patching
  const ezRTTI* pType = accessor.GetType();
  auto* pProp = pType->FindPropertyByName(pObject->m_sParentProperty);
  if (pProp->GetCategory() == ezPropertyCategory::Array || pProp->GetCategory() == ezPropertyCategory::Set ||
      pProp->GetCategory() == ezPropertyCategory::Map)
  {
    ezVariant index = accessor.GetPropertyChildIndex(pObject->m_sParentProperty, pObject->GetGuid());
    bool bRes = accessor.RemoveValue(pObject->m_sParentProperty, index);
    EZ_ASSERT_DEV(bRes, "");
  }
  else if (pProp->GetCategory() == ezPropertyCategory::Member)
  {
    bool bRes = accessor.SetValue(pObject->m_sParentProperty, ezUuid());
    EZ_ASSERT_DEV(bRes, "");
  }

  m_Children.RemoveAndCopy(pObject);
  pObject->m_pParent = nullptr;
}

void ezDocumentObject::ComputeObjectHash(ezUInt64& uiHash) const
{
  const ezIReflectedTypeAccessor& acc = GetTypeAccessor();
  auto pType = acc.GetType();

  uiHash = ezHashingUtils::xxHash64(&m_Guid, sizeof(ezUuid), uiHash);
  HashPropertiesRecursive(acc, uiHash, pType);
}


ezDocumentObject* ezDocumentObject::GetChild(const ezUuid& guid)
{
  for (auto* pChild : m_Children)
  {
    if (pChild->GetGuid() == guid)
      return pChild;
  }
  return nullptr;
}


const ezDocumentObject* ezDocumentObject::GetChild(const ezUuid& guid) const
{
  for (auto* pChild : m_Children)
  {
    if (pChild->GetGuid() == guid)
      return pChild;
  }
  return nullptr;
}

ezAbstractProperty* ezDocumentObject::GetParentPropertyType() const
{
  if (!m_pParent)
    return nullptr;
  const ezIReflectedTypeAccessor& accessor = m_pParent->GetTypeAccessor();
  const ezRTTI* pType = accessor.GetType();
  return pType->FindPropertyByName(m_sParentProperty);
}

ezVariant ezDocumentObject::GetPropertyIndex() const
{
  if (m_pParent == nullptr)
    return ezVariant();
  const ezIReflectedTypeAccessor& accessor = m_pParent->GetTypeAccessor();
  return accessor.GetPropertyChildIndex(m_sParentProperty.GetData(), GetGuid());
}

bool ezDocumentObject::IsOnHeap() const
{
  /// \todo Christopher: This crashes when the pointer is nullptr, which appears to be possible
  /// It happened for me when duplicating (CTRL+D) 2 objects 2 times then moving them and finally undoing everything
  EZ_ASSERT_DEV(m_pParent != nullptr,
                "Object being modified is not part of the document, e.g. may be in the undo stack instead. "
                "This could happen if within an undo / redo op some callback tries to create a new undo scope / update prefabs etc.");

  if (GetParent() == GetDocumentObjectManager()->GetRootObject())
    return true;

  auto* pProp = GetParentPropertyType();
  return pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner);
}


void ezDocumentObject::HashPropertiesRecursive(const ezIReflectedTypeAccessor& acc, ezUInt64& uiHash, const ezRTTI* pType) const
{
  // Parse parent class
  const ezRTTI* pParentType = pType->GetParentType();
  if (pParentType != nullptr)
    HashPropertiesRecursive(acc, uiHash, pParentType);

  // Parse properties
  ezUInt32 uiPropertyCount = pType->GetProperties().GetCount();
  for (ezUInt32 i = 0; i < uiPropertyCount; ++i)
  {
    const ezAbstractProperty* pProperty = pType->GetProperties()[i];

    if (pProperty->GetFlags().IsSet(ezPropertyFlags::ReadOnly))
      continue;

    if (pProperty->GetCategory() == ezPropertyCategory::Member)
    {
      const ezVariant var = acc.GetValue(pProperty->GetPropertyName());
      uiHash = var.ComputeHash(uiHash);
    }
    else if (pProperty->GetCategory() == ezPropertyCategory::Array || pProperty->GetCategory() == ezPropertyCategory::Set)
    {
      ezHybridArray<ezVariant, 16> keys;
      acc.GetValues(pProperty->GetPropertyName(), keys);
      for (const ezVariant& var : keys)
      {
        uiHash = var.ComputeHash(uiHash);
      }
    }
    else if (pProperty->GetCategory() == ezPropertyCategory::Map)
    {
      ezHybridArray<ezVariant, 16> keys;
      acc.GetKeys(pProperty->GetPropertyName(), keys);
      keys.Sort([](const ezVariant& a, const ezVariant& b) { return a.Get<ezString>().Compare(b.Get<ezString>()) < 0; });
      for (const ezVariant& key : keys)
      {
        uiHash = key.ComputeHash(uiHash);
        ezVariant value = acc.GetValue(pProperty->GetPropertyName(), key);
        uiHash = value.ComputeHash(uiHash);
      }
    }
  }
}
