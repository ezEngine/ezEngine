#include <EditorPluginKraut/EditorPluginKrautPCH.h>

#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/StandardMenus.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>

void OnLoadPlugin(bool bReloading)
{
  ezQtEditorApp::GetSingleton()->AddRuntimePluginDependency("EditorPluginKraut", "ezKrautPlugin");
  ezQtEditorApp::GetSingleton()->AddRuntimePluginDependency("EditorPluginKraut", "ezEnginePluginKraut");


  // Kraut Tree
  {
    // Menu Bar
    {
      ezActionMapManager::RegisterActionMap("KrautTreeAssetMenuBar").IgnoreResult();
      ezProjectActions::MapActions("KrautTreeAssetMenuBar");
      ezStandardMenus::MapActions("KrautTreeAssetMenuBar", ezStandardMenuTypes::File | ezStandardMenuTypes::Edit | ezStandardMenuTypes::Panels | ezStandardMenuTypes::Help);
      ezDocumentActions::MapActions("KrautTreeAssetMenuBar", "Menu.File", false);
      ezCommandHistoryActions::MapActions("KrautTreeAssetMenuBar", "Menu.Edit");
    }

    // Tool Bar
    {
      ezActionMapManager::RegisterActionMap("KrautTreeAssetToolBar").IgnoreResult();
      ezDocumentActions::MapActions("KrautTreeAssetToolBar", "", true);
      ezCommandHistoryActions::MapActions("KrautTreeAssetToolBar", "");
      ezAssetActions::MapActions("KrautTreeAssetToolBar", true);
    }
  }
}

void OnUnloadPlugin(bool bReloading) {}

ezPlugin g_Plugin(false, OnLoadPlugin, OnUnloadPlugin, "ezEditorPluginScene");
