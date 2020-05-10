#include <OpenXRPluginPCH.h>

#include <OpenXRPlugin/Basics.h>
#include <OpenXRPlugin/OpenXRIncludes.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Strings/TranslationLookup.h>

EZ_STATICLINK_LIBRARY(OpenXRPlugin)
{
  if (bReturn)
    return;

  EZ_STATICLINK_REFERENCE(OpenXRPlugin_OpenXRSingleton);
  EZ_STATICLINK_REFERENCE(OpenXRPlugin_OpenXRStartup);
}

void OnLoadPlugin(bool bReloading) {}
void OnUnloadPlugin(bool bReloading) {}

ezPlugin g_Plugin(false, OnLoadPlugin, OnUnloadPlugin);

