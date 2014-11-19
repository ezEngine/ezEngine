#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Serialization/ObjectSerializationHelper.h>
#include <ToolsFoundation/Serialization/ObjectSerializationContext.h>
#include <ToolsFoundation/Reflection/IReflectedTypeAccessor.h>

static void WriteTypeAccessorToContextRecursive(ezObjectSerializationContext& context, const ezIReflectedTypeAccessor& et, const ezReflectedType* pType, ezPropertyPath& ParentPath)
{
  ezReflectedTypeHandle hParent = pType->GetParentTypeHandle();
  if (!hParent.IsInvalidated())
    WriteTypeAccessorToContextRecursive(context, et, hParent.GetType(), ParentPath);

  if (pType->GetPropertyCount() == 0)
    return;

  for (ezUInt32 i = 0; i < pType->GetPropertyCount(); ++i)
  {
    const ezReflectedProperty* pProp = pType->GetPropertyByIndex(i);

    if (pProp->m_Flags.IsAnySet(PropertyFlags::IsPOD))
    {
      ParentPath.PushBack(pProp->m_sPropertyName.GetString().GetData());

      context.AddProperty(ParentPath, et.GetValue(ParentPath));

      ParentPath.PopBack();
    }
    else
    {
      ParentPath.PushBack(pProp->m_sPropertyName.GetString().GetData());

      WriteTypeAccessorToContextRecursive(context, et, pProp->m_hTypeHandle.GetType(), ParentPath);

      ParentPath.PopBack();
    }
  }
}

void ezObjectSerializationHelper::WriteTypeAccessorToContext(const ezIReflectedTypeAccessor& accessor, ezObjectSerializationContext& context)
{
  const ezReflectedType* pType = accessor.GetReflectedTypeHandle().GetType();
  ezPropertyPath parentPath;
  WriteTypeAccessorToContextRecursive(context, accessor, pType, parentPath);
}

