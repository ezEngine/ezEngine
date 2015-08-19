#include <PCH.h>
#include <EnginePluginScene/Plugin.h>

void OnLoadPlugin(bool bReloading)
{
  int i = 0;
}

void OnUnloadPlugin(bool bReloading)  
{

}

ezPlugin g_Plugin(false, OnLoadPlugin, OnUnloadPlugin);

EZ_DYNAMIC_PLUGIN_IMPLEMENTATION(ezEnginePluginScene);
