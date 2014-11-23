#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Reflection/IReflectedTypeAccessor.h>

const ezVariant ezIReflectedTypeAccessor::GetValue(const char* szPath) const
{
  ezHybridArray<ezString, 6> tempArray;
  ezPropertyPath path;
  ezToolsReflectionUtils::GetPropertyPathFromString(szPath, path, tempArray);
  return GetValue(path);
}

bool ezIReflectedTypeAccessor::SetValue(const char* szPath, const ezVariant& value)
{
  ezHybridArray<ezString, 6> tempArray;
  ezPropertyPath path;
  ezToolsReflectionUtils::GetPropertyPathFromString(szPath, path, tempArray);
  return SetValue(path, value);
}

