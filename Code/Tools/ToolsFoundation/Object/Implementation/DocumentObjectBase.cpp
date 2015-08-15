#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEmptyProperties, ezReflectedClass, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

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


const ezIReflectedTypeAccessor& ezDocumentObjectBase::GetParentAccessor() const
{
  return m_bEditorProperty ? GetParent()->GetEditorTypeAccessor() : GetParent()->GetTypeAccessor();
}

ezUInt32 ezDocumentObjectBase::GetChildIndex(ezDocumentObjectBase* pChild) const
{
  return m_Children.IndexOf(pChild);
}

void ezDocumentObjectBase::InsertSubObject(ezDocumentObjectBase* pObject, const char* szProperty, const ezVariant& index, bool bEditorProperty)
{
  EZ_ASSERT_DEV(pObject != nullptr, "");
  EZ_ASSERT_DEV(!ezStringUtils::IsNullOrEmpty(szProperty), "Child objects must have a parent property to insert into");
  ezIReflectedTypeAccessor& accessor = bEditorProperty ? GetEditorTypeAccessor() : GetTypeAccessor();


  // Property patching
  const ezRTTI* pType = accessor.GetType();
  auto* pProp = pType->FindPropertyByName(szProperty);
  EZ_ASSERT_DEV(!pProp->GetFlags().IsSet(ezPropertyFlags::Pointer) || pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner), "");
  EZ_ASSERT_DEV(pProp->GetFlags().IsSet(ezPropertyFlags::Pointer) == (pObject->GetObjectType() == ezDocumentObjectType::Object)
    || (pObject->GetObjectType() == ezDocumentObjectType::ArrayElement) == (pProp->GetCategory() == ezPropertyCategory::Array)
    || (pObject->GetObjectType() == ezDocumentObjectType::SetElement) == (pProp->GetCategory() == ezPropertyCategory::Set), "");
  ezPropertyPath path(szProperty);
  if (pProp->GetCategory() == ezPropertyCategory::Array || pProp->GetCategory() == ezPropertyCategory::Set)
  {
    bool bRes = accessor.InsertValue(path, index, pObject->GetGuid());
    EZ_ASSERT_DEV(bRes, "");
  }
  else if (pProp->GetCategory() == ezPropertyCategory::Member)
  {
    bool bRes = accessor.SetValue(path, pObject->GetGuid());
    EZ_ASSERT_DEV(bRes, "");
  }

  // Object patching
  pObject->m_bEditorProperty = bEditorProperty;
  pObject->m_sParentProperty = szProperty;
  pObject->m_pParent = this;
  m_Children.PushBack(pObject);
}

void ezDocumentObjectBase::RemoveSubObject(ezDocumentObjectBase* pObject)
{
  EZ_ASSERT_DEV(pObject != nullptr, "");
  EZ_ASSERT_DEV(!pObject->m_sParentProperty.IsEmpty(), "");
  EZ_ASSERT_DEV(this == pObject->m_pParent, "");
  ezIReflectedTypeAccessor& accessor = pObject->IsEditorProperty() ? GetEditorTypeAccessor() : GetTypeAccessor();

  // Property patching
  const ezRTTI* pType = accessor.GetType();
  auto* pProp = pType->FindPropertyByName(pObject->m_sParentProperty);
  ezPropertyPath path(pObject->m_sParentProperty);
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

ezVariant ezDocumentObjectBase::GetPropertyIndex() const
{
  if (m_pParent == nullptr)
    return ezVariant();
  const ezIReflectedTypeAccessor& accessor = m_bEditorProperty ? m_pParent->GetEditorTypeAccessor() : m_pParent->GetTypeAccessor();
  return accessor.GetPropertyChildIndex(m_sParentProperty.GetData(), GetGuid());
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
  m_ObjectType = ezDocumentObjectType::SubObject;
}

void ezDocumentSubObject::SetObject(ezDocumentObjectBase* pOwnerObject, const ezPropertyPath& subPath, bool bEditorProperty)
{
  ezAbstractProperty* pProp = ezToolsReflectionUtils::GetPropertyByPath(pOwnerObject->GetTypeAccessor().GetType(), subPath);
  EZ_ASSERT_DEV(pProp != nullptr && pProp->GetSpecificType() == m_Accessor.GetType(), "ezDocumentSubObject was created for a different type it is mapped to!");

  m_pParent = pOwnerObject;
  m_SubPath = subPath;
  m_bEditorProperty = bEditorProperty;
  m_pDocumentObjectManager = pOwnerObject->GetDocumentObjectManager();
  m_Accessor.SetSubAccessor(bEditorProperty ? &pOwnerObject->GetEditorTypeAccessor() : &pOwnerObject->GetTypeAccessor(), subPath);
}
