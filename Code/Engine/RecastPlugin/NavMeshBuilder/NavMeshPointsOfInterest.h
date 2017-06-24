#pragma once

#include <RecastPlugin/Basics.h>
#include <GameEngine/AI/PointOfInterestGraph.h>

struct rcPolyMesh;

struct EZ_RECASTPLUGIN_DLL ezNavMeshPointsOfInterest
{
  ezVec3 m_vFloorPosition;
  ezUInt32 m_uiVisibleMarker = 0;
};

class EZ_RECASTPLUGIN_DLL ezNavMeshPointOfInterestGraph
{
public:
  ezNavMeshPointOfInterestGraph();
  ~ezNavMeshPointOfInterestGraph();

  void ExtractInterestPointsFromMesh(const rcPolyMesh& mesh, bool bReinitialize);

  ezUInt32 GetCheckVisibilityTimeStamp() const { return m_uiCheckVisibilityTimeStamp; }
  void IncreaseCheckVisibiblityTimeStamp(ezTime tNow);

  ezPointOfInterestGraph<ezNavMeshPointsOfInterest>& GetGraph() { return m_NavMeshPointGraph; }
  const ezPointOfInterestGraph<ezNavMeshPointsOfInterest>& GetGraph() const { return m_NavMeshPointGraph; }

protected:
  ezTime m_LastTimeStampStep;
  ezUInt32 m_uiCheckVisibilityTimeStamp = 100;
  ezPointOfInterestGraph<ezNavMeshPointsOfInterest> m_NavMeshPointGraph;
};
