#include <RecastPlugin/RecastPluginPCH.h>

#include <RecastPlugin/RecastPluginDLL.h>

EZ_STATICLINK_LIBRARY(RecastPlugin)
{
  if (bReturn)
    return;

  EZ_STATICLINK_REFERENCE(RecastPlugin_Components_AgentSteeringComponent);
  EZ_STATICLINK_REFERENCE(RecastPlugin_Components_DetourCrowdAgentComponent);
  EZ_STATICLINK_REFERENCE(RecastPlugin_Components_MarkPoiVisibleComponent);
  EZ_STATICLINK_REFERENCE(RecastPlugin_Components_NpcComponent);
  EZ_STATICLINK_REFERENCE(RecastPlugin_Components_RecastAgentComponent);
  EZ_STATICLINK_REFERENCE(RecastPlugin_Components_RecastNavMeshComponent);
  EZ_STATICLINK_REFERENCE(RecastPlugin_Components_SoldierComponent);
  EZ_STATICLINK_REFERENCE(RecastPlugin_NavMeshBuilder_NavMeshBuilder);
  EZ_STATICLINK_REFERENCE(RecastPlugin_Resources_RecastNavMeshResource);
  EZ_STATICLINK_REFERENCE(RecastPlugin_Startup);
  EZ_STATICLINK_REFERENCE(RecastPlugin_WorldModule_DetourCrowdWorldModule);
  EZ_STATICLINK_REFERENCE(RecastPlugin_WorldModule_RecastWorldModule);
}
