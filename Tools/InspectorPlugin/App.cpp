#include <PCH.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Threading/Thread.h>
#include <Foundation/System/SystemInformation.h>
#include <Foundation/Utilities/Stats.h>

static ezAssertHandler g_PreviousAssertHandler = nullptr;

static bool TelemetryAssertHandler(const char* szSourceFile, ezUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szAssertMsg)
{
  if (ezTelemetry::IsConnectedToClient())
  {
    ezTelemetryMessage msg;
    msg.SetMessageID('APP', 'ASRT');
    msg.GetWriter() << szSourceFile;
    msg.GetWriter() << uiLine;
    msg.GetWriter() << szFunction;
    msg.GetWriter() << szExpression;
    msg.GetWriter() << szAssertMsg;

    ezTelemetry::Broadcast(ezTelemetry::Reliable, msg);

    // messages might not arrive, if the network does not get enough time to transmit them
    // since we are crashing the application in (half) 'a second', we need to make sure the network traffic has indeed been sent
    for (ezUInt32 i = 0; i < 5; ++i)
    {
      ezThreadUtils::Sleep(100);
      ezTelemetry::UpdateNetwork();
    }
  }

  if (g_PreviousAssertHandler)
    return g_PreviousAssertHandler(szSourceFile, uiLine, szFunction, szExpression, szAssertMsg);

  return true;
}

void AddTelemetryAssertHandler()
{
  g_PreviousAssertHandler = ezGetAssertHandler();
  ezSetAssertHandler(TelemetryAssertHandler);
}

void RemoveTelemetryAssertHandler()
{
  ezSetAssertHandler(g_PreviousAssertHandler);
  g_PreviousAssertHandler = nullptr;
}

void SetAppStats()
{
  ezStringBuilder sOut;
  const ezSystemInformation info = ezSystemInformation::Get();

  ezStats::SetStat("Platform/Name", info.GetPlatformName());

  sOut.Format("%i", info.GetCPUCoreCount());
  ezStats::SetStat("Hardware/CPU Cores", sOut.GetData());

  sOut.Format("%.1f GB", info.GetInstalledMainMemory() / 1024.0f / 1024.0f / 1024.0f);
  ezStats::SetStat("Hardware/RAM", sOut.GetData());

  sOut = info.Is64BitOS() ? "64 Bit" : "32 Bit";
  ezStats::SetStat("Platform/Architecture", sOut.GetData());

  #if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    sOut = "Debug";
  #elif EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
    sOut = "Dev";
  #else
    sOut = "Release";
  #endif
  ezStats::SetStat("Platform/Build", sOut.GetData());

  #if EZ_ENABLED(EZ_USE_PROFILING)
    sOut = "Enabled";
  #else
    sOut = "Disabled";
  #endif
  ezStats::SetStat("Features/Profiling", sOut.GetData());

  #if EZ_ENABLED(EZ_USE_ALLOCATION_STACK_TRACING)
    sOut = "Enabled";
  #else
    sOut = "Disabled";
  #endif
  ezStats::SetStat("Features/Allocation Stack Tracing", sOut.GetData());

  #if EZ_ENABLED(EZ_PLATFORM_LITTLE_ENDIAN)
    sOut = "Little";
  #else
    sOut = "Big";
  #endif
  ezStats::SetStat("Platform/Endianess", sOut.GetData());

}




EZ_STATICLINK_FILE(InspectorPlugin, InspectorPlugin_App);

