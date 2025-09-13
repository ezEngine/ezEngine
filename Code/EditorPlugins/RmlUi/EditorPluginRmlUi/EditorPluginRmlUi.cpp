#include <EditorPluginRmlUi/EditorPluginRmlUiPCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginRmlUi/Actions/RmlUiActions.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/StandardMenus.h>

void OnLoadPlugin()
{
  // RmlUi
  {
    ezRmlUiActions::RegisterActions();

    // Menu Bar
    {
      ezActionMapManager::RegisterActionMap("RmlUiAssetMenuBar", "AssetMenuBar");
      ezRmlUiActions::MapActionsMenu("RmlUiAssetMenuBar");
    }

    // Tool Bar
    {
      ezActionMapManager::RegisterActionMap("RmlUiAssetToolBar", "AssetToolbar");
      ezRmlUiActions::MapActionsToolbar("RmlUiAssetToolBar");
    }
  }
}

void OnUnloadPlugin()
{
  ezRmlUiActions::UnregisterActions();
}

EZ_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

EZ_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}
