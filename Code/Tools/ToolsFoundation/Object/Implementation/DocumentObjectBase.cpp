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
    auto pType = acc.GetReflectedTypeHandle().GetType();
    ezPropertyPath path;
    HashPropertiesRecursive(acc, uiHash, pType, path);
  }

  {
    const ezIReflectedTypeAccessor& acc = GetTypeAccessor();
    auto pType = acc.GetReflectedTypeHandle().GetType();
    ezPropertyPath path;
    HashPropertiesRecursive(acc, uiHash, pType, path);
  }
}


void ezDocumentObjectBase::HashPropertiesRecursive(const ezIReflectedTypeAccessor& acc, ezUInt64& uiHash, const ezReflectedType* pType, ezPropertyPath& path) const
{
  // Parse parent class
  ezReflectedTypeHandle hParent = pType->GetParentTypeHandle();
  if (!hParent.IsInvalidated())
    HashPropertiesRecursive(acc, uiHash, hParent.GetType(), path);
  
  // Parse properties
  ezUInt32 uiPropertyCount = pType->GetPropertyCount();
  for (ezUInt32 i = 0; i < uiPropertyCount; ++i)
  {
    const ezReflectedProperty* pProperty = pType->GetPropertyByIndex(i);

    if (pProperty->m_Flags.IsSet(ezPropertyFlags::ReadOnly))
      continue;

    // Build property path
    path.PushBack(pProperty->m_sPropertyName.GetString().GetData());

    if (pProperty->m_Flags.IsSet(ezPropertyFlags::StandardType))
    {
      ezVariant var = acc.GetValue(path);
      uiHash = var.ComputeHash(uiHash);
    }
    else if (pProperty->m_Flags.IsAnySet(ezPropertyFlags::IsEnum | ezPropertyFlags::Bitflags))
    {
      ezVariant var = acc.GetValue(path);
      uiHash = var.ComputeHash(uiHash);
    }
    else
    {
      // Not POD type, recurse further
      if (pProperty->m_hTypeHandle.IsInvalidated())
      {
        EZ_ASSERT_DEV(false, "A non-POD property was found that cannot be recursed into!");
      }
      else
      {
        HashPropertiesRecursive(acc, uiHash, pProperty->m_hTypeHandle.GetType(), path);
      }
    }

    path.PopBack();
  }
}
