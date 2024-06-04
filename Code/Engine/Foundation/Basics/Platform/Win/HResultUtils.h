#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Basics/Platform/Win/MinWindows.h>
#include <Foundation/Strings/String.h>

/// Build string implementation for HRESULT.
EZ_FOUNDATION_DLL ezString ezHRESULTtoString(ezMinWindows::HRESULT result);

/// Conversion of HRESULT to ezResult.
EZ_ALWAYS_INLINE ezResult ezToResult(ezMinWindows::HRESULT result)
{
  return result >= 0 ? EZ_SUCCESS : EZ_FAILURE;
}

#define EZ_HRESULT_TO_FAILURE(code)   \
  do                                  \
  {                                   \
    ezMinWindows::HRESULT s = (code); \
    if (s < 0)                        \
      return EZ_FAILURE;              \
  } while (false)

#define EZ_HRESULT_TO_FAILURE_LOG(code)                                                         \
  do                                                                                            \
  {                                                                                             \
    ezMinWindows::HRESULT s = (code);                                                           \
    if (s < 0)                                                                                  \
    {                                                                                           \
      ezLog::Error("Call '{0}' failed with: {1}", EZ_PP_STRINGIFY(code), ezHRESULTtoString(s)); \
      return EZ_FAILURE;                                                                        \
    }                                                                                           \
  } while (false)

#define EZ_HRESULT_TO_LOG(code)                                                                 \
  do                                                                                            \
  {                                                                                             \
    ezMinWindows::HRESULT s = (code);                                                           \
    if (s < 0)                                                                                  \
    {                                                                                           \
      ezLog::Error("Call '{0}' failed with: {1}", EZ_PP_STRINGIFY(code), ezHRESULTtoString(s)); \
    }                                                                                           \
  } while (false)

#define EZ_NO_RETURNVALUE

#define EZ_HRESULT_TO_LOG_RET(code, ret)                                                        \
  do                                                                                            \
  {                                                                                             \
    ezMinWindows::HRESULT s = (code);                                                           \
    if (s < 0)                                                                                  \
    {                                                                                           \
      ezLog::Error("Call '{0}' failed with: {1}", EZ_PP_STRINGIFY(code), ezHRESULTtoString(s)); \
      return ret;                                                                               \
    }                                                                                           \
  } while (false)

#define EZ_HRESULT_TO_ASSERT(code)                                                                     \
  do                                                                                                   \
  {                                                                                                    \
    ezMinWindows::HRESULT s = (code);                                                                  \
    EZ_ASSERT_DEV(s >= 0, "Call '{0}' failed with: {1}", EZ_PP_STRINGIFY(code), ezHRESULTtoString(s)); \
  } while (false)
