#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Reflection/IReflectedTypeAccessor.h>

bool ezIReflectedTypeAccessor::GetValues(ezStringView sProperty, ezDynamicArray<ezVariant>& out_values) const
{
  ezHybridArray<ezVariant, 16> keys;
  if (!GetKeys(sProperty, keys))
    return false;

  out_values.Clear();
  out_values.Reserve(keys.GetCount());
  for (const ezVariant& key : keys)
  {
    out_values.PushBack(GetValue(sProperty, key));
  }
  return true;
}
