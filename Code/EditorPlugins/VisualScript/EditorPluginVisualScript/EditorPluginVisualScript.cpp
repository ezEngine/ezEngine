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
      ezStandardMenus::MapActions("VisualScriptAssetMenuBar", ezStandardMenuTypes::Default | ezStandardMenuTypes::Edit);
      ezProjectActions::MapActions("VisualScriptAssetMenuBar");
      ezDocumentActions::MapMenuActions("VisualScriptAssetMenuBar");
      ezAssetActions::MapMenuActions("VisualScriptAssetMenuBar");
      ezCommandHistoryActions::MapActions("VisualScriptAssetMenuBar");
      ezEditActions::MapActions("VisualScriptAssetMenuBar", false, false);
    }

    // Tool Bar
    {
      ezActionMapManager::RegisterActionMap("VisualScriptAssetToolBar").IgnoreResult();
      ezDocumentActions::MapToolbarActions("VisualScriptAssetToolBar");
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
