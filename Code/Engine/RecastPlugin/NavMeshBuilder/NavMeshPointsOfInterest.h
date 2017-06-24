#pragma once

#include <RecastPlugin/Basics.h>
#include <GameEngine/AI/PointOfInterestGraph.h>

struct rcPolyMesh;

struct EZ_RECASTPLUGIN_DLL ezNavMeshPointsOfInterest
{
  ezVec3 m_vFloorPosition;
  ezUInt32 m_uiVisibleMarkerLow = 0;
  ezUInt32 m_uiVisibleMarkerHigh = 0;
};

class EZ_RECASTPLUGIN_DLL ezNavMeshPointOfInterestGraph
{
public:
  ezNavMeshPointOfInterestGraph();
  ~ezNavMeshPointOfInterestGraph();

  void ExtractInterestPointsFromMesh(const rcPolyMesh& mesh, bool bReinitialize);

  ezPointOfInterestGraph<ezNavMeshPointsOfInterest>& GetGraph() { return m_NavMeshPointGraph; }
  const ezPointOfInterestGraph<ezNavMeshPointsOfInterest>& GetGraph() const { return m_NavMeshPointGraph; }

protected:
  ezPointOfInterestGraph<ezNavMeshPointsOfInterest> m_NavMeshPointGraph;
};
