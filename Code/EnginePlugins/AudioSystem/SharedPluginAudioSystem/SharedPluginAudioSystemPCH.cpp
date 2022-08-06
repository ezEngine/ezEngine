#include <SharedPluginAudioSystem/SharedPluginAudioSystemPCH.h>

EZ_STATICLINK_LIBRARY(SharedPluginAudioSystem)
{
  if (bReturn)
    return;

  EZ_STATICLINK_REFERENCE(SharedPluginAudioSystem_Middleware_AudioMiddlewareControlsManager);
}
