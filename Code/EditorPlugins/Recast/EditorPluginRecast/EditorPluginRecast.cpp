#include <EditorPluginRecastPCH.h>

#include <GuiFoundation/Action/Action.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/Action/ActionMapManager.h>

#include <EditorPluginRecast/Actions/RecastActions.h>

void OnLoadPlugin(bool bReloading)
{
  ezQtEditorApp::GetSingleton()->AddRuntimePluginDependency("EditorPluginRecast", "ezRecastPlugin");
  ezQtEditorApp::GetSingleton()->AddRuntimePluginDependency("EditorPluginRecast", "ezEnginePluginRecast");

  ezRecastActions::RegisterActions();

  ezRecastActions::MapActions("EditorPluginScene_DocumentMenuBar");
}

void OnUnloadPlugin(bool bReloading)
{
  ezRecastActions::UnregisterActions();
}

ezPlugin g_Plugin(false, OnLoadPlugin, OnUnloadPlugin, "ezEditorPluginScene");
