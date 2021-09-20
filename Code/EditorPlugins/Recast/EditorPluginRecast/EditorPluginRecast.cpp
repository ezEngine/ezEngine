#include <EditorPluginRecast/EditorPluginRecastPCH.h>

void OnLoadPlugin()
{
  ezQtEditorApp::GetSingleton()->AddRuntimePluginDependency("EditorPluginRecast", "ezRecastPlugin");
  ezQtEditorApp::GetSingleton()->AddRuntimePluginDependency("EditorPluginRecast", "ezEnginePluginRecast");
}

void OnUnloadPlugin() {}

EZ_PLUGIN_DEPENDENCY(ezEditorPluginScene);

EZ_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

EZ_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}
