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
      ezStandardMenus::MapActions("RmlUiAssetMenuBar", ezStandardMenuTypes::File | ezStandardMenuTypes::Edit | ezStandardMenuTypes::Panels | ezStandardMenuTypes::Help);
      ezProjectActions::MapActions("RmlUiAssetMenuBar");
      ezDocumentActions::MapActions("RmlUiAssetMenuBar", "Menu.File", false);
      ezCommandHistoryActions::MapActions("RmlUiAssetMenuBar", "Menu.Edit");
    }

    // Tool Bar
    {
      ezActionMapManager::RegisterActionMap("RmlUiAssetToolBar").IgnoreResult();
      ezDocumentActions::MapActions("RmlUiAssetToolBar", "", true);
      ezCommandHistoryActions::MapActions("RmlUiAssetToolBar", "");
      ezAssetActions::MapActions("RmlUiAssetToolBar", true);
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
