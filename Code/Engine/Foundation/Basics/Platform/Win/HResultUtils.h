#pragma once

#include <Foundation/Basics.h>

/// Build string implementation for HRESULT.
EZ_FOUNDATION_DLL ezStringView BuildString(char* tmp, ezUInt32 uiLength, HRESULT result);

/// Conversion of HRESULT to ezResult.
inline ezResult ezToResult(HRESULT result)
{
  return SUCCEEDED(result) ? EZ_SUCCESS : EZ_FAILURE;
}

/// Return HRESULT if failed.
#define EZ_SUCCEED_OR_PASS_HRESULT_ON(x) \
  do { HRESULT h = (x); if(FAILED(h)) return h; } while(false)
