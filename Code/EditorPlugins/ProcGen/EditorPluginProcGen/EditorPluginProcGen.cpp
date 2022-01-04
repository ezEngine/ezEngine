#include <EditorPluginProcGen/EditorPluginProcGenPCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorPluginProcGen/Actions/ProcGenActions.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/EditActions.h>
#include <GuiFoundation/Action/StandardMenus.h>

static void ToolsProjectEventHandler(const ezToolsProjectEvent& e);

void OnLoadPlugin()
{
  ezQtEditorApp::GetSingleton()->AddRuntimePluginDependency("EditorPluginProcGen", "ezProcGenPlugin");
  // ezQtEditorApp::GetSingleton()->AddRuntimePluginDependency("EditorPluginProcGen", "ezEnginePluginProcGen");

  // ezToolsProject::GetSingleton()->s_Events.AddEventHandler(ToolsProjectEventHandler);

  // Asset
  {// Menu Bar
    {const char* szMenuBar = "ProcGenAssetMenuBar";

  ezActionMapManager::RegisterActionMap(szMenuBar).IgnoreResult();
  ezStandardMenus::MapActions(szMenuBar, ezStandardMenuTypes::File | ezStandardMenuTypes::Edit | ezStandardMenuTypes::Panels | ezStandardMenuTypes::Help);
  ezProjectActions::MapActions(szMenuBar);
  ezDocumentActions::MapActions(szMenuBar, "Menu.File", false);
  ezCommandHistoryActions::MapActions(szMenuBar, "Menu.Edit");

  ezEditActions::MapActions("ProcGenAssetMenuBar", "Menu.Edit", false, false);
}

// Tool Bar
{
  const char* szToolBar = "ProcGenAssetToolBar";
  ezActionMapManager::RegisterActionMap(szToolBar).IgnoreResult();
  ezDocumentActions::MapActions(szToolBar, "", true);
  ezCommandHistoryActions::MapActions(szToolBar, "");
  ezAssetActions::MapActions(szToolBar, true);
}
}

// Scene
{
  // Menu Bar
  {
    ezProcGenActions::RegisterActions();
    ezProcGenActions::MapMenuActions();
  }

  // Tool Bar
  {
  }
}
}

void OnUnloadPlugin()
{
  ezProcGenActions::UnregisterActions();
  // ezToolsProject::GetSingleton()->s_Events.RemoveEventHandler(ToolsProjectEventHandler);
}

/*static void ToolsProjectEventHandler(const ezToolsProjectEvent& e)
{
  if (e.m_Type == ezToolsProjectEvent::Type::ProjectOpened)
  {
    ezFmodProjectPreferences* pPreferences = ezPreferences::QueryPreferences<ezFmodProjectPreferences>();
    pPreferences->SyncCVars();
  }
}*/

EZ_PLUGIN_DEPENDENCY(ezEditorPluginScene);

EZ_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

EZ_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}
