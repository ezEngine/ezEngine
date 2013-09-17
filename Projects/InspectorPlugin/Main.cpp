#include <PCH.h>

void OnLoadPlugin(bool bReloading);
void OnUnloadPlugin(bool bReloading);

ezPlugin g_Plugin(false, OnLoadPlugin, OnUnloadPlugin);

void OnLoadPlugin(bool bReloading)
{

}

void OnUnloadPlugin(bool bReloading)
{

}

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

void SetAppStats();

EZ_BEGIN_SUBSYSTEM_DECLARATION(InspectorPlugin, Main)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORE_STARTUP
  {
    AddLogWriter();
    AddStatsEventHandler();
    AddStartupEventHandler();
    AddCVarEventHandler();
    AddMemoryEventHandler();
    AddInputEventHandler();

    SetAppStats();
  }

  ON_CORE_SHUTDOWN
  {
    RemoveInputEventHandler();
    RemoveMemoryEventHandler();
    RemoveCVarEventHandler();
    RemoveStartupEventHandler();
    RemoveStatsEventHandler();
    RemoveLogWriter();

    ezTelemetry::AcceptMessagesForSystem('APP', false);
  }

EZ_END_SUBSYSTEM_DECLARATION


