#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/System/EnvironmentVariableUtils.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Utilities/ConversionUtils.h>

// The POSIX functions are not thread safe by definition.
static ezMutex s_EnvVarMutex;


ezString ezEnvironmentVariableUtils::GetValueString(ezStringView sName, ezStringView sDefault /*= nullptr*/)
{
  EZ_ASSERT_DEV(!sName.IsEmpty(), "Null or empty name passed to ezEnvironmentVariableUtils::GetValueString()");

  EZ_LOCK(s_EnvVarMutex);

  return GetValueStringImpl(sName, sDefault);
}

ezResult ezEnvironmentVariableUtils::SetValueString(ezStringView sName, ezStringView sValue)
{
  EZ_LOCK(s_EnvVarMutex);

  return SetValueStringImpl(sName, sValue);
}

ezInt32 ezEnvironmentVariableUtils::GetValueInt(ezStringView sName, ezInt32 iDefault /*= -1*/)
{
  EZ_LOCK(s_EnvVarMutex);

  ezString value = GetValueString(sName);

  if (value.IsEmpty())
    return iDefault;

  ezInt32 iRetVal = 0;
  if (ezConversionUtils::StringToInt(value, iRetVal).Succeeded())
    return iRetVal;
  else
    return iDefault;
}

ezResult ezEnvironmentVariableUtils::SetValueInt(ezStringView sName, ezInt32 iValue)
{
  ezStringBuilder sb;
  sb.SetFormat("{}", iValue);

  return SetValueString(sName, sb);
}

bool ezEnvironmentVariableUtils::IsVariableSet(ezStringView sName)
{
  EZ_LOCK(s_EnvVarMutex);

  return IsVariableSetImpl(sName);
}

ezResult ezEnvironmentVariableUtils::UnsetVariable(ezStringView sName)
{
  EZ_LOCK(s_EnvVarMutex);

  return UnsetVariableImpl(sName);
}


