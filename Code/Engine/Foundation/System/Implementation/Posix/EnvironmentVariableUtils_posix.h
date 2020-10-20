#include <Foundation/FoundationInternal.h>
EZ_FOUNDATION_INTERNAL_HEADER

#include <stdlib.h>

ezString ezEnvironmentVariableUtils::GetValueStringImpl(const char* szName, const char* szDefault)
{
  const char* value = getenv(szName);
  return value != nullptr ? value : szDefault;
}

ezResult ezEnvironmentVariableUtils::SetValueStringImpl(const char* szName, const char* szValue)
{
  if (setenv(szName, szValue, 1) == 0)
    return EZ_SUCCESS;
  else
    return EZ_FAILURE;
}

bool ezEnvironmentVariableUtils::IsVariableSetImpl(const char* szName)
{
  return getenv(szName) != nullptr;
}

ezResult ezEnvironmentVariableUtils::UnsetVariableImpl(const char* szName)
{
  if (unsetenv(szName) == 0)
    return EZ_SUCCESS;
  else
    return EZ_FAILURE;
}
