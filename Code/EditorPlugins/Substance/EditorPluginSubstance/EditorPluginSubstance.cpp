#include <EditorPluginSubstance/EditorPluginSubstancePCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
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
      const char* szMenuBar = "SubstanceAssetMenuBar";

  ezActionMapManager::RegisterActionMap(szMenuBar).IgnoreResult();
  ezStandardMenus::MapActions(szMenuBar, ezStandardMenuTypes::Default | ezStandardMenuTypes::Edit);
  ezProjectActions::MapActions(szMenuBar);
  ezDocumentActions::MapMenuActions(szMenuBar);
  ezAssetActions::MapMenuActions(szMenuBar);
  ezCommandHistoryActions::MapActions(szMenuBar);

  ezEditActions::MapActions("SubstanceAssetMenuBar", false, false);
}

// Tool Bar
{
  const char* szToolBar = "SubstanceAssetToolBar";
  ezActionMapManager::RegisterActionMap(szToolBar).IgnoreResult();
  ezDocumentActions::MapToolbarActions(szToolBar);
  ezCommandHistoryActions::MapActions(szToolBar, "");
  ezAssetActions::MapToolBarActions(szToolBar, true);
}
}

// Scene
{
  // Menu Bar
  {}

  // Tool Bar
  {
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
