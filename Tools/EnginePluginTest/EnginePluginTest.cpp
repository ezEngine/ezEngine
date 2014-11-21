#include <PCH.h>
#include <EnginePluginTest/EnginePluginTest.h>

void OnLoadPlugin(bool bReloading)    
{

}

void OnUnloadPlugin(bool bReloading)  
{
}

ezPlugin g_Plugin(false, OnLoadPlugin, OnUnloadPlugin);

EZ_DYNAMIC_PLUGIN_IMPLEMENTATION(ezEnginePluginTest);
