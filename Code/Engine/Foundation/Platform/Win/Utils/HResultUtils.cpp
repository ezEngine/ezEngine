#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)

#  include <Foundation/Platform/Win/Utils/HResultUtils.h>
#  include <Foundation/Strings/StringBuilder.h>
#  include <Foundation/Strings/StringConversion.h>

EZ_FOUNDATION_DLL ezString ezHRESULTtoString(ezMinWindows::HRESULT result)
{
  wchar_t buffer[4096];
  if (::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM,
        nullptr,
        result,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
        buffer,
        EZ_ARRAY_SIZE(buffer),
        nullptr) == 0)
  {
    return {};
  }

  // Com error tends to put /r/n at the end. Remove it.
  ezStringBuilder message(ezStringUtf8(&buffer[0]).GetData());
  message.ReplaceAll("\n", "");
  message.ReplaceAll("\r", "");

  return message;
}
#else

#  include <Foundation/Platform/Win/Utils/HResultUtils.h>

EZ_FOUNDATION_DLL ezString ezHRESULTtoString(ezMinWindows::HRESULT result)
{
  return "NOT_SUPPORTED";
}
#endif
