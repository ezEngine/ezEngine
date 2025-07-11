#pragma once

#include <AiPlugin/Navigation/NavMesh.h>
#include <DetourNavMeshQuery.h>
#include <Foundation/Math/Vec3.h>

/// \brief Contains information about a raycast hit in a navmesh
struct EZ_AIPLUGIN_DLL ezAiNavmeshRaycastHit
{
  ezVec3 m_vHitPosition;
  float m_fHitDistanceNormalized;
  float m_fHitDistance;
};

/// \brief Allows to do queries on a navmesh.
class EZ_AIPLUGIN_DLL ezAiNavmeshQuery
{
public:
  ezAiNavmeshQuery();

  ezAiNavMesh* GetNavmesh() const { return m_pNavmesh; }

  /// \brief Sets on which navmesh to do the queries.
  ///
  /// \see ezAiNavMeshWorldModule::GetNavMesh()
  void SetNavmesh(ezAiNavMesh* pNavmesh);

  /// \brief Sets the filter to use on the navmesh to ignore certain areas.
  ///
  /// \see ezAiNavMeshWorldModule::GetPathSearchFilter()
  void SetQueryFilter(const dtQueryFilter& filter);

  /// \brief Checks that the given area of the navmesh is loaded, such that query results are useful.
  ///
  /// Returns false, if some navmesh sector is not yet available.
  /// It will be put into a queue and generated over the next frames.
  bool PrepareQueryArea(const ezVec3& vCenter, float fRadius);

  /// \brief Does a raycast along the navmesh from the start position into a given direction.
  ///
  /// Returns true, if a navmesh edge has been hit and the result struct was filled with details.
  bool Raycast(const ezVec3& vStart, const ezVec3& vDir, float fDistance, ezAiNavmeshRaycastHit& out_raycastHit);

  /// \brief Attempts to find a random point on the navmesh. The circle limits which navmesh polygons are visited.
  ///
  /// The result may be outside the circle, if the circle overlaps with a large navmesh polygon.
  bool FindRandomPointAroundCircle(const ezVec3& vStart, float fRadius, ezRandom& ref_rng, ezVec3& out_vPoint);

private:
  ezUInt8 m_uiReinitQueryBit : 1;

  ezAiNavMesh* m_pNavmesh = nullptr;
  dtNavMeshQuery m_Query;
  const dtQueryFilter* m_pFilter = nullptr;
};
