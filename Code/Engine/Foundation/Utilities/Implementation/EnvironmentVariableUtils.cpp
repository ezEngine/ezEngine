#include <FoundationPCH.h>

#include <Foundation/Utilities/EnvironmentVariableUtils.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Logging/Log.h>

// The POSIX functions are not thread safe by definition.
static ezMutex s_EnvVarMutex;


ezString ezEnvironmentVariableUtils::GetValueString(const char* szName, const char* szDefault /*= nullptr*/)
{
  EZ_ASSERT_DEV(!ezStringUtils::IsNullOrEmpty(szName), "Null or empty name passed to ezEnvironmentVariableUtils::GetValueString()");

  EZ_LOCK(s_EnvVarMutex);

  return GetValueStringImpl(szName, szDefault);
}

ezResult ezEnvironmentVariableUtils::SetValueString(const char* szName, const char* szValue)
{
  EZ_LOCK(s_EnvVarMutex);

  return SetValueStringImpl(szName, szValue);
}

ezInt32 ezEnvironmentVariableUtils::GetValueInt(const char* szName, ezInt32 iDefault /*= -1*/)
{
  EZ_LOCK(s_EnvVarMutex);

  ezString value = GetValueString(szName);

  if(value.IsEmpty())
    return iDefault;

  ezInt32 iRetVal = 0;
  if(ezConversionUtils::StringToInt(value, iRetVal).Succeeded())
    return iRetVal;
  else
    return iDefault;
}

ezResult ezEnvironmentVariableUtils::SetValueInt(const char* szName, ezInt32 iValue)
{
  ezStringBuilder sb;
  sb.Format("{}", iValue);

  return SetValueString(szName, sb);
}

bool ezEnvironmentVariableUtils::IsVariableSet(const char* szName)
{
  EZ_LOCK(s_EnvVarMutex);

  return IsVariableSetImpl(szName);
}

ezResult ezEnvironmentVariableUtils::UnsetVariable(const char* szName)
{
  EZ_LOCK(s_EnvVarMutex);

  return UnsetVariableImpl(szName);
}

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  #include <Foundation/Utilities/Implementation/Win/EnvironmentVariableUtils_win.h>
#elif EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
  #include <Foundation/Utilities/Implementation/Win/EnvironmentVariableUtils_win_uwp.h>
#else
  #include <Foundation/Utilities/Implementation/Posix/EnvironmentVariableUtils_posix.h>
#endif


EZ_STATICLINK_FILE(Foundation, Foundation_Utilities_Implementation_EnvironmentVariableUtils);
