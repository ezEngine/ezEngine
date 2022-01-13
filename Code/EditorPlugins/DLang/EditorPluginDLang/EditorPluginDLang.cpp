#include <EditorPluginDLang/EditorPluginDLangPCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginDLang/Actions/DLangActions.h>
#include <EditorPluginDLang/DLangAsset/DLangAssetObjects.h>
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
  ezQtEditorApp::GetSingleton()->AddRuntimePluginDependency("EditorPluginDLang", "ezDLangPlugin");

  ezDLangActions::RegisterActions();

  // DLang
  {
    // Menu Bar
    {
      ezActionMapManager::RegisterActionMap("DLangAssetMenuBar").IgnoreResult();
      ezProjectActions::MapActions("DLangAssetMenuBar");
      ezStandardMenus::MapActions("DLangAssetMenuBar", ezStandardMenuTypes::File | ezStandardMenuTypes::Edit | ezStandardMenuTypes::Panels | ezStandardMenuTypes::Help);
      ezDocumentActions::MapActions("DLangAssetMenuBar", "Menu.File", false);
      ezCommandHistoryActions::MapActions("DLangAssetMenuBar", "Menu.Edit");
    }

    // Tool Bar
    {
      ezActionMapManager::RegisterActionMap("DLangAssetToolBar").IgnoreResult();
      ezDocumentActions::MapActions("DLangAssetToolBar", "", true);
      ezCommandHistoryActions::MapActions("DLangAssetToolBar", "");
      ezAssetActions::MapActions("DLangAssetToolBar", true);
      ezDLangActions::MapActions("DLangAssetToolBar", "");
    }
  }
}

void OnUnloadPlugin()
{
  ezDLangActions::UnregisterActions();
}

EZ_PLUGIN_DEPENDENCY(ezEditorPluginScene);

EZ_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

EZ_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}
