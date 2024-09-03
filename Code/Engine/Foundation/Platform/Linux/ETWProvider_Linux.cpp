#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_LINUX) && defined(BUILDSYSTEM_ENABLE_TRACELOGGING_LTTNG_SUPPORT)

#  include <Foundation/Platform/Linux/ETWProvider_Linux.h>

#  include <tracelogging/TraceLoggingProvider.h>

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

void ezETWProvider::LogMessage(ezLogMsgType::Enum eventType, ezUInt8 uiIndentation, ezStringView sText)
{
  const ezStringBuilder sTemp = sText;

  TraceLoggingWrite(g_ezETWLogProvider, "LogMessage", TraceLoggingValue((int)eventType, "Type"), TraceLoggingValue(uiIndentation, "Indentation"),
    TraceLoggingValue(sTemp.GetData(), "Text"));
}

ezETWProvider& ezETWProvider::GetInstance()
{
  static ezETWProvider instance;
  return instance;
}
#endif


