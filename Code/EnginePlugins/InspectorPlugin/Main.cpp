#include <InspectorPluginPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/Startup.h>

ezPlugin g_Plugin(false);

void AddLogWriter();
void RemoveLogWriter();

void AddStatsEventHandler();
void RemoveStatsEventHandler();

void AddStartupEventHandler();
void RemoveStartupEventHandler();

void AddCVarEventHandler();
void RemoveCVarEventHandler();

void AddMemoryEventHandler();
void RemoveMemoryEventHandler();

void AddInputEventHandler();
void RemoveInputEventHandler();

void AddPluginEventHandler();
void RemovePluginEventHandler();

void AddGlobalEventHandler();
void RemoveGlobalEventHandler();

void AddOSFileEventHandler();
void RemoveOSFileEventHandler();

void AddTimeEventHandler();
void RemoveTimeEventHandler();

void AddReflectionEventHandler();
void RemoveReflectionEventHandler();

void AddTelemetryAssertHandler();
void RemoveTelemetryAssertHandler();

void AddFileSystemEventHandler();
void RemoveFileSystemEventHandler();

void AddResourceManagerEventHandler();
void RemoveResourceManagerEventHandler();

void SetAppStats();

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(InspectorPlugin, InspectorPluginMain)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    AddTelemetryAssertHandler();
    AddLogWriter();
    AddStatsEventHandler();
    AddStartupEventHandler();
    AddCVarEventHandler();
    AddReflectionEventHandler();
    AddMemoryEventHandler();
    AddInputEventHandler();
    AddPluginEventHandler();
    AddGlobalEventHandler();
    AddOSFileEventHandler();
    AddTimeEventHandler();
    AddFileSystemEventHandler();
    AddResourceManagerEventHandler();

    SetAppStats();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    RemoveResourceManagerEventHandler();
    RemoveFileSystemEventHandler();
    RemoveTimeEventHandler();
    RemoveOSFileEventHandler();
    RemoveGlobalEventHandler();
    RemovePluginEventHandler();
    RemoveInputEventHandler();
    RemoveMemoryEventHandler();
    RemoveReflectionEventHandler();
    RemoveCVarEventHandler();
    RemoveStartupEventHandler();
    RemoveStatsEventHandler();
    RemoveLogWriter();
    RemoveTelemetryAssertHandler();

    ezTelemetry::AcceptMessagesForSystem(' APP', false);
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

EZ_STATICLINK_FILE(InspectorPlugin, InspectorPlugin_Main);
