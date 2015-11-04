#include <GameFoundation/PCH.h>





EZ_STATICLINK_LIBRARY(GameFoundation)
{
  if (bReturn)
    return;

  EZ_STATICLINK_REFERENCE(GameFoundation_DefaultResources);
  EZ_STATICLINK_REFERENCE(GameFoundation_GameApplication);
  EZ_STATICLINK_REFERENCE(GameFoundation_GameState);
}

