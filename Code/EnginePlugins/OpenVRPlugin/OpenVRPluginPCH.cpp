#include <OpenVRPlugin/OpenVRPluginPCH.h>

#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <OpenVRPlugin/Basics.h>

EZ_STATICLINK_LIBRARY(OpenVRPlugin)
{
  if (bReturn)
    return;

  EZ_STATICLINK_REFERENCE(OpenVRPlugin_OpenVRSingleton);
  EZ_STATICLINK_REFERENCE(OpenVRPlugin_OpenVRStartup);
}

EZ_DYNAMIC_PLUGIN_IMPLEMENTATION(EZ_OPENVRPLUGIN_DLL, ezOpenVRPlugin);
