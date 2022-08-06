#include <EditorPluginAudioSystem/EditorPluginAudioSystemPCH.h>

#include <EditorPluginAudioSystem/Actions/AudioSystemActions.h>
//#include <EditorPluginAudioSystem/Preferences/AudioSystemPreferences.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>

#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/StandardMenus.h>


static void ToolsProjectEventHandler(const ezToolsProjectEvent& e);

void OnLoadPlugin()
{
  ezToolsProject::s_Events.AddEventHandler(ToolsProjectEventHandler);

  // Control Collection
  {
    // Menu Bar
    ezActionMapManager::RegisterActionMap("AudioControlCollectionAssetMenuBar").IgnoreResult();
    ezStandardMenus::MapActions("AudioControlCollectionAssetMenuBar", ezStandardMenuTypes::File | ezStandardMenuTypes::Edit | ezStandardMenuTypes::Panels | ezStandardMenuTypes::Help);
    ezProjectActions::MapActions("AudioControlCollectionAssetMenuBar");
    ezDocumentActions::MapActions("AudioControlCollectionAssetMenuBar", "Menu.File", false);
    ezCommandHistoryActions::MapActions("AudioControlCollectionAssetMenuBar", "Menu.Edit");

    // Tool Bar
    {
      ezActionMapManager::RegisterActionMap("AudioControlCollectionAssetToolBar").IgnoreResult();
      ezDocumentActions::MapActions("AudioControlCollectionAssetToolBar", "", true);
      ezCommandHistoryActions::MapActions("AudioControlCollectionAssetToolBar", "");
      ezAssetActions::MapActions("AudioControlCollectionAssetToolBar", true);
    }
  }

  // Scene
  {
    // Menu Bar
    {
      ezAudioSystemActions::RegisterActions();
      ezAudioSystemActions::MapMenuActions("EditorPluginScene_DocumentMenuBar");
      ezAudioSystemActions::MapMenuActions("EditorPluginScene_Scene2MenuBar");
      ezAudioSystemActions::MapToolbarActions("EditorPluginScene_DocumentToolBar");
      ezAudioSystemActions::MapToolbarActions("EditorPluginScene_Scene2ToolBar");
    }
  }
}

void OnUnloadPlugin()
{
  ezAudioSystemActions::UnregisterActions();
  ezToolsProject::s_Events.RemoveEventHandler(ToolsProjectEventHandler);
}

static void ToolsProjectEventHandler(const ezToolsProjectEvent& e)
{
  if (e.m_Type == ezToolsProjectEvent::Type::ProjectOpened)
  {
    ezLog::Info("Project Opened");
//    ezFmodProjectPreferences* pPreferences = ezPreferences::QueryPreferences<ezFmodProjectPreferences>();
//    pPreferences->SyncCVars();
  }
}

EZ_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

EZ_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}
