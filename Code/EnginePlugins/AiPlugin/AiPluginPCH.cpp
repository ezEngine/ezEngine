#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/AiPluginDLL.h>

EZ_STATICLINK_LIBRARY(AiPlugin)
{
  if (bReturn)
    return;

  EZ_STATICLINK_REFERENCE(AiPlugin_Navigation3D_Implementation_VoxelGridComponent);
  EZ_STATICLINK_REFERENCE(AiPlugin_Navigation3D_Implementation_VoxelNavigationComponent);
  EZ_STATICLINK_REFERENCE(AiPlugin_Navigation3D_Implementation_VoxelPathTestComponent);
  EZ_STATICLINK_REFERENCE(AiPlugin_Navigation3D_Implementation_VoxelWorldModule);
  EZ_STATICLINK_REFERENCE(AiPlugin_Navigation_Components_DetourCrowdAgentComponent);
  EZ_STATICLINK_REFERENCE(AiPlugin_Navigation_Components_NavMeshObstacleComponent);
  EZ_STATICLINK_REFERENCE(AiPlugin_Navigation_Components_NavMeshPathTestComponent);
  EZ_STATICLINK_REFERENCE(AiPlugin_Navigation_Components_NavigationComponent);
  EZ_STATICLINK_REFERENCE(AiPlugin_Navigation_Implementation_NavMeshWorldModule);
}
