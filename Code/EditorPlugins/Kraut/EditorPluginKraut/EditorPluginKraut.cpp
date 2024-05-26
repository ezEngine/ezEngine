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
      ezActionMapManager::RegisterActionMap("KrautTreeAssetMenuBar", "AssetMenuBar").IgnoreResult();
    }

    // Tool Bar
    {
      ezActionMapManager::RegisterActionMap("KrautTreeAssetToolBar", "AssetToolbar").IgnoreResult();
    }
  }
}

EZ_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}
