#include <AngelScriptPlugin/AngelScriptPluginPCH.h>

#include <AngelScriptPlugin/AngelScriptPluginDLL.h>

#include <Foundation/Configuration/Plugin.h>

EZ_STATICLINK_LIBRARY(AngelScriptPlugin)
{
  if (bReturn)
    return;

  EZ_STATICLINK_REFERENCE(AngelScriptPlugin_Resources_AngelScriptResource);
  EZ_STATICLINK_REFERENCE(AngelScriptPlugin_Runtime_AsEngineSingleton);
  EZ_STATICLINK_REFERENCE(AngelScriptPlugin_Runtime_AsFunctionDispatch);
}
