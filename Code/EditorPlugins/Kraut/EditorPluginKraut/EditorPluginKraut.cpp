#include <EditorPluginKraut/EditorPluginKrautPCH.h>

#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/StandardMenus.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>

void OnLoadPlugin()
{
  // Kraut Tree
  {
    // Menu Bar
    {
      ezActionMapManager::RegisterActionMap("KrautTreeAssetMenuBar").IgnoreResult();
      ezStandardMenus::MapActions("KrautTreeAssetMenuBar", ezStandardMenuTypes::Default | ezStandardMenuTypes::Edit);
      ezProjectActions::MapActions("KrautTreeAssetMenuBar");
      ezDocumentActions::MapMenuActions("KrautTreeAssetMenuBar");
      ezAssetActions::MapMenuActions("KrautTreeAssetMenuBar");
      ezCommandHistoryActions::MapActions("KrautTreeAssetMenuBar");
    }

    // Tool Bar
    {
      ezActionMapManager::RegisterActionMap("KrautTreeAssetToolBar").IgnoreResult();
      ezDocumentActions::MapToolbarActions("KrautTreeAssetToolBar");
      ezCommandHistoryActions::MapActions("KrautTreeAssetToolBar", "");
      ezAssetActions::MapToolBarActions("KrautTreeAssetToolBar", true);
    }
  }
}

EZ_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}
