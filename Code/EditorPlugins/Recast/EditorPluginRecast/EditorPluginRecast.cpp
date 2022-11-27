#include <EditorPluginRecast/EditorPluginRecastPCH.h>

void OnLoadPlugin()
{
}

void OnUnloadPlugin() {}

EZ_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

EZ_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}
