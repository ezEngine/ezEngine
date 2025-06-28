#include <GameEngine/GameEnginePCH.h>

#include <Foundation/Math/Random.h>
#include <Foundation/Profiling/Profiling.h>
#include <GameEngine/Physics/Breakable2D.h>

#define JC_VORONOI_IMPLEMENTATION
#include <GameEngine/ThirdParty/jc_voronoi.h>

ezBreakable2D::ezBreakable2D() = default;
ezBreakable2D::~ezBreakable2D() = default;

void ezBreakable2D::Clear()
{
  m_Shards.Clear();
  m_fMaxRadius = 0.0f;
}

void ezBreakable2D::Initialize()
{
  Clear();

  m_Shards.SetCount(1);
  m_Shards[0].m_vCenterPosition.SetZero();
  m_Shards[0].m_fBoundingRadius = 5.0f;
  m_Shards[0].m_uiBreakablePatterns = (ezUInt8)ezBreakablePattern::All;
}

void ezBreakable2D::RemoveShard(ezUInt32 uiShardIdx)
{
  auto& shard = m_Shards[uiShardIdx];
  shard.m_bShattered = true;
}

void ezBreakable2D::ShatterShard(ezUInt32 uiShardIdx, const ezVec2& vShatterPosition, ezRandom& ref_rng, float fImpactRadius, float fCellSize, ezUInt8 uiAllowedBreakPatterns)
{
  EZ_PROFILE_SCOPE("ShatterShard");

  if (m_Shards[uiShardIdx].m_bShattered)
    return;

  m_Shards[uiShardIdx].m_bShattered = true;

  if (m_Shards[uiShardIdx].m_bDynamic)
  {
    // don't further shatter shards that are already dynamic
    // just remove them
    return;
  }

  uiAllowedBreakPatterns &= m_Shards[uiShardIdx].m_uiBreakablePatterns;
  if (uiAllowedBreakPatterns == (ezUInt8)ezBreakablePattern::None)
    return;

  ezHybridArray<ClipPlane, 6> clipPlanes;
  {
    const ezVec3 vNormal = ezVec3::MakeAxisZ();
    const auto& shard = m_Shards[uiShardIdx];

    ezUInt32 uiPrevIdx = shard.m_Edges.GetCount() - 1;

    for (ezUInt32 i = 0; i < shard.m_Edges.GetCount(); ++i)
    {
      ClipPlane& cp = clipPlanes.ExpandAndGetRef();
      if (cp.m_Plane.SetFromPoints(shard.m_Edges[i].m_vStartPosition.GetAsVec3(0), shard.m_Edges[uiPrevIdx].m_vStartPosition.GetAsVec3(0), shard.m_Edges[i].m_vStartPosition.GetAsVec3(0) + vNormal).Failed())
      {
        clipPlanes.PopBack();
        continue;
      }

      cp.m_uiOutsideShardIdx = shard.m_Edges[uiPrevIdx].m_uiOutsideShardIdx;
      uiPrevIdx = i;
    }
  }

  const ezUInt32 uiPrevShardCount = m_Shards.GetCount();

  if ((uiAllowedBreakPatterns & (ezUInt8)ezBreakablePattern::Radial) != 0)
  {
    ShatterWithRadialPattern(clipPlanes, vShatterPosition, ref_rng, fImpactRadius);
  }
  else if ((uiAllowedBreakPatterns & (ezUInt8)ezBreakablePattern::Cellular) != 0)
  {
    ShatterWithCellularPattern(uiShardIdx, clipPlanes, ref_rng, fCellSize);
  }
  else
  {
    EZ_ASSERT_NOT_IMPLEMENTED;
  }

  if (uiPrevShardCount == m_Shards.GetCount())
  {
    // too small for shatter size -> keep it, mark it as not-shatterable
    m_Shards[uiShardIdx].m_bShattered = false;
    m_Shards[uiShardIdx].m_uiBreakablePatterns = 0;
  }
}

void ezBreakable2D::ShatterAll(float fShardSize, ezRandom& ref_rng, bool bMakeAllDynamic)
{
  const ezUInt32 uiNumShards = m_Shards.GetCount();

  for (ezUInt32 i = 0; i < uiNumShards; ++i)
  {
    auto& shard = m_Shards[i];
    if (shard.m_bShattered || shard.m_bDynamic)
      continue;

    if ((shard.m_uiBreakablePatterns & (ezUInt8)ezBreakablePattern::Cellular) == 0)
      continue;

    ShatterShard(i, ezVec2::MakeZero(), ref_rng, 0.0f, fShardSize, (ezUInt8)ezBreakablePattern::Cellular);
  }

  if (bMakeAllDynamic)
  {
    for (ezUInt32 i = 0; i < m_Shards.GetCount(); ++i)
    {
      m_Shards[i].m_bDynamic = true;
    }
  }
}

void ezBreakable2D::ShatterWithRadialPattern(ezArrayPtr<const ClipPlane> clipPlanes, const ezVec2& vShatterPosition, ezRandom& ref_rng, float fImpactRadius)
{
  const float fMinAngle = 15.0f;
  const float fMaxAngle = 30.0f;

  ezHybridArray<ezAngle, 32> angles;
  float fRemainingAngle = 360.0f;

  while (fRemainingAngle > fMaxAngle)
  {
    const float fAngle = ref_rng.FloatMinMax(fMinAngle, fMaxAngle);
    fRemainingAngle -= fAngle;
    angles.PushBack(ezAngle::MakeFromDegree(fAngle));
  }
  angles.PushBack(ezAngle::MakeFromDegree(fRemainingAngle));

  const ezUInt32 uiRingDetail = angles.GetCount();

  ezHybridArray<ezQuat, 32> qRots;
  ezHybridArray<float, 32> radii1;
  ezHybridArray<float, 32> radii2;
  ezHybridArray<float, 32> radii3;
  ezHybridArray<float, 32> radii4;
  ezHybridArray<float, 32> radii5;
  ezHybridArray<float, 32> radii6;
  ezHybridArray<ezVec2, 16> ring0, ring1;
  ezHybridArray<ezUInt32, 16> shardIDs0, shardIDs1;

  radii1.Reserve(uiRingDetail);
  radii2.Reserve(uiRingDetail);
  radii3.Reserve(uiRingDetail);
  radii4.Reserve(uiRingDetail);
  radii5.Reserve(uiRingDetail);
  radii6.Reserve(uiRingDetail);

  for (ezUInt32 i = 0; i < uiRingDetail; ++i)
  {
    const float r1i = 1.0f;
    const float r1o = r1i * 1.5f;

    const float r2i = r1i * 2.5f;
    const float r2o = r2i * 1.5f;

    const float r3i = r2i * 2.5f;
    const float r3o = r3i * 1.5f;

    const float r4i = r3i * 2.5f;
    const float r4o = r4i * 1.5f;

    const float r5i = r4i * 2.5f;
    const float r5o = r5i * 1.5f;

    qRots.PushBack(ezQuat::MakeFromAxisAndAngle(ezVec3::MakeAxisZ(), angles[i]));
    radii1.PushBack(ref_rng.FloatMinMax(r1i * fImpactRadius, r1o * fImpactRadius));
    radii2.PushBack(ref_rng.FloatMinMax(r2i * fImpactRadius, r2o * fImpactRadius));
    radii3.PushBack(ref_rng.FloatMinMax(r3i * fImpactRadius, r3o * fImpactRadius));
    radii4.PushBack(ref_rng.FloatMinMax(r4i * fImpactRadius, r4o * fImpactRadius));
    radii5.PushBack(ref_rng.FloatMinMax(r5i * fImpactRadius, r5o * fImpactRadius));
    radii6.PushBack(100.0f);
    shardIDs0.PushBack(ezInvalidIndex);
  }

  GenerateRingVertices(ring1, vShatterPosition, radii6, qRots);

  GenerateRingVertices(ring0, vShatterPosition, radii5, qRots);
  GenerateRingShards(clipPlanes, ring0, ring1, shardIDs0, shardIDs1);

  GenerateRingVertices(ring1, vShatterPosition, radii4, qRots);
  GenerateRingShards(clipPlanes, ring1, ring0, shardIDs1, shardIDs0);

  GenerateRingVertices(ring0, vShatterPosition, radii3, qRots);
  GenerateRingShards(clipPlanes, ring0, ring1, shardIDs0, shardIDs1);

  GenerateRingVertices(ring1, vShatterPosition, radii2, qRots);
  GenerateRingShards(clipPlanes, ring1, ring0, shardIDs1, shardIDs0);

  GenerateRingVertices(ring0, vShatterPosition, radii1, qRots);
  GenerateRingShards(clipPlanes, ring0, ring1, shardIDs0, shardIDs1);
}

void ezBreakable2D::GenerateRingVertices(ezDynamicArray<ezVec2>& vertices, const ezVec2& vCenter, ezArrayPtr<float> radii, const ezArrayPtr<ezQuat> qRotations)
{
  vertices.Clear();

  ezVec2 vSide(1, 0);

  const ezUInt32 uiRingDetail = radii.GetCount();

  for (ezUInt32 i = 0; i < uiRingDetail; ++i)
  {
    const float fRadius = radii[i];

    const ezVec2 vPos1 = vCenter + vSide * fRadius;
    vSide = (qRotations[i] * vSide.GetAsVec3(0)).GetAsVec2();
    const ezVec2 vPos2 = vCenter + vSide * fRadius;

    vertices.PushBack(vPos1);
    vertices.PushBack(vPos2);
  }
}

void ezBreakable2D::GenerateRingShards(ezArrayPtr<const ClipPlane> clipPlanes, ezArrayPtr<ezVec2> innerVertices, ezArrayPtr<ezVec2> outerVertices, ezArrayPtr<ezUInt32> prevShardIDs, ezDynamicArray<ezUInt32>& out_ShardIDs)
{
  out_ShardIDs.Clear();

  ezHybridArray<ezBreakableShard2D::Edge, 6> shape;

  const ezUInt32 uiRingDetail = innerVertices.GetCount() / 2;

  for (ezUInt32 i = 0; i < uiRingDetail; ++i)
  {
    shape.Clear();

    auto& e0 = shape.ExpandAndGetRef();
    e0.m_vStartPosition = innerVertices[i * 2];
    e0.m_uiOutsideShardIdx = ezInvalidIndex; // not connected to the inner ring

    auto& e1 = shape.ExpandAndGetRef();
    e1.m_vStartPosition = innerVertices[i * 2 + 1];
    e1.m_uiOutsideShardIdx = ezInvalidIndex; // would need to be connected to the next shard, but we don't know whether that even will exist

    auto& e2 = shape.ExpandAndGetRef();
    e2.m_vStartPosition = outerVertices[i * 2 + 1];
    e2.m_uiOutsideShardIdx = prevShardIDs[i]; // connected to the outer ring

    auto& e3 = shape.ExpandAndGetRef();
    e3.m_vStartPosition = outerVertices[i * 2];
    e3.m_uiOutsideShardIdx = (i > 0) ? out_ShardIDs.PeekBack() : ezInvalidIndex; // connected to the previous shard

    out_ShardIDs.PushBack(AddShard(clipPlanes, shape));
  }
}

ezUInt32 ezBreakable2D::AddShard(ezArrayPtr<const ClipPlane> clipPlanes, ezArrayPtr<ezBreakableShard2D::Edge> shape)
{
  ezHybridArray<ezBreakableShard2D::Edge, 16> input(shape);
  ezHybridArray<ezBreakableShard2D::Edge, 16> output;

  for (ezUInt32 pIdx = 0; pIdx < clipPlanes.GetCount(); ++pIdx)
  {
    if (input.GetCount() < 3)
      return ezInvalidIndex;

    const ezPlane& clipPlane = clipPlanes[pIdx].m_Plane;

    output.Clear();

    const ezUInt32 uiNumVtx = input.GetCount();
    ezUInt32 uiPrevVtx = uiNumVtx - 1;

    for (ezUInt32 uiCurVtx = 0; uiCurVtx < uiNumVtx; ++uiCurVtx)
    {
      const auto& edge1 = input[uiPrevVtx];
      const auto& edge2 = input[uiCurVtx];
      const ezVec3 v1 = edge1.m_vStartPosition.GetAsVec3(0);
      const ezVec3 v2 = edge2.m_vStartPosition.GetAsVec3(0);

      uiPrevVtx = uiCurVtx;

      const bool bInside1 = clipPlane.GetPointPosition(v1) == ezPositionOnPlane::Back;
      const bool bInside2 = clipPlane.GetPointPosition(v2) == ezPositionOnPlane::Back;

      if (bInside1)
      {
        {
          auto& res = output.ExpandAndGetRef();
          res.m_vStartPosition = v1.GetAsVec2();
          res.m_uiOutsideShardIdx = edge1.m_uiOutsideShardIdx;
        }

        if (!bInside2)
        {
          ezVec3 vIntersection;
          if (clipPlane.GetRayIntersectionBiDirectional(v1, v2 - v1, nullptr, &vIntersection))
          {
            auto& res = output.ExpandAndGetRef();
            res.m_vStartPosition = vIntersection.GetAsVec2();
            res.m_uiOutsideShardIdx = clipPlanes[pIdx].m_uiOutsideShardIdx;
          }
        }
      }
      else
      {
        if (bInside2)
        {
          ezVec3 vIntersection;
          if (clipPlane.GetRayIntersectionBiDirectional(v1, v2 - v1, nullptr, &vIntersection))
          {
            auto& res = output.ExpandAndGetRef();
            res.m_vStartPosition = vIntersection.GetAsVec2();
            res.m_uiOutsideShardIdx = edge1.m_uiOutsideShardIdx;
          }
        }
      }
    }

    input = output;
  }

  if (output.GetCount() > 12)
  {
    // clamp maximum detail
    output.SetCount(12);
  }

  if (output.GetCount() <= 2)
    return ezInvalidIndex;

  auto& shard = m_Shards.ExpandAndGetRef();
  shard.m_Edges = output;

  ezBoundingBox bbox = ezBoundingBox::MakeInvalid();

  constexpr float fCellularLength = ezMath::Square(0.35f); // any edge must be longer
  constexpr float fGlassMinLength = ezMath::Square(0.2f);  // no edge must be shorter
  constexpr float fGlassMaxLength = ezMath::Square(0.4f);  // any edge must be longer

  bool bAllowGlassMin = true;
  bool bAllowGlassMax = false;
  bool bAllowCellular = false;

  ezUInt32 uiPrevEdge = shard.m_Edges.GetCount() - 1;
  for (ezUInt32 e = 0; e < shard.m_Edges.GetCount(); ++e)
  {
    const auto& edge = shard.m_Edges[e];
    bbox.ExpandToInclude(edge.m_vStartPosition.GetAsVec3(0));

    const float fEdgeLengthSqr = (edge.m_vStartPosition - shard.m_Edges[uiPrevEdge].m_vStartPosition).GetLengthSquared();
    uiPrevEdge = e;

    if (fEdgeLengthSqr < fGlassMinLength)
      bAllowGlassMin = false;
    if (fEdgeLengthSqr > fGlassMaxLength)
      bAllowGlassMax = true;

    if (fEdgeLengthSqr > fCellularLength)
      bAllowCellular = true;
  }

  shard.m_uiBreakablePatterns = 0;
  if (bAllowGlassMin && bAllowGlassMax)
    shard.m_uiBreakablePatterns |= (ezUInt8)ezBreakablePattern::Radial;
  if (bAllowCellular)
    shard.m_uiBreakablePatterns |= (ezUInt8)ezBreakablePattern::Cellular;

  shard.m_vCenterPosition = bbox.GetCenter().GetAsVec2();
  shard.m_fBoundingRadius = (bbox.m_vMax - bbox.m_vMin).GetLength() * 0.5f;

  m_fMaxRadius = ezMath::Max(m_fMaxRadius, shard.m_fBoundingRadius);

  return m_Shards.GetCount() - 1;
}

void ezBreakable2D::RecalculateDymamic()
{
  m_fMaxRadius = 0.0f;

  for (ezUInt32 i = 0; i < m_Shards.GetCount(); ++i)
  {
    auto& shard = m_Shards[i];
    if (shard.m_bShattered)
      continue;

    m_fMaxRadius = ezMath::Max(m_fMaxRadius, shard.m_fBoundingRadius);

    if (shard.m_bDynamic)
      continue;

    bool bHasSupport = false;

    for (ezUInt32 e = 0; e < shard.m_Edges.GetCount(); ++e)
    {
      const ezUInt32 outIdx = shard.m_Edges[e].m_uiOutsideShardIdx;

      if (outIdx == ezInvalidIndex)
        continue;

      if (outIdx == ezBreakableShard2D::FixedEdge)
      {
        bHasSupport = true;
        break;
      }

      if (outIdx > i)
      {
        // only consider shards that we already updated before
        continue;
      }

      if (!m_Shards[outIdx].m_bShattered && !m_Shards[outIdx].m_bDynamic)
      {
        bHasSupport = true;
        break;
      }
    }

    if (!bHasSupport)
    {
      shard.m_bDynamic = true;
    }
  }
}

EZ_DEFINE_AS_POD_TYPE(jcv_point);

bool ezBreakable2D::ShatterWithCellularPattern(ezUInt32 uiShardIdx, ezArrayPtr<const ClipPlane> clipPlanes, ezRandom& ref_rng, float fShardSize)
{
  const ezBreakableShard2D& shard = m_Shards[uiShardIdx];

  ezBoundingBox box = ezBoundingBox::MakeInvalid();
  for (const auto& edge : shard.m_Edges)
  {
    box.ExpandToInclude(edge.m_vStartPosition.GetAsVec3(0));
  }

  const ezVec2 pointCenter = box.GetCenter().GetAsVec2();
  const ezVec2 halfext = box.GetHalfExtents().GetAsVec2();
  const ezVec2 pointBounds = halfext * 0.9f;
  const ezVec2 pointStep = ezVec2(ezMath::Clamp(fShardSize, 0.2f, 2.0f));
  const ezVec2 variation = pointStep * 0.7f;

  ezHybridArray<jcv_point, 64> diagramPoints;

  for (float y = pointCenter.y - pointBounds.y; y < pointCenter.y + pointBounds.y; y += pointStep.y)
  {
    for (float x = pointCenter.x - pointBounds.x; x < pointCenter.x + pointBounds.x; x += pointStep.x)
    {
      const float cx = static_cast<float>(ref_rng.DoubleMinMax(x, x + variation.x));
      const float cy = static_cast<float>(ref_rng.DoubleMinMax(y, y + variation.y));
      diagramPoints.PushBack({cx, cy});
    }
  }

  // too small area?
  if (diagramPoints.GetCount() < 3)
    return false;

  jcv_rect boundingBox;
  boundingBox.min.x = box.m_vMin.x - 1;
  boundingBox.min.y = box.m_vMin.y - 1;
  boundingBox.max.x = box.m_vMax.x + 1;
  boundingBox.max.y = box.m_vMax.y + 1;

  jcv_diagram diagram;
  ezMemoryUtils::ZeroFill(&diagram, 1);
  jcv_diagram_generate(diagramPoints.GetCount(), diagramPoints.GetData(), &boundingBox, nullptr, &diagram);

  if (diagram.numsites == 0)
    return false;

  ezHybridArray<ezBreakableShard2D::Edge, 12> shape, shapeInv;

  const jcv_site* sites = jcv_diagram_get_sites(&diagram);

  ezMap<const jcv_site*, ezUInt32> ptrToShard;

  for (int i = 0; i < diagram.numsites; ++i)
  {
    shape.Clear();

    const jcv_site* site = &sites[i];
    const jcv_graphedge* e = site->edges;

    while (e)
    {
      auto& outEdge = shape.ExpandAndGetRef();
      outEdge.m_vStartPosition.Set(e->pos[0].x, e->pos[0].y);

      auto pBuddy = e->neighbor;

      auto itBuddy = ptrToShard.Find(pBuddy);
      if (itBuddy.IsValid())
      {
        outEdge.m_uiOutsideShardIdx = itBuddy.Value();
      }

      e = e->next;
    }

    shapeInv.Clear();
    shapeInv.SetCount(shape.GetCount());
    ezUInt32 i2 = shape.GetCount() - 1;
    for (ezUInt32 i = 0; i < shape.GetCount(); ++i)
    {
      shapeInv[i2] = shape[i];
      --i2;
    }

    ptrToShard[site] = AddShard(clipPlanes, shapeInv);
  }

  return true;
}
