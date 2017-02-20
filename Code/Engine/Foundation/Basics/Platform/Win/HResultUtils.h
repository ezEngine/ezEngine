#pragma once

#include <Foundation/Basics.h>

/// Build string implementation for HRESULT.
EZ_FOUNDATION_DLL ezStringView BuildString(char* tmp, ezUInt32 uiLength, HRESULT result);

/// Conversion of HRESULT to ezResult.
inline ezResult ezToResult(HRESULT result)
{
  return SUCCEEDED(result) ? EZ_SUCCESS : EZ_FAILURE;
}

#define EZ_HRESULT_TO_FAILURE(code) \
  do { HRESULT s = (code); if (FAILED(s)) return EZ_FAILURE; } while(false)

#define EZ_HRESULT_TO_FAILURE_LOG(code) \
  do { HRESULT s = (code); if (FAILED(s)) { ezLog::Error("Call '{0}' failed with: {1}", EZ_STRINGIZE(code), s); return EZ_FAILURE; } } while (false)

