#include <EditorPluginKraut/EditorPluginKrautPCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorPluginKraut/Actions/KrautActions.h>
#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAssetObjects.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>

EZ_PLUGIN_ON_LOADED()
{
  ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezKrautTreeAssetProperties::PropertyMetaStateEventHandler);

  ezKrautActions::RegisterActions();

  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("KrautTreeAssetMenuBar", "AssetMenuBar");
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("KrautTreeAssetToolBar", "AssetToolbar");
    ezKrautActions::MapActions("KrautTreeAssetToolBar");
  }
}

EZ_PLUGIN_ON_UNLOADED()
{
  ezPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(ezKrautTreeAssetProperties::PropertyMetaStateEventHandler);
  ezKrautActions::UnregisterActions();
}
