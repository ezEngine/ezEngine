#include <AiPlugin/Navigation/NavMesh.h>
#include <AiPlugin/Navigation/NavMeshQuery.h>

ezAiNavmeshQuery::ezAiNavmeshQuery()
{
  m_uiReinitQueryBit = 1;
}

void ezAiNavmeshQuery::SetNavmesh(ezAiNavMesh* pNavmesh)
{
  if (m_pNavmesh == pNavmesh)
    return;

  m_pNavmesh = pNavmesh;
  m_uiReinitQueryBit = 1;
}

void ezAiNavmeshQuery::SetQueryFilter(const dtQueryFilter& filter)
{
  if (m_pFilter == &filter)
    return;

  m_pFilter = &filter;
}

bool ezAiNavmeshQuery::PrepareQueryArea(const ezVec3& vCenter, float fRadius)
{
  EZ_ASSERT_DEV(m_pNavmesh != nullptr, "Navmesh has not been set.");
  return m_pNavmesh->RequestSector(vCenter.GetAsVec2(), ezVec2(fRadius));
}

bool ezAiNavmeshQuery::Raycast(const ezVec3& vStart, const ezVec3& vDir, float fDistance, ezAiNavmeshRaycastHit& out_raycastHit)
{
  if (m_uiReinitQueryBit)
  {
    EZ_ASSERT_DEV(m_pNavmesh != nullptr, "Navmesh has not been set.");
    EZ_ASSERT_DEV(m_pFilter != nullptr, "Navmesh filter has not been set.");

    m_uiReinitQueryBit = 0;
    m_Query.init(m_pNavmesh->GetDetourNavMesh(), 1);
  }

  // TODO: hardcoded 'epsilon'
  float he[3] = {2, 2, 2};

  dtPolyRef ref;
  float pt[3];

  if (dtStatusFailed(m_Query.findNearestPoly(ezRcPos(vStart), he, m_pFilter, &ref, pt)))
    return false;

  dtRaycastHit hit{};
  if (dtStatusFailed(m_Query.raycast(ref, ezRcPos(vStart), ezRcPos(vStart + vDir * fDistance), m_pFilter, 0, &hit)))
    return false;

  if (hit.t > 1.0f)
    return false;

  out_raycastHit.m_fHitDistanceNormalized = hit.t;
  out_raycastHit.m_fHitDistance = hit.t * fDistance;
  out_raycastHit.m_vHitPosition = vStart + (vDir * fDistance * hit.t);

  return true;
}
