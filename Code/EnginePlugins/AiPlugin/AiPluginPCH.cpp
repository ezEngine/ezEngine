#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/AiPluginDLL.h>

EZ_STATICLINK_LIBRARY(AiPlugin)
{
  if (bReturn)
    return;

  EZ_STATICLINK_REFERENCE(AiPlugin_Navigation_Components_NavMeshObstacleComponent);
  EZ_STATICLINK_REFERENCE(AiPlugin_Navigation_Components_NavMeshPathTestComponent);
  EZ_STATICLINK_REFERENCE(AiPlugin_Navigation_Components_NavigationComponent);
  EZ_STATICLINK_REFERENCE(AiPlugin_Navigation_Implementation_NavMeshWorldModule);
}
