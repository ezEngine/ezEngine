#include <PhysXPlugin/PCH.h>
#include <Core/PCH.h>

EZ_STATICLINK_LIBRARY(PhysX)
{
  if (bReturn)
    return;
}

void OnLoadPlugin(bool bReloading) { }
void OnUnloadPlugin(bool bReloading) { }

ezPlugin g_Plugin(false, OnLoadPlugin, OnUnloadPlugin);

EZ_DYNAMIC_PLUGIN_IMPLEMENTATION(ezPhysXPlugin);