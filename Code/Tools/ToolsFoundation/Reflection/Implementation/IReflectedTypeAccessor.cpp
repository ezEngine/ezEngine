#include <PCH.h>
#include <ToolsFoundation/Reflection/IReflectedTypeAccessor.h>

bool ezIReflectedTypeAccessor::GetValues(const char* szProperty, ezHybridArray<ezVariant, 16>& out_values) const
{
  ezHybridArray<ezVariant, 16> keys;
  if (!GetKeys(szProperty, keys))
    return false;

  out_values.Clear();
  out_values.Reserve(keys.GetCount());
  for (ezVariant key : keys)
  {
    out_values.PushBack(GetValue(szProperty, key));
  }
  return true;
}