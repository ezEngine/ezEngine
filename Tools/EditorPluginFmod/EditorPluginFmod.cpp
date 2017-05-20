#include <PCH.h>
#include <EditorPluginFmod/EditorPluginFmod.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>
#include <Foundation/Reflection/Reflection.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorFramework/Actions/AssetActions.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <Foundation/Strings/TranslationLookup.h>

#include <EditorPluginFmod/Actions/FmodActions.h>
#include <GuiFoundation/UIServices/DynamicEnums.h>
#include <GameEngine/CollisionFilter/CollisionFilter.h>
#include <EditorPluginFmod/Preferences/FmodPreferences.h>

static void ToolsProjectEventHandler(const ezToolsProjectEvent& e);

void OnLoadPlugin(bool bReloading)
{
  ezQtEditorApp::GetSingleton()->AddRuntimePluginDependency("EditorPluginFmod", "ezFmodPlugin");

  ezToolsProject::GetSingleton()->s_Events.AddEventHandler(ToolsProjectEventHandler);
  
  // Mesh Asset
  {
    // Menu Bar
    {
      ezActionMapManager::RegisterActionMap("SoundBankAssetMenuBar");
      ezProjectActions::MapActions("SoundBankAssetMenuBar");
      ezStandardMenus::MapActions("SoundBankAssetMenuBar", ezStandardMenuTypes::File | ezStandardMenuTypes::Edit | ezStandardMenuTypes::Panels | ezStandardMenuTypes::Help);
      ezDocumentActions::MapActions("SoundBankAssetMenuBar", "Menu.File", false);
      ezCommandHistoryActions::MapActions("SoundBankAssetMenuBar", "Menu.Edit");
    }

    // Tool Bar
    {
      ezActionMapManager::RegisterActionMap("SoundBankAssetToolBar");
      ezDocumentActions::MapActions("SoundBankAssetToolBar", "", true);
      ezCommandHistoryActions::MapActions("SoundBankAssetToolBar", "");
      ezAssetActions::MapActions("SoundBankAssetToolBar", true);
    }
  }

  // Scene
  {
    // Menu Bar
    {
      ezFmodActions::RegisterActions();
      ezFmodActions::MapMenuActions();
    }

    // Tool Bar
    {

    }
  }
}

void OnUnloadPlugin(bool bReloading)
{
  ezFmodActions::UnregisterActions();
  ezToolsProject::GetSingleton()->s_Events.RemoveEventHandler(ToolsProjectEventHandler);
}

static void ToolsProjectEventHandler(const ezToolsProjectEvent& e)
{
  if (e.m_Type == ezToolsProjectEvent::Type::ProjectOpened)
  {
    ezFmodProjectPreferences* pPreferences = ezPreferences::QueryPreferences<ezFmodProjectPreferences>();
    pPreferences->SyncCVars();
  }
}


ezPlugin g_Plugin(false, OnLoadPlugin, OnUnloadPlugin, "ezEditorPluginScene");

EZ_DYNAMIC_PLUGIN_IMPLEMENTATION(EZ_EDITORPLUGINFMOD_DLL, ezEditorPluginFmod);



