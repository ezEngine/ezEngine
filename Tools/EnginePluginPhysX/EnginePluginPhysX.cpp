#include <PCH.h>
#include <EnginePluginPhysX/Plugin.h>

void OnLoadPlugin(bool bReloading)
{
}

void OnUnloadPlugin(bool bReloading)  
{
}

ezPlugin g_Plugin(false, OnLoadPlugin, OnUnloadPlugin);

EZ_DYNAMIC_PLUGIN_IMPLEMENTATION(EZ_ENGINEPLUGINPHYSX_DLL, ezEnginePluginPhysX);
