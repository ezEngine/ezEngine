#include <PCH.h>
#include <EditorPluginAssets/EditorPluginAssets.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetObjects.h>
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
  ezToolsReflectionUtils::RegisterType(ezGetStaticRTTI<ezTextureAssetProperties>());

  ezEditorApp::GetInstance()->RegisterPluginNameForSettings("AssetsPlugin");

  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("TextureAssetMenuBar");
    ezProjectActions::MapActions("TextureAssetMenuBar");
    ezStandardMenus::MapActions("TextureAssetMenuBar", ezStandardMenuTypes::File | ezStandardMenuTypes::Edit);
    ezDocumentActions::MapActions("TextureAssetMenuBar", "File", false);
    ezCommandHistoryActions::MapActions("TextureAssetMenuBar", "Edit");
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("TextureAssetToolBar");
    ezDocumentActions::MapActions("TextureAssetToolBar", "", true);
    ezCommandHistoryActions::MapActions("TextureAssetToolBar", "");
    ezAssetActions::MapActions("TextureAssetToolBar", true);
  }
}

void OnUnloadPlugin(bool bReloading)
{

}

ezPlugin g_Plugin(false, OnLoadPlugin, OnUnloadPlugin);

EZ_DYNAMIC_PLUGIN_IMPLEMENTATION(ezEditorPluginAssets);
