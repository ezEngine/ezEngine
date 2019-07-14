#include <FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)

#  include <Foundation/Basics/Platform/Win/HResultUtils.h>
#  include <Foundation/Strings/StringBuilder.h>
#  include <Foundation/Strings/StringConversion.h>

#  include <comdef.h>

EZ_FOUNDATION_DLL ezString ezHRESULTtoString(ezMinWindows::HRESULT result)
{
  _com_error error(result, nullptr);
  const TCHAR* messageW = error.ErrorMessage();

  // Com error tends to put /r/n at the end. Remove it.
  ezStringBuilder message(ezStringUtf8(messageW).GetData());
  message.ReplaceAll("\n", "");
  message.ReplaceAll("\r", "");

  return message;
}

#endif



EZ_STATICLINK_FILE(Foundation, Foundation_Basics_Platform_Win_HResultUtils);

