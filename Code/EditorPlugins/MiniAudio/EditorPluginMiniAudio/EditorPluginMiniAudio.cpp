#include <EditorPluginMiniAudio/EditorPluginMiniAudioPCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorPluginMiniAudio/Actions/MiniAudioActions.h>
#include <EditorPluginMiniAudio/Preferences/MiniAudioPreferences.h>
#include <EditorPluginMiniAudio/SoundAsset/MiniAudioSoundAsset.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>

static void ToolsProjectEventHandler(const ezToolsProjectEvent& e);

void OnLoadPlugin()
{
  ezToolsProject::GetSingleton()->s_Events.AddEventHandler(ToolsProjectEventHandler);
  ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezMiniAudioSoundAssetProperties::PropertyMetaStateEventHandler);

  // Mesh
  {
    // Menu Bar
    ezActionMapManager::RegisterActionMap("MiniAudioSoundAssetMenuBar", "AssetMenuBar");

    // Tool Bar
    {
      ezActionMapManager::RegisterActionMap("MiniAudioSoundAssetToolBar", "AssetToolbar");
    }
  }

  // Scene
  {
    // Menu Bar
    {
      ezMiniAudioActions::RegisterActions();
      ezMiniAudioActions::MapMenuActions("EditorPluginScene_DocumentMenuBar");
      ezMiniAudioActions::MapMenuActions("EditorPluginScene_Scene2MenuBar");
      ezMiniAudioActions::MapToolbarActions("EditorPluginScene_DocumentToolBar");
      ezMiniAudioActions::MapToolbarActions("EditorPluginScene_Scene2ToolBar");
    }
  }
}

void OnUnloadPlugin()
{
  ezMiniAudioActions::UnregisterActions();
  ezToolsProject::GetSingleton()->s_Events.RemoveEventHandler(ToolsProjectEventHandler);
  ezPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(ezMiniAudioSoundAssetProperties::PropertyMetaStateEventHandler);
}

static void ToolsProjectEventHandler(const ezToolsProjectEvent& e)
{
  if (e.m_Type == ezToolsProjectEvent::Type::ProjectOpened)
  {
    ezMiniAudioProjectPreferences* pPreferences = ezPreferences::QueryPreferences<ezMiniAudioProjectPreferences>();
    pPreferences->SyncCVars();
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
