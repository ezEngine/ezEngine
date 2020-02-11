#include <PCH.h>

#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <UltralightPlugin/Basics.h>

EZ_STATICLINK_LIBRARY(UltralightPlugin)
{
  if (bReturn)
    return;

  EZ_STATICLINK_REFERENCE(UltralightPlugin_UltralightStartup);
  EZ_STATICLINK_REFERENCE(UltralightPlugin_Resources_UltralightHTMLResource);
  EZ_STATICLINK_REFERENCE(UltralightPlugin_Resources_UltralightHTMLResourceLoader);

}

void OnLoadPlugin(bool bReloading)
{
}

void OnUnloadPlugin(bool bReloading)
{
}

ezPlugin g_Plugin(false, OnLoadPlugin, OnUnloadPlugin);
