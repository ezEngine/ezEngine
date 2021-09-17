#include <EditorPluginRecast/EditorPluginRecastPCH.h>

void OnLoadPlugin()
{
  ezQtEditorApp::GetSingleton()->AddRuntimePluginDependency("EditorPluginRecast", "ezRecastPlugin");
  ezQtEditorApp::GetSingleton()->AddRuntimePluginDependency("EditorPluginRecast", "ezEnginePluginRecast");
}

void OnUnloadPlugin() {}

// clang-format off
EZ_BEGIN_PLUGIN(ezEditorPluginRecast)

  BEGIN_PLUGIN_DEPENDENCIES
    "ezEditorPluginScene"
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
