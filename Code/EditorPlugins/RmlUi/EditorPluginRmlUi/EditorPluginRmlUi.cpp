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
      ezActionMapManager::RegisterActionMap("RmlUiAssetMenuBar").IgnoreResult();
      ezStandardMenus::MapActions("RmlUiAssetMenuBar", ezStandardMenuTypes::Default | ezStandardMenuTypes::Edit);
      ezProjectActions::MapActions("RmlUiAssetMenuBar");
      ezDocumentActions::MapMenuActions("RmlUiAssetMenuBar");
      ezAssetActions::MapMenuActions("RmlUiAssetMenuBar");
      ezCommandHistoryActions::MapActions("RmlUiAssetMenuBar");
    }

    // Tool Bar
    {
      ezActionMapManager::RegisterActionMap("RmlUiAssetToolBar").IgnoreResult();
      ezDocumentActions::MapToolbarActions("RmlUiAssetToolBar");
      ezCommandHistoryActions::MapActions("RmlUiAssetToolBar", "");
      ezAssetActions::MapToolBarActions("RmlUiAssetToolBar", true);
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
