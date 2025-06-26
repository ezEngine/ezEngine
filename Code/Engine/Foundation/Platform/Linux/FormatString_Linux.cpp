#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_LINUX)

#  include <Foundation/Strings/FormatString.h>
#  include <Foundation/Strings/String.h>
#  include <Foundation/Strings/StringBuilder.h>

#  include <string.h>

ezStringView BuildString(char* szTmp, ezUInt32 uiLength, const ezArgErrno& arg)
{
  const char* szErrorMsg = std::strerror(arg.m_iErrno);
  ezStringUtils::snprintf(szTmp, uiLength, "%i (\"%s\")", arg.m_iErrno, szErrorMsg);
  return ezStringView(szTmp);
}

ezStringView BuildString(char* szTmp, ezUInt32 uiLength, const ezArgErrorCode& arg)
{
  ezStringUtils::snprintf(szTmp, uiLength, "%u", arg.m_ErrorCode);
  return ezStringView(szTmp);
}
#endif
