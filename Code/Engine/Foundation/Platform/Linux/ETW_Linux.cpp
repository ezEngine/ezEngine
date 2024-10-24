#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_LINUX)

#  if defined(BUILDSYSTEM_ENABLE_TRACELOGGING_LTTNG_SUPPORT)

#    include <Foundation/Logging/ETW.h>
#    include <tracelogging/TraceLoggingProvider.h>

TRACELOGGING_DECLARE_PROVIDER(g_ezETWLogProvider);

// Define the GUID to use for the ez ETW Logger
// {BFD4350A-BA77-463D-B4BE-E30374E42494}
#    define EZ_LOGGER_GUID (0xbfd4350a, 0xba77, 0x463d, 0xb4, 0xbe, 0xe3, 0x3, 0x74, 0xe4, 0x24, 0x94)

TRACELOGGING_DEFINE_PROVIDER(g_ezETWLogProvider, "ezLogProvider", EZ_LOGGER_GUID);

static bool g_bRegistered = false;

void ezETW::LogMessage(ezLogMsgType::Enum eventType, ezUInt8 uiIndentation, ezStringView sText)
{
  if (!g_bRegistered)
  {
    g_bRegistered = true;
    TraceLoggingRegister(g_ezETWLogProvider);
  }

  const ezStringBuilder sTemp = sText;

  TraceLoggingWrite(g_ezETWLogProvider, "LogMessage", TraceLoggingValue((int)eventType, "Type"), TraceLoggingValue(uiIndentation, "Indentation"),
    TraceLoggingValue(sTemp.GetData(), "Text"));
}

#  else

void ezETW::LogMessage(ezLogMsgType::Enum eventType, ezUInt8 uiIndentation, ezStringView sText)
{
}

#  endif

#endif
