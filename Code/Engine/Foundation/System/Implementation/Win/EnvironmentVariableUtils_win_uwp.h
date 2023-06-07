#include <Foundation/FoundationInternal.h>
EZ_FOUNDATION_INTERNAL_HEADER

ezString ezEnvironmentVariableUtils::GetValueStringImpl(ezStringView sName, ezStringView sDefault)
{
  EZ_ASSERT_NOT_IMPLEMENTED
  return "";
}

ezResult ezEnvironmentVariableUtils::SetValueStringImpl(ezStringView sName, ezStringView szValue)
{
  EZ_ASSERT_NOT_IMPLEMENTED
  return EZ_FAILURE;
}

bool ezEnvironmentVariableUtils::IsVariableSetImpl(ezStringView sName)
{
  return false;
}

ezResult ezEnvironmentVariableUtils::UnsetVariableImpl(ezStringView sName)
{
  return EZ_FAILURE;
}
