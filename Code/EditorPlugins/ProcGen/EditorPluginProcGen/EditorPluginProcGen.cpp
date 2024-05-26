#include <EditorPluginProcGen/EditorPluginProcGenPCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorPluginProcGen/Actions/ProcGenActions.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/EditActions.h>
#include <GuiFoundation/Action/StandardMenus.h>

static void ToolsProjectEventHandler(const ezToolsProjectEvent& e);

void OnLoadPlugin()
{
  // Asset
  {
    // Menu Bar
    {
      const char* szMenuBar = "ProcGenAssetMenuBar";
      ezActionMapManager::RegisterActionMap(szMenuBar, "AssetMenuBar").IgnoreResult();
      ezEditActions::MapActions(szMenuBar, false, false);
    }

    // Tool Bar
    {
      const char* szToolBar = "ProcGenAssetToolBar";
      ezActionMapManager::RegisterActionMap(szToolBar, "AssetToolbar").IgnoreResult();
    }
  }

  // Scene
  {
    // Menu Bar
    {
      ezProcGenActions::RegisterActions();
      ezProcGenActions::MapMenuActions();
    }

    // Tool Bar
    {
    }
  }
}

void OnUnloadPlugin()
{
  ezProcGenActions::UnregisterActions();
}

EZ_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

EZ_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}
