#include <PCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>

#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAssetObjects.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <GuiFoundation/UIServices/DynamicEnums.h>
#include <PhysXCooking/PhysXCooking.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorFramework/Actions/AssetActions.h>

void OnLoadPlugin(bool bReloading)
{
  ezQtEditorApp::GetSingleton()->AddRuntimePluginDependency("EditorPluginKraut", "ezKrautPlugin");
  ezQtEditorApp::GetSingleton()->AddRuntimePluginDependency("EditorPluginKraut", "ezEnginePluginKraut");


  // Kraut Tree Asset
  {
    // Menu Bar
    {
      ezActionMapManager::RegisterActionMap("KrautTreeAssetMenuBar");
      ezProjectActions::MapActions("KrautTreeAssetMenuBar");
      ezStandardMenus::MapActions("KrautTreeAssetMenuBar", ezStandardMenuTypes::File | ezStandardMenuTypes::Edit |
                                                                   ezStandardMenuTypes::Panels | ezStandardMenuTypes::Help);
      ezDocumentActions::MapActions("KrautTreeAssetMenuBar", "Menu.File", false);
      ezCommandHistoryActions::MapActions("KrautTreeAssetMenuBar", "Menu.Edit");
    }

    // Tool Bar
    {
      ezActionMapManager::RegisterActionMap("KrautTreeAssetToolBar");
      ezDocumentActions::MapActions("KrautTreeAssetToolBar", "", true);
      ezCommandHistoryActions::MapActions("KrautTreeAssetToolBar", "");
      ezAssetActions::MapActions("KrautTreeAssetToolBar", true);
    }
  }
}

void OnUnloadPlugin(bool bReloading)
{
}

ezPlugin g_Plugin(false, OnLoadPlugin, OnUnloadPlugin, "ezEditorPluginScene");

EZ_DYNAMIC_PLUGIN_IMPLEMENTATION(EZ_EDITORPLUGINKRAUT_DLL, ezEditorPluginKraut);
