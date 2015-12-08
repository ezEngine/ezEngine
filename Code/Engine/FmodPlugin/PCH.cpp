#include <FmodPlugin/PCH.h>
#include <Core/PCH.h>
#include <FmodPlugin/Basics.h>
#include <FmodPlugin/FmodSceneModule.h>

EZ_STATICLINK_LIBRARY(Fmod)
{
  if (bReturn)
    return;
}

void OnLoadPlugin(bool bReloading) { }
void OnUnloadPlugin(bool bReloading) { }

ezPlugin g_Plugin(false, OnLoadPlugin, OnUnloadPlugin);

EZ_DYNAMIC_PLUGIN_IMPLEMENTATION(EZ_FMODPLUGIN_DLL, ezFmodPlugin);