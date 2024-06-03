#include <EditorPluginRmlUi/EditorPluginRmlUiPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/StandardMenus.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>

void OnLoadPlugin()
{
  // RmlUi
  {
    // Menu Bar
    {
      ezActionMapManager::RegisterActionMap("RmlUiAssetMenuBar", "AssetMenuBar");
    }

    // Tool Bar
    {
      ezActionMapManager::RegisterActionMap("RmlUiAssetToolBar", "AssetToolbar");
    }
  }
}

void OnUnloadPlugin() {}

EZ_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

EZ_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}
