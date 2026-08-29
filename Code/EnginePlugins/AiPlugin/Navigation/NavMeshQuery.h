#pragma once

#include <AiPlugin/Navigation/NavMesh.h>
#include <DetourNavMeshQuery.h>
#include <Foundation/Math/Vec3.h>

/// Contains information about a raycast hit in a navmesh
struct EZ_AIPLUGIN_DLL ezAiNavmeshRaycastHit
{
  ezVec3 m_vHitPosition;
  float m_fHitDistanceNormalized;
  float m_fHitDistance;
};

/// Allows to do queries on a navmesh.
class EZ_AIPLUGIN_DLL ezAiNavmeshQuery
{
public:
  ezAiNavmeshQuery();

  ezAiNavMesh* GetNavmesh() const { return m_pNavmesh; }

  /// Sets on which navmesh to do the queries.
  ///
  /// \see ezAiNavMeshWorldModule::GetNavMesh()
  void SetNavmesh(ezAiNavMesh* pNavmesh);

  /// Sets the filter to use on the navmesh to ignore certain areas.
  ///
  /// \see ezAiNavMeshWorldModule::GetPathSearchFilter()
  void SetQueryFilter(const dtQueryFilter& filter);

  /// Sets the half-extents of the search box used to snap an arbitrary position onto the nearest navmesh
  /// polygon (findNearestPoly). fExtentsXY is also used as the search radius for FindClosestPointOnNavmesh's wall
  /// query. Separate from the vertical (Z) extent, since navmeshes are usually thin (often a flat plane), so a
  /// horizontal search distance that's appropriate for e.g. how far inertia can carry something off the mesh would
  /// vastly over-match vertically if the same value were used for both.
  ///
  /// Applies to all queries made through this instance from now on. Pick this based on how far a queried position
  /// may realistically be off the navmesh, not per-call.
  void SetSearchExtents(float fExtentsXY, float fExtentsZ)
  {
    m_fSearchExtentsXY = fExtentsXY;
    m_fSearchExtentsZ = fExtentsZ;
  }
  float GetSearchExtentsXY() const { return m_fSearchExtentsXY; }
  float GetSearchExtentsZ() const { return m_fSearchExtentsZ; }

  /// Checks that the given area of the navmesh is loaded, such that query results are useful.
  ///
  /// Returns false, if some navmesh sector is not yet available.
  /// It will be put into a queue and generated over the next frames.
  bool PrepareQueryArea(const ezVec3& vCenter, float fRadius);

  /// Does a raycast along the navmesh from the start position into a given direction.
  ///
  /// Returns true, if a navmesh edge has been hit and the result struct was filled with details.
  bool Raycast(const ezVec3& vStart, const ezVec3& vDir, float fDistance, ezAiNavmeshRaycastHit& out_raycastHit);

  /// Attempts to find a random point on the navmesh. The circle limits which navmesh polygons are visited.
  ///
  /// The result may be outside the circle, if the circle overlaps with a large navmesh polygon.
  bool FindRandomPointAroundCircle(const ezVec3& vStart, float fRadius, ezRandom& ref_rng, ezVec3& out_vPoint);

  /// Finds the closest point on the navmesh to vPos, e.g. to clamp a position back onto walkable ground.
  ///
  /// out_pNormal, if provided, is filled with the direction from the navmesh border back towards the mesh interior
  /// (i.e. "inwards"), so that the caller can nudge the clamped point away from the border to counteract floating
  /// point imprecision. It is only set if a border could be determined; check the return value regardless.
  ///
  /// Returns false, if no navmesh polygon could be found near vPos at all.
  bool FindClosestPointOnNavmesh(const ezVec3& vPos, ezVec3& out_vPoint, ezVec3* out_pNormal = nullptr);

private:
  ezUInt8 m_uiReinitQueryBit : 1;

  float m_fSearchExtentsXY = 2.0f;
  float m_fSearchExtentsZ = 2.0f;

  ezAiNavMesh* m_pNavmesh = nullptr;
  dtNavMeshQuery m_Query;
  const dtQueryFilter* m_pFilter = nullptr;
};
