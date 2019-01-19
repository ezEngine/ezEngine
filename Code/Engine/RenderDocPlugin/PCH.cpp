#include <PCH.h>

#include <RenderDocPlugin/Basics.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Strings/TranslationLookup.h>

EZ_STATICLINK_LIBRARY(RenderDocPlugin)
{
  if (bReturn)
    return;

  EZ_STATICLINK_REFERENCE(RenderDocPlugin_RenderDocSingleton);
}

void OnLoadPlugin(bool bReloading) {}
void OnUnloadPlugin(bool bReloading) {}

ezPlugin g_Plugin(false, OnLoadPlugin, OnUnloadPlugin);

EZ_DYNAMIC_PLUGIN_IMPLEMENTATION(EZ_RENDERDOCPLUGIN_DLL, ezRenderDocPlugin);
