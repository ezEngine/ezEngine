#include <PCH.h>

EZ_STATICLINK_LIBRARY(InspectorPlugin)
{
  if (bReturn)
    return;

  EZ_STATICLINK_REFERENCE(InspectorPlugin_App);
  EZ_STATICLINK_REFERENCE(InspectorPlugin_CVars);
  EZ_STATICLINK_REFERENCE(InspectorPlugin_GlobalEvents);
  EZ_STATICLINK_REFERENCE(InspectorPlugin_Input);
  EZ_STATICLINK_REFERENCE(InspectorPlugin_Log);
  EZ_STATICLINK_REFERENCE(InspectorPlugin_Main);
  EZ_STATICLINK_REFERENCE(InspectorPlugin_Memory);
  EZ_STATICLINK_REFERENCE(InspectorPlugin_OSFile);
  EZ_STATICLINK_REFERENCE(InspectorPlugin_Plugins);
  EZ_STATICLINK_REFERENCE(InspectorPlugin_Startup);
  EZ_STATICLINK_REFERENCE(InspectorPlugin_Stats);
  EZ_STATICLINK_REFERENCE(InspectorPlugin_Time);
}

