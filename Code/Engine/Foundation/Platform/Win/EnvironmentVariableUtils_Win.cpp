#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)

#  include <Foundation/Logging/Log.h>
#  include <Foundation/Strings/StringBuilder.h>
#  include <Foundation/System/EnvironmentVariableUtils.h>
#  include <Foundation/Threading/Mutex.h>
#  include <Foundation/Utilities/ConversionUtils.h>

#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#  include <intsafe.h>

ezString ezEnvironmentVariableUtils::GetValueStringImpl(ezStringView sName, ezStringView szDefault)
{
  ezStringWChar szwName(sName);
  wchar_t szStaticValueBuffer[64] = {0};
  size_t uiRequiredSize = 0;

  errno_t res = _wgetenv_s(&uiRequiredSize, szStaticValueBuffer, szwName);

  // Variable doesn't exist
  if (uiRequiredSize == 0)
  {
    return szDefault;
  }

  // Succeeded
  if (res == 0)
  {
    return ezString(szStaticValueBuffer);
  }
  // Static buffer was too small, do a heap allocation to query the value
  else if (res == ERANGE)
  {
    EZ_ASSERT_DEV(uiRequiredSize != SIZE_T_MAX, "");
    const size_t uiDynamicSize = uiRequiredSize + 1;
    wchar_t* szDynamicBuffer = EZ_DEFAULT_NEW_RAW_BUFFER(wchar_t, uiDynamicSize);
    ezMemoryUtils::ZeroFill(szDynamicBuffer, uiDynamicSize);

    res = _wgetenv_s(&uiRequiredSize, szDynamicBuffer, uiDynamicSize, szwName);

    if (res != 0)
    {
      ezLog::Error("Error getting environment variable \"{0}\" with dynamic buffer.", sName);
      EZ_DEFAULT_DELETE_RAW_BUFFER(szDynamicBuffer);
      return szDefault;
    }
    else
    {
      ezString retVal(szDynamicBuffer);
      EZ_DEFAULT_DELETE_RAW_BUFFER(szDynamicBuffer);
      return retVal;
    }
  }
  else
  {
    ezLog::Warning("Couldn't get environment variable value for \"{0}\", got {1} as a result.", sName, res);
    return szDefault;
  }
}

ezResult ezEnvironmentVariableUtils::SetValueStringImpl(ezStringView sName, ezStringView szValue)
{
  ezStringWChar szwName(sName);
  ezStringWChar szwValue(szValue);

  if (_wputenv_s(szwName, szwValue) == 0)
    return EZ_SUCCESS;
  else
    return EZ_FAILURE;
}

bool ezEnvironmentVariableUtils::IsVariableSetImpl(ezStringView sName)
{
  ezStringWChar szwName(sName);
  wchar_t szStaticValueBuffer[16] = {0};
  size_t uiRequiredSize = 0;

  errno_t res = _wgetenv_s(&uiRequiredSize, szStaticValueBuffer, szwName);

  if (res == 0 || res == ERANGE)
  {
    // Variable doesn't exist if uiRequiredSize is 0
    return uiRequiredSize > 0;
  }
  else
  {
    ezLog::Error("ezEnvironmentVariableUtils::IsVariableSet(\"{0}\") got {1} from _wgetenv_s.", sName, res);
    return false;
  }
}

ezResult ezEnvironmentVariableUtils::UnsetVariableImpl(ezStringView sName)
{
  ezStringWChar szwName(sName);

  if (_wputenv_s(szwName, L"") == 0)
    return EZ_SUCCESS;
  else
    return EZ_FAILURE;
}

#endif


