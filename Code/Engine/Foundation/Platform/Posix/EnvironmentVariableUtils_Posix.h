#include <Foundation/FoundationInternal.h>
EZ_FOUNDATION_INTERNAL_HEADER

#include <Foundation/System/EnvironmentVariableUtils.h>
#include <stdlib.h>

ezString ezEnvironmentVariableUtils::GetValueStringImpl(ezStringView sName, ezStringView sDefault)
{
  ezStringBuilder tmp;
  const char* value = getenv(sName.GetData(tmp));
  return value != nullptr ? value : sDefault;
}

ezResult ezEnvironmentVariableUtils::SetValueStringImpl(ezStringView sName, ezStringView sValue)
{
  ezStringBuilder tmp, tmp2;
  if (setenv(sName.GetData(tmp), sValue.GetData(tmp2), 1) == 0)
    return EZ_SUCCESS;
  else
    return EZ_FAILURE;
}

bool ezEnvironmentVariableUtils::IsVariableSetImpl(ezStringView sName)
{
  ezStringBuilder tmp;
  return getenv(sName.GetData(tmp)) != nullptr;
}

ezResult ezEnvironmentVariableUtils::UnsetVariableImpl(ezStringView sName)
{
  ezStringBuilder tmp;
  if (unsetenv(sName.GetData(tmp)) == 0)
    return EZ_SUCCESS;
  else
    return EZ_FAILURE;
}
