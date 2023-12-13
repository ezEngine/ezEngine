#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)

#  include <Foundation/Platform/Win/ETWProvider_Win.h>

#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#  include <TraceLoggingProvider.h>

// Workaround to support TraceLoggingProvider.h and /utf-8 compiler switch.
#  undef _TlgPragmaUtf8Begin
#  undef _TlgPragmaUtf8End
#  define _TlgPragmaUtf8Begin
#  define _TlgPragmaUtf8End
#  undef _tlgPragmaUtf8Begin
#  undef _tlgPragmaUtf8End
#  define _tlgPragmaUtf8Begin
#  define _tlgPragmaUtf8End

TRACELOGGING_DECLARE_PROVIDER(g_ezETWLogProvider);

// Define the GUID to use for the ez ETW Logger
// {BFD4350A-BA77-463D-B4BE-E30374E42494}
#  define EZ_LOGGER_GUID (0xbfd4350a, 0xba77, 0x463d, 0xb4, 0xbe, 0xe3, 0x3, 0x74, 0xe4, 0x24, 0x94)

TRACELOGGING_DEFINE_PROVIDER(g_ezETWLogProvider, "ezLogProvider", EZ_LOGGER_GUID);

ezETWProvider::ezETWProvider()
{
  TraceLoggingRegister(g_ezETWLogProvider);
}

ezETWProvider::~ezETWProvider()
{
  TraceLoggingUnregister(g_ezETWLogProvider);
}

void ezETWProvider::LogMessge(ezLogMsgType::Enum eventType, ezUInt8 uiIndentation, ezStringView sText)
{
  const ezStringBuilder sTemp = sText;

  TraceLoggingWrite(g_ezETWLogProvider, "LogMessge", TraceLoggingValue((int)eventType, "Type"), TraceLoggingValue(uiIndentation, "Indentation"),
    TraceLoggingValue(sTemp.GetData(), "Text"));
}

ezETWProvider& ezETWProvider::GetInstance()
{
  static ezETWProvider instance;
  return instance;
}
#endif


EZ_STATICLINK_FILE_DISABLE()
