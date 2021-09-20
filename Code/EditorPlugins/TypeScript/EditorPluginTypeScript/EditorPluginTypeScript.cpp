#include <EditorPluginTypeScript/EditorPluginTypeScriptPCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginTypeScript/Actions/TypeScriptActions.h>
#include <EditorPluginTypeScript/TypeScriptAsset/TypeScriptAssetObjects.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <GuiFoundation/UIServices/DynamicEnums.h>
#include <PhysXCooking/PhysXCooking.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>

void OnLoadPlugin()
{
  ezQtEditorApp::GetSingleton()->AddRuntimePluginDependency("EditorPluginTypeScript", "ezTypeScriptPlugin");

  ezTypeScriptActions::RegisterActions();

  // TypeScript
  {
    // Menu Bar
    {
      ezActionMapManager::RegisterActionMap("TypeScriptAssetMenuBar").IgnoreResult();
      ezProjectActions::MapActions("TypeScriptAssetMenuBar");
      ezStandardMenus::MapActions("TypeScriptAssetMenuBar", ezStandardMenuTypes::File | ezStandardMenuTypes::Edit | ezStandardMenuTypes::Panels | ezStandardMenuTypes::Help);
      ezDocumentActions::MapActions("TypeScriptAssetMenuBar", "Menu.File", false);
      ezCommandHistoryActions::MapActions("TypeScriptAssetMenuBar", "Menu.Edit");
    }

    // Tool Bar
    {
      ezActionMapManager::RegisterActionMap("TypeScriptAssetToolBar").IgnoreResult();
      ezDocumentActions::MapActions("TypeScriptAssetToolBar", "", true);
      ezCommandHistoryActions::MapActions("TypeScriptAssetToolBar", "");
      ezAssetActions::MapActions("TypeScriptAssetToolBar", true);
      ezTypeScriptActions::MapActions("TypeScriptAssetToolBar", "");
    }
  }
}

void OnUnloadPlugin()
{
  ezTypeScriptActions::UnregisterActions();
}

EZ_PLUGIN_DEPENDENCY(ezEditorPluginScene);

// clang-format off
EZ_BEGIN_PLUGIN(ezEditorPluginTypeScript)

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
