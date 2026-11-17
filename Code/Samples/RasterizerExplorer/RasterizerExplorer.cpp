#include <RasterizerExplorer/RasterizerExplorer.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Logging/Log.h>

EZ_APPLICATION_ENTRY_POINT(ezRasterizerExplorerApp);

ezRasterizerExplorerApp::ezRasterizerExplorerApp()
  : ezGameApplication("RasterizerExplorer", nullptr)
{
}

ezResult ezRasterizerExplorerApp::BeforeCoreSystemsStartup()
{
  ezStartup::AddApplicationTag("game");

  EZ_SUCCEED_OR_RETURN(SUPER::BeforeCoreSystemsStartup());

  m_sAppProjectPath = "Data/Samples/RasterizerExplorer";

  return EZ_SUCCESS;
}

void ezRasterizerExplorerApp::AfterCoreSystemsStartup()
{
  ExecuteInitFunctions();

  ezStartup::StartupHighLevelSystems();

  ActivateGameState(nullptr, {}, ezTransform::MakeIdentity());
}

