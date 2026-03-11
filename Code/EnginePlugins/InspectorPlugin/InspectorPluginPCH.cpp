#include <InspectorPlugin/InspectorPluginPCH.h>

EZ_STATICLINK_LIBRARY(InspectorPlugin)
{
  if (bReturn)
    return;

  EZ_STATICLINK_REFERENCE(InspectorPlugin_InspectorPlugin);
  EZ_STATICLINK_REFERENCE(InspectorPlugin_Startup);
}
