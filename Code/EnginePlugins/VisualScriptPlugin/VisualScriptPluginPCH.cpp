#include <VisualScriptPlugin/VisualScriptPluginPCH.h>

#include <VisualScriptPlugin/VisualScriptPluginDLL.h>

#include <Foundation/Configuration/Plugin.h>

EZ_STATICLINK_LIBRARY(VisualScriptPlugin)
{
  if (bReturn)
    return;

  EZ_STATICLINK_REFERENCE(VisualScriptPlugin_Resources_VisualScriptClassResource);
  EZ_STATICLINK_REFERENCE(VisualScriptPlugin_Runtime_VisualScript);
  EZ_STATICLINK_REFERENCE(VisualScriptPlugin_Runtime_VisualScriptDataType);
}
