#include <EditorPluginRecastPCH.h>

#include <GuiFoundation/Action/Action.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/BaseActions.h>

void OnLoadPlugin(bool bReloading)
{
  ezQtEditorApp::GetSingleton()->AddRuntimePluginDependency("EditorPluginRecast", "ezRecastPlugin");
  ezQtEditorApp::GetSingleton()->AddRuntimePluginDependency("EditorPluginRecast", "ezEnginePluginRecast");
}

void OnUnloadPlugin(bool bReloading) {}

ezPlugin g_Plugin(false, OnLoadPlugin, OnUnloadPlugin, "ezEditorPluginScene");
