#include <Foundation/FoundationInternal.h>
EZ_FOUNDATION_INTERNAL_HEADER

#include <Foundation/System/EnvironmentVariableUtils.h>

ezString ezEnvironmentVariableUtils::GetValueStringImpl(ezStringView sName, ezStringView sDefault)
{
  EZ_IGNORE_UNUSED(sName);
  EZ_IGNORE_UNUSED(sDefault);
  EZ_ASSERT_NOT_IMPLEMENTED
  return "";
}

ezResult ezEnvironmentVariableUtils::SetValueStringImpl(ezStringView sName, ezStringView sValue)
{
  EZ_IGNORE_UNUSED(sName);
  EZ_IGNORE_UNUSED(sValue);
  EZ_ASSERT_NOT_IMPLEMENTED
  return EZ_FAILURE;
}

bool ezEnvironmentVariableUtils::IsVariableSetImpl(ezStringView sName)
{
  EZ_IGNORE_UNUSED(sName);
  return false;
}

ezResult ezEnvironmentVariableUtils::UnsetVariableImpl(ezStringView sName)
{
  EZ_IGNORE_UNUSED(sName);
  return EZ_FAILURE;
}
