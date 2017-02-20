#include <PCH.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)

#include <Foundation/Basics/Platform/Win/HResultUtils.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Strings/StringConversion.h>

#include <comdef.h>

ezStringView BuildString(char* tmp, ezUInt32 uiLength, HRESULT result)
{
  _com_error error(result, nullptr);
  const TCHAR* messageW = error.ErrorMessage();

  // Com error tends to put /r/n at the end. Remove it.
  ezStringBuilder message(ezStringUtf8(messageW).GetData());
  message.ReplaceAll("\n", "");
  message.ReplaceAll("\r", "");

  ezStringUtils::Copy(tmp, uiLength, message.GetData());
  return tmp;
}

#endif
