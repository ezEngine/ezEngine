#include <ParticlePlugin/PCH.h>
#include <Core/PCH.h>
#include <ParticlePlugin/Basics.h>

EZ_STATICLINK_LIBRARY(Particle)
{
  if (bReturn)
    return;
}

void OnLoadPlugin(bool bReloading) { }
void OnUnloadPlugin(bool bReloading) { }

ezPlugin g_Plugin(false, OnLoadPlugin, OnUnloadPlugin);

EZ_DYNAMIC_PLUGIN_IMPLEMENTATION(EZ_PARTICLEPLUGIN_DLL, ezParticlePlugin);