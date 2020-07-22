#include <EditorPluginRmlUiPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/StandardMenus.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>

void OnLoadPlugin(bool bReloading)
{
  ezQtEditorApp::GetSingleton()->AddRuntimePluginDependency("EditorPluginRmlUi", "ezRmlUiPlugin");
  ezQtEditorApp::GetSingleton()->AddRuntimePluginDependency("EditorPluginRmlUi", "ezEnginePluginRmlUi");


  // RmlUi
  {
    // Menu Bar
    {
      ezActionMapManager::RegisterActionMap("RmlUiAssetMenuBar");
      ezProjectActions::MapActions("RmlUiAssetMenuBar");
      ezStandardMenus::MapActions(
        "RmlUiAssetMenuBar", ezStandardMenuTypes::File | ezStandardMenuTypes::Edit | ezStandardMenuTypes::Panels | ezStandardMenuTypes::Help);
      ezDocumentActions::MapActions("RmlUiAssetMenuBar", "Menu.File", false);
      ezCommandHistoryActions::MapActions("RmlUiAssetMenuBar", "Menu.Edit");
    }

    // Tool Bar
    {
      ezActionMapManager::RegisterActionMap("RmlUiAssetToolBar");
      ezDocumentActions::MapActions("RmlUiAssetToolBar", "", true);
      ezCommandHistoryActions::MapActions("RmlUiAssetToolBar", "");
      ezAssetActions::MapActions("RmlUiAssetToolBar", true);
    }
  }
}

void OnUnloadPlugin(bool bReloading) {}

ezPlugin g_Plugin(false, OnLoadPlugin, OnUnloadPlugin, "ezEditorPluginScene");
