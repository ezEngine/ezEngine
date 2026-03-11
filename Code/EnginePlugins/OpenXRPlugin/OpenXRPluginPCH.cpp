#include <OpenXRPlugin/OpenXRPluginPCH.h>

#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <OpenXRPlugin/Basics.h>
#include <OpenXRPlugin/OpenXRIncludes.h>

EZ_STATICLINK_LIBRARY(OpenXRPlugin)
{
  if (bReturn)
    return;

  EZ_STATICLINK_REFERENCE(OpenXRPlugin_OpenXRInputDevice);
}
