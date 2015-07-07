#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Serialization/ObjectSerializationHelper.h>
#include <ToolsFoundation/Serialization/ObjectSerializationContext.h>
#include <ToolsFoundation/Reflection/IReflectedTypeAccessor.h>

static void WriteTypeAccessorToContextRecursive(ezPropertySerializationContext& context, const ezIReflectedTypeAccessor& et, const ezRTTI* pType, ezPropertyPath& ParentPath)
{
  const ezRTTI* pParentType = pType->GetParentType();
  if (pParentType != nullptr)
    WriteTypeAccessorToContextRecursive(context, et, pParentType, ParentPath);

  if (pType->GetProperties().GetCount() == 0)
    return;

  for (ezUInt32 i = 0; i < pType->GetProperties().GetCount(); ++i)
  {
    const ezAbstractProperty* pProp = pType->GetProperties()[i];

    if (pProp->GetFlags().IsAnySet(ezPropertyFlags::StandardType))
    {
      ParentPath.PushBack(pProp->GetPropertyName());

      context.AddProperty(ParentPath, et.GetValue(ParentPath));

      ParentPath.PopBack();
    }
    else if (pProp->GetFlags().IsAnySet(ezPropertyFlags::IsEnum | ezPropertyFlags::Bitflags))
    {
      ParentPath.PushBack(pProp->GetPropertyName());

      ezStringBuilder sEnumValue;
      ezReflectionUtils::EnumerationToString(pProp->GetSpecificType(), et.GetValue(ParentPath).ConvertTo<ezInt64>(), sEnumValue);
      context.AddProperty(ParentPath, sEnumValue.GetData());

      ParentPath.PopBack();
    }
    else if (pProp->GetCategory() == ezPropertyCategory::Member)
    {
      ParentPath.PushBack(pProp->GetPropertyName());

      WriteTypeAccessorToContextRecursive(context, et, pProp->GetSpecificType(), ParentPath);

      ParentPath.PopBack();
    }
  }
}

void ezObjectSerializationHelper::WriteTypeAccessorToContext(const ezIReflectedTypeAccessor& accessor, ezPropertySerializationContext& context)
{
  const ezRTTI* pType = accessor.GetType();
  ezPropertyPath parentPath;
  WriteTypeAccessorToContextRecursive(context, accessor, pType, parentPath);
}

