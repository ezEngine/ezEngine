#include <EditorPluginVisualScript/EditorPluginVisualScriptPCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/EditActions.h>
#include <GuiFoundation/Action/StandardMenus.h>

void OnLoadPlugin()
{
  // VisualScript
  {
    // Menu Bar
    {
      ezActionMapManager::RegisterActionMap("VisualScriptAssetMenuBar").IgnoreResult();
      ezStandardMenus::MapActions("VisualScriptAssetMenuBar", ezStandardMenuTypes::File | ezStandardMenuTypes::Edit | ezStandardMenuTypes::Panels | ezStandardMenuTypes::Help);
      ezProjectActions::MapActions("VisualScriptAssetMenuBar");
      ezDocumentActions::MapActions("VisualScriptAssetMenuBar", "Menu.File", false);
      ezAssetActions::MapMenuActions("VisualScriptAssetMenuBar", "Menu.File");
      ezCommandHistoryActions::MapActions("VisualScriptAssetMenuBar", "Menu.Edit");
      ezEditActions::MapActions("VisualScriptAssetMenuBar", "Menu.Edit", false, false);
    }

    // Tool Bar
    {
      ezActionMapManager::RegisterActionMap("VisualScriptAssetToolBar").IgnoreResult();
      ezDocumentActions::MapActions("VisualScriptAssetToolBar", "", true);
      ezCommandHistoryActions::MapActions("VisualScriptAssetToolBar", "");
      ezAssetActions::MapToolBarActions("VisualScriptAssetToolBar", true);
    }
  }
}

void OnUnloadPlugin()
{
}

EZ_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

EZ_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}
