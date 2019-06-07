#include <EditorPluginProcGenPCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginProcGen/Actions/ProcGenActions.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/EditActions.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>

static void ToolsProjectEventHandler(const ezToolsProjectEvent& e);

void OnLoadPlugin(bool bReloading)
{
  ezQtEditorApp::GetSingleton()->AddRuntimePluginDependency("EditorPluginProcGen", "ezProcGenPlugin");
  //ezQtEditorApp::GetSingleton()->AddRuntimePluginDependency("EditorPluginProcGen", "ezEnginePluginProcGen");

  // ezToolsProject::GetSingleton()->s_Events.AddEventHandler(ToolsProjectEventHandler);

  // Asset
  {// Menu Bar
    {const char* szMenuBar = "ProcGenAssetMenuBar";

  ezActionMapManager::RegisterActionMap(szMenuBar);
  ezProjectActions::MapActions(szMenuBar);
  ezStandardMenus::MapActions(
    szMenuBar, ezStandardMenuTypes::File | ezStandardMenuTypes::Edit | ezStandardMenuTypes::Panels | ezStandardMenuTypes::Help);
  ezDocumentActions::MapActions(szMenuBar, "Menu.File", false);
  ezCommandHistoryActions::MapActions(szMenuBar, "Menu.Edit");

  ezEditActions::MapActions("ProcGenAssetMenuBar", "Menu.Edit", false, false);
}

// Tool Bar
{
  const char* szToolBar = "ProcGenAssetToolBar";
  ezActionMapManager::RegisterActionMap(szToolBar);
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

void OnUnloadPlugin(bool bReloading)
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


ezPlugin g_Plugin(false, OnLoadPlugin, OnUnloadPlugin, "ezEditorPluginScene");
