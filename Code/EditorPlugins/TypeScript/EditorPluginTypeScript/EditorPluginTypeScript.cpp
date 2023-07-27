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
  ezTypeScriptActions::RegisterActions();

  // TypeScript
  {
    // Menu Bar
    {
      ezActionMapManager::RegisterActionMap("TypeScriptAssetMenuBar").IgnoreResult();
      ezStandardMenus::MapActions("TypeScriptAssetMenuBar", ezStandardMenuTypes::Default | ezStandardMenuTypes::Edit);
      ezProjectActions::MapActions("TypeScriptAssetMenuBar");
      ezDocumentActions::MapMenuActions("TypeScriptAssetMenuBar");
      ezAssetActions::MapMenuActions("TypeScriptAssetMenuBar");
      ezCommandHistoryActions::MapActions("TypeScriptAssetMenuBar");
    }

    // Tool Bar
    {
      ezActionMapManager::RegisterActionMap("TypeScriptAssetToolBar").IgnoreResult();
      ezDocumentActions::MapToolbarActions("TypeScriptAssetToolBar");
      ezCommandHistoryActions::MapActions("TypeScriptAssetToolBar", "");
      ezAssetActions::MapToolBarActions("TypeScriptAssetToolBar", true);
      ezTypeScriptActions::MapActions("TypeScriptAssetToolBar");
    }
  }
}

void OnUnloadPlugin()
{
  ezTypeScriptActions::UnregisterActions();
}

EZ_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

EZ_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}
