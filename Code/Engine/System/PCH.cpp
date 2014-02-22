#include <System/PCH.h>
#include <Foundation/PCH.h>
#include <Core/PCH.h>

EZ_STATICLINK_LIBRARY(System)
{
  if (bReturn)
    return;

  EZ_STATICLINK_REFERENCE(System_Window_Implementation_Window);
}

