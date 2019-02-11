#include <RenderDocPluginPCH.h>

#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <RenderDocPlugin/RenderDocPluginDLL.h>

EZ_STATICLINK_LIBRARY(RenderDocPlugin)
{
  if (bReturn)
    return;

  EZ_STATICLINK_REFERENCE(RenderDocPlugin_RenderDocSingleton);
}

ezPlugin g_Plugin(false);
