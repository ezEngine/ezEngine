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
      ezActionMapManager::RegisterActionMap("VisualScriptAssetMenuBar", "AssetMenuBar");
      ezEditActions::MapActions("VisualScriptAssetMenuBar", false, false);
    }

    // Tool Bar
    {
      ezActionMapManager::RegisterActionMap("VisualScriptAssetToolBar", "AssetToolbar");
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
