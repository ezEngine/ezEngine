#include <PacManPlugin/PacManPluginPCH.h>

EZ_STATICLINK_LIBRARY(PacManPlugin)
{
  if (bReturn)
    return;

  EZ_STATICLINK_REFERENCE(PacManPlugin_Components_GhostComponent);
  EZ_STATICLINK_REFERENCE(PacManPlugin_Components_PacManComponent);
  EZ_STATICLINK_REFERENCE(PacManPlugin_GameState_PacManGameState);
}
