#include <EditorPluginFmod/EditorPluginFmodPCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/StandardMenus.h>

#include <EditorPluginFmod/Actions/FmodActions.h>
#include <EditorPluginFmod/Preferences/FmodPreferences.h>

static void ToolsProjectEventHandler(const ezToolsProjectEvent& e);

void OnLoadPlugin()
{
  ezToolsProject::GetSingleton()->s_Events.AddEventHandler(ToolsProjectEventHandler);

  // Mesh
  {
    // Menu Bar
    ezActionMapManager::RegisterActionMap("SoundBankAssetMenuBar", "AssetMenuBar");

    // Tool Bar
    {
      ezActionMapManager::RegisterActionMap("SoundBankAssetToolBar", "AssetToolbar");
    }
  }

  // Scene
  {
    // Menu Bar
    {
      ezFmodActions::RegisterActions();
      ezFmodActions::MapPluginMenuActions("AssetMenuBar");
      ezFmodActions::MapMenuActions("EditorPluginScene_DocumentMenuBar");
      ezFmodActions::MapMenuActions("EditorPluginScene_Scene2MenuBar");
      ezFmodActions::MapToolbarActions("EditorPluginScene_DocumentToolBar");
      ezFmodActions::MapToolbarActions("EditorPluginScene_Scene2ToolBar");
    }
  }
}

void OnUnloadPlugin()
{
  ezFmodActions::UnregisterActions();
  ezToolsProject::GetSingleton()->s_Events.RemoveEventHandler(ToolsProjectEventHandler);
}

static void ToolsProjectEventHandler(const ezToolsProjectEvent& e)
{
  if (e.m_Type == ezToolsProjectEvent::Type::ProjectOpened)
  {
    ezFmodProjectPreferences* pPreferences = ezPreferences::QueryPreferences<ezFmodProjectPreferences>();
    pPreferences->SyncCVars();
  }
}

EZ_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

EZ_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}
