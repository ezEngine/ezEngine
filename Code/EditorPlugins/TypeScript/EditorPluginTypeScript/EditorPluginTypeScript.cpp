#include <EditorPluginTypeScriptPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorPluginTypeScript/TypeScriptAsset/TypeScriptAssetObjects.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <GuiFoundation/UIServices/DynamicEnums.h>
#include <PhysXCooking/PhysXCooking.h>

void OnLoadPlugin(bool bReloading)
{
  ezQtEditorApp::GetSingleton()->AddRuntimePluginDependency("EditorPluginTypeScript", "ezTypeScriptPlugin");

  // TypeScript Asset
  {
    // Menu Bar
    {
      ezActionMapManager::RegisterActionMap("TypeScriptAssetMenuBar");
      ezProjectActions::MapActions("TypeScriptAssetMenuBar");
      ezStandardMenus::MapActions("TypeScriptAssetMenuBar", ezStandardMenuTypes::File | ezStandardMenuTypes::Edit | ezStandardMenuTypes::Panels | ezStandardMenuTypes::Help);
      ezDocumentActions::MapActions("TypeScriptAssetMenuBar", "Menu.File", false);
      ezCommandHistoryActions::MapActions("TypeScriptAssetMenuBar", "Menu.Edit");
    }

    // Tool Bar
    {
      ezActionMapManager::RegisterActionMap("TypeScriptAssetToolBar");
      ezDocumentActions::MapActions("TypeScriptAssetToolBar", "", true);
      ezCommandHistoryActions::MapActions("TypeScriptAssetToolBar", "");
      ezAssetActions::MapActions("TypeScriptAssetToolBar", true);
    }
  }
}

void OnUnloadPlugin(bool bReloading) {}

ezPlugin g_Plugin(false, OnLoadPlugin, OnUnloadPlugin, "ezEditorPluginScene");
