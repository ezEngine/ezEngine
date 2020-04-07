

ezString ezEnvironmentVariableUtils::GetValueStringImpl(const char* szName, const char* szDefault)
{
  EZ_ASSERT_NOT_IMPLEMENTED
  return "";
}

ezResult ezEnvironmentVariableUtils::SetValueStringImpl(const char* szName, const char* szValue)
{
  EZ_ASSERT_NOT_IMPLEMENTED
  return EZ_FAILURE;
}

bool ezEnvironmentVariableUtils::IsVariableSetImpl(const char* szName)
{
  return false;
}

ezResult ezEnvironmentVariableUtils::UnsetVariableImpl(const char* szName)
{
  return EZ_FAILURE;
}
