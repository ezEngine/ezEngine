#include <PCH.h>
#include <EditorPluginAssets/EditorPluginAssets.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetObjects.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAssetObjects.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>
#include <Foundation/Reflection/Reflection.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorFramework/Actions/AssetActions.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>

void OnLoadPlugin(bool bReloading)
{
  ezQtEditorApp::GetInstance()->RegisterPluginNameForSettings("AssetsPlugin");

  // Texture Asset
  {
    // Menu Bar
    {
      ezActionMapManager::RegisterActionMap("TextureAssetMenuBar");
      ezProjectActions::MapActions("TextureAssetMenuBar");
      ezStandardMenus::MapActions("TextureAssetMenuBar", ezStandardMenuTypes::File | ezStandardMenuTypes::Edit | ezStandardMenuTypes::Panels | ezStandardMenuTypes::Help);
      ezDocumentActions::MapActions("TextureAssetMenuBar", "MenuFile", false);
      ezCommandHistoryActions::MapActions("TextureAssetMenuBar", "MenuEdit");
    }

    // Tool Bar
    {
      ezActionMapManager::RegisterActionMap("TextureAssetToolBar");
      ezDocumentActions::MapActions("TextureAssetToolBar", "", true);
      ezCommandHistoryActions::MapActions("TextureAssetToolBar", "");
      ezAssetActions::MapActions("TextureAssetToolBar", true);
    }
  }

  // Material Asset
  {
    // Menu Bar
    {
      ezActionMapManager::RegisterActionMap("MaterialAssetMenuBar");
      ezProjectActions::MapActions("MaterialAssetMenuBar");
      ezStandardMenus::MapActions("MaterialAssetMenuBar", ezStandardMenuTypes::File | ezStandardMenuTypes::Edit | ezStandardMenuTypes::Panels | ezStandardMenuTypes::Help);
      ezDocumentActions::MapActions("MaterialAssetMenuBar", "MenuFile", false);
      ezCommandHistoryActions::MapActions("MaterialAssetMenuBar", "MenuEdit");
    }

    // Tool Bar
    {
      ezActionMapManager::RegisterActionMap("MaterialAssetToolBar");
      ezDocumentActions::MapActions("MaterialAssetToolBar", "", true);
      ezCommandHistoryActions::MapActions("MaterialAssetToolBar", "");
      ezAssetActions::MapActions("MaterialAssetToolBar", true);
    }
  }

  // Mesh Asset
  {
    // Menu Bar
    {
      ezActionMapManager::RegisterActionMap("MeshAssetMenuBar");
      ezProjectActions::MapActions("MeshAssetMenuBar");
      ezStandardMenus::MapActions("MeshAssetMenuBar", ezStandardMenuTypes::File | ezStandardMenuTypes::Edit | ezStandardMenuTypes::Panels | ezStandardMenuTypes::Help);
      ezDocumentActions::MapActions("MeshAssetMenuBar", "MenuFile", false);
      ezCommandHistoryActions::MapActions("MeshAssetMenuBar", "MenuEdit");
    }

    // Tool Bar
    {
      ezActionMapManager::RegisterActionMap("MeshAssetToolBar");
      ezDocumentActions::MapActions("MeshAssetToolBar", "", true);
      ezCommandHistoryActions::MapActions("MeshAssetToolBar", "");
      ezAssetActions::MapActions("MeshAssetToolBar", true);
    }
  }
}

void OnUnloadPlugin(bool bReloading)
{

}

ezPlugin g_Plugin(false, OnLoadPlugin, OnUnloadPlugin);

EZ_DYNAMIC_PLUGIN_IMPLEMENTATION(ezEditorPluginAssets);
