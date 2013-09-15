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

void Inspector_AppDataRequests(void* pPassThrough);

void AddLogWriter();
void RemoveLogWriter();

void AddStatsEventHandler();
void RemoveStatsEventHandler();

void AddStartupEventHandler();
void RemoveStartupEventHandler();

void AddCVarEventHandler();
void RemoveCVarEventHandler();


EZ_BEGIN_SUBSYSTEM_DECLARATION(InspectorPlugin, Main)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORE_STARTUP
  {
    ezTelemetry::AcceptMessagesForSystem('APP', true, Inspector_AppDataRequests);

    AddLogWriter();
    AddStatsEventHandler();
    AddStartupEventHandler();
    AddCVarEventHandler();
  }

  ON_CORE_SHUTDOWN
  {
    RemoveCVarEventHandler();
    RemoveStartupEventHandler();
    RemoveStatsEventHandler();
    RemoveLogWriter();

    ezTelemetry::AcceptMessagesForSystem('APP', false);
  }

  ON_ENGINE_STARTUP
  {
  }

  ON_ENGINE_SHUTDOWN
  {
  }

EZ_END_SUBSYSTEM_DECLARATION


