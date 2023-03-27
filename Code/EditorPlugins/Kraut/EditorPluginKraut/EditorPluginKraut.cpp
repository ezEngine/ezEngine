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
      ezStandardMenus::MapActions("KrautTreeAssetMenuBar", ezStandardMenuTypes::File | ezStandardMenuTypes::Edit | ezStandardMenuTypes::Panels | ezStandardMenuTypes::Help);
      ezProjectActions::MapActions("KrautTreeAssetMenuBar");
      ezDocumentActions::MapActions("KrautTreeAssetMenuBar", "Menu.File", false);
      ezAssetActions::MapMenuActions("KrautTreeAssetMenuBar", "Menu.File");
      ezCommandHistoryActions::MapActions("KrautTreeAssetMenuBar", "Menu.Edit");
    }

    // Tool Bar
    {
      ezActionMapManager::RegisterActionMap("KrautTreeAssetToolBar").IgnoreResult();
      ezDocumentActions::MapActions("KrautTreeAssetToolBar", "", true);
      ezCommandHistoryActions::MapActions("KrautTreeAssetToolBar", "");
      ezAssetActions::MapToolBarActions("KrautTreeAssetToolBar", true);
    }
  }
}

EZ_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}
