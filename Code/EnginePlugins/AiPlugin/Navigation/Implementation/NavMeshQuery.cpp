#include <AiPlugin/Navigation/NavMesh.h>
#include <AiPlugin/Navigation/NavMeshQuery.h>

static constexpr ezUInt32 MaxSearchNodes = 16;

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
    m_Query.init(m_pNavmesh->GetDetourNavMesh(), MaxSearchNodes);
  }

  // TODO: hardcoded 'epsilon'
  float he[3] = {2, 2, 2};

  dtPolyRef ref;
  float pt[3];

  if (dtStatusFailed(m_Query.findNearestPoly(ezRcPos(vStart), he, m_pFilter, &ref, pt)))
    return false;

  if (ref == 0)
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

thread_local ezRandom* tl_pRandom = nullptr;

static float frand()
{
  return tl_pRandom->FloatZeroToOneInclusive();
}

bool ezAiNavmeshQuery::FindRandomPointAroundCircle(const ezVec3& vStart, float fRadius, ezRandom& ref_rng, ezVec3& out_vPoint)
{
  if (m_uiReinitQueryBit)
  {
    EZ_ASSERT_DEV(m_pNavmesh != nullptr, "Navmesh has not been set.");
    EZ_ASSERT_DEV(m_pFilter != nullptr, "Navmesh filter has not been set.");

    m_uiReinitQueryBit = 0;
    m_Query.init(m_pNavmesh->GetDetourNavMesh(), MaxSearchNodes);
  }

  // TODO: hardcoded 'epsilon'
  float he[3] = {2, 2, 2};

  dtPolyRef ref;
  float pt[3];

  if (dtStatusFailed(m_Query.findNearestPoly(ezRcPos(vStart), he, m_pFilter, &ref, pt)))
    return false;

  if (ref == 0)
    return false;

  tl_pRandom = &ref_rng;

  dtPolyRef resultRef;
  ezRcPos resPt;
  if (dtStatusFailed(m_Query.findRandomPointAroundCircle(ref, ezRcPos(vStart), fRadius, m_pFilter, frand, &resultRef, resPt)))
    return false;

  out_vPoint = resPt;
  return true;
}
