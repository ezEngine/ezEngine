#include <PCH.h>

EZ_STATICLINK_LIBRARY(InputXBox360)
{
  if (bReturn)
    return;

  EZ_STATICLINK_REFERENCE(InputXBox360_InputDeviceXBox);
  EZ_STATICLINK_REFERENCE(InputXBox360_Startup);
}

