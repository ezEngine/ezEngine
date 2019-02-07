#include <SystemPCH.h>

EZ_STATICLINK_LIBRARY(System)
{
  if (bReturn)
    return;

  EZ_STATICLINK_REFERENCE(System_Screen_Implementation_Screen);
  EZ_STATICLINK_REFERENCE(System_Window_Implementation_Window);
  EZ_STATICLINK_REFERENCE(System_XBoxController_InputDeviceXBox);
  EZ_STATICLINK_REFERENCE(System_XBoxController_Startup);
}

