#include <FmodPluginPCH.h>

#include <FmodPlugin/FmodPluginDLL.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Strings/TranslationLookup.h>

EZ_STATICLINK_LIBRARY(FmodPlugin)
{
  if (bReturn)
    return;

  EZ_STATICLINK_REFERENCE(FmodPlugin_Components_FmodComponent);
  EZ_STATICLINK_REFERENCE(FmodPlugin_Components_FmodEventComponent);
  EZ_STATICLINK_REFERENCE(FmodPlugin_Components_FmodListenerComponent);
  EZ_STATICLINK_REFERENCE(FmodPlugin_Components_FmodReverbComponent);
  EZ_STATICLINK_REFERENCE(FmodPlugin_FmodSingleton);
  EZ_STATICLINK_REFERENCE(FmodPlugin_FmodStartup);
  EZ_STATICLINK_REFERENCE(FmodPlugin_Resources_FmodSoundBankResource);
  EZ_STATICLINK_REFERENCE(FmodPlugin_Resources_FmodSoundBankResourceLoader);
  EZ_STATICLINK_REFERENCE(FmodPlugin_Resources_FmodSoundEventResource);
  EZ_STATICLINK_REFERENCE(FmodPlugin_Resources_FmodSoundEventResourceLoader);
}

ezPlugin g_Plugin(false);
