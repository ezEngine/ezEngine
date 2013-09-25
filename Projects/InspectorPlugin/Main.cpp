#include <PCH.h>

void OnLoadPlugin(bool bReloading)    { }
void OnUnloadPlugin(bool bReloading)  { }

ezPlugin g_Plugin(false, OnLoadPlugin, OnUnloadPlugin);

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

void AddTelemetryAssertHandler();
void RemoveTelemetryAssertHandler();


void SetAppStats();

EZ_BEGIN_SUBSYSTEM_DECLARATION(InspectorPlugin, Main)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORE_STARTUP
  {
    AddTelemetryAssertHandler();
    AddLogWriter();
    AddStatsEventHandler();
    AddStartupEventHandler();
    AddCVarEventHandler();
    AddMemoryEventHandler();
    AddInputEventHandler();
    AddPluginEventHandler();
    AddGlobalEventHandler();
    AddOSFileEventHandler();

    SetAppStats();
  }

  ON_CORE_SHUTDOWN
  {
    RemoveOSFileEventHandler();
    RemoveGlobalEventHandler();
    RemovePluginEventHandler();
    RemoveInputEventHandler();
    RemoveMemoryEventHandler();
    RemoveCVarEventHandler();
    RemoveStartupEventHandler();
    RemoveStatsEventHandler();
    RemoveLogWriter();
    RemoveTelemetryAssertHandler();

    ezTelemetry::AcceptMessagesForSystem('APP', false);
  }

EZ_END_SUBSYSTEM_DECLARATION


