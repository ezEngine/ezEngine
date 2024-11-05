#include <RmlUiPlugin/RmlUiPluginPCH.h>

EZ_STATICLINK_LIBRARY(RmlUiPlugin)
{
  if (bReturn)
    return;

  EZ_STATICLINK_REFERENCE(RmlUiPlugin_Components_Implementation_RmlUiCanvas2DComponent);
  EZ_STATICLINK_REFERENCE(RmlUiPlugin_Components_Implementation_RmlUiMessages);
  EZ_STATICLINK_REFERENCE(RmlUiPlugin_Implementation_RmlUiRenderer);
  EZ_STATICLINK_REFERENCE(RmlUiPlugin_Resources_RmlUiResource);
  EZ_STATICLINK_REFERENCE(RmlUiPlugin_Startup);
}
