#include <EditorPluginSubstance/EditorPluginSubstancePCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorPluginSubstance/Assets/SubstancePackageAssetWindow.moc.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/EditActions.h>
#include <GuiFoundation/Action/StandardMenus.h>

static void ToolsProjectEventHandler(const ezToolsProjectEvent& e);

void OnLoadPlugin()
{
  ezSubstancePackageAssetActions::RegisterActions();

  // Asset
  {
    // Menu Bar
    {
      const char* szMenuBar = "SubstanceAssetMenuBar";

      ezActionMapManager::RegisterActionMap(szMenuBar, "AssetMenuBar");
      ezEditActions::MapActions(szMenuBar, false, false);
    }

    // Tool Bar
    {
      const char* szToolBar = "SubstanceAssetToolBar";
      ezActionMapManager::RegisterActionMap(szToolBar, "AssetToolbar");
      ezSubstancePackageAssetActions::MapToolbarActions("SubstanceAssetToolBar");
    }
  }

  // Scene
  {
    // Menu Bar
    {
    }

    // Tool Bar
    {
    }
  }
}

void OnUnloadPlugin()
{
  ezSubstancePackageAssetActions::UnregisterActions();
}

EZ_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

EZ_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}
