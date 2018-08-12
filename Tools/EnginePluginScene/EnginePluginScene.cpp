#include <PCH.h>

#include <EnginePluginScene/Plugin.h>

void OnLoadPlugin(bool bReloading) {}

void OnUnloadPlugin(bool bReloading) {}

ezPlugin g_Plugin(false, OnLoadPlugin, OnUnloadPlugin);

EZ_DYNAMIC_PLUGIN_IMPLEMENTATION(EZ_ENGINEPLUGINSCENE_DLL, ezEnginePluginScene);
