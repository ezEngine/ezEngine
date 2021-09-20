#include <EditorPluginRmlUi/EditorPluginRmlUiPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/StandardMenus.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>

void OnLoadPlugin()
{
  ezQtEditorApp::GetSingleton()->AddRuntimePluginDependency("EditorPluginRmlUi", "ezRmlUiPlugin");
  ezQtEditorApp::GetSingleton()->AddRuntimePluginDependency("EditorPluginRmlUi", "ezEnginePluginRmlUi");


  // RmlUi
  {
    // Menu Bar
    {
      ezActionMapManager::RegisterActionMap("RmlUiAssetMenuBar").IgnoreResult();
      ezProjectActions::MapActions("RmlUiAssetMenuBar");
      ezStandardMenus::MapActions("RmlUiAssetMenuBar", ezStandardMenuTypes::File | ezStandardMenuTypes::Edit | ezStandardMenuTypes::Panels | ezStandardMenuTypes::Help);
      ezDocumentActions::MapActions("RmlUiAssetMenuBar", "Menu.File", false);
      ezCommandHistoryActions::MapActions("RmlUiAssetMenuBar", "Menu.Edit");
    }

    // Tool Bar
    {
      ezActionMapManager::RegisterActionMap("RmlUiAssetToolBar").IgnoreResult();
      ezDocumentActions::MapActions("RmlUiAssetToolBar", "", true);
      ezCommandHistoryActions::MapActions("RmlUiAssetToolBar", "");
      ezAssetActions::MapActions("RmlUiAssetToolBar", true);
    }
  }
}

void OnUnloadPlugin() {}

EZ_PLUGIN_DEPENDENCY(ezEditorPluginScene);

// clang-format off
EZ_BEGIN_PLUGIN(ezEditorPluginRmlUi)

  BEGIN_PLUGIN_DEPENDENCIES
    //"ezEditorPluginScene"
  END_PLUGIN_DEPENDENCIES

  ON_PLUGIN_LOADED
  {
    OnLoadPlugin();
  }
  
  ON_PLUGIN_UNLOADED
  {
    OnUnloadPlugin();
  }

EZ_END_PLUGIN;
// clang-format on
