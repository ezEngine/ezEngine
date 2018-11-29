#include <PCH.h>

#include <OpenVRPlugin/Basics.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Strings/TranslationLookup.h>

EZ_STATICLINK_LIBRARY(OpenVRPlugin)
{
  if (bReturn)
    return;

  EZ_STATICLINK_REFERENCE(OpenVRPlugin_OpenVRSingleton);
  EZ_STATICLINK_REFERENCE(OpenVRPlugin_OpenVRStartup);
}

void OnLoadPlugin(bool bReloading) {}
void OnUnloadPlugin(bool bReloading) {}

ezPlugin g_Plugin(false, OnLoadPlugin, OnUnloadPlugin);

EZ_DYNAMIC_PLUGIN_IMPLEMENTATION(EZ_OPENVRPLUGIN_DLL, ezOpenVRPlugin);
