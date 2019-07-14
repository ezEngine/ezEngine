#include <RecastPluginPCH.h>

#include <Foundation/Containers/StaticArray.h>
#include <RecastPlugin/NavMeshBuilder/NavMeshPointsOfInterest.h>
#include <Recast/Recast.h>

ezNavMeshPointOfInterestGraph::ezNavMeshPointOfInterestGraph() = default;
ezNavMeshPointOfInterestGraph::~ezNavMeshPointOfInterestGraph() = default;

void ezNavMeshPointOfInterestGraph::IncreaseCheckVisibiblityTimeStamp(ezTime tNow)
{
  if (tNow - m_LastTimeStampStep < ezTime::Seconds(0.5f))
    return;

  m_LastTimeStampStep = tNow;
  m_uiCheckVisibilityTimeStamp += 4;
}

EZ_ALWAYS_INLINE static ezVec3 GetNavMeshVertex(const rcPolyMesh* pMesh, ezUInt16 uiVertex, const ezVec3& vMeshOrigin, float fCellSize,
                                                float fCellHeight)
{
  const ezUInt16* v = &pMesh->verts[uiVertex * 3];
  const float x = vMeshOrigin.x + v[0] * fCellSize;
  const float y = vMeshOrigin.y + v[2] * fCellSize;
  const float z = vMeshOrigin.z + v[1] * fCellHeight;

  return ezVec3(x, y, z);
}

const float toCenterOffset = 0.1f;
const float alongLineOffset = 0.5f;

struct PotentialPoI
{
  bool m_bUsed;
  ezVec3 m_vVertexPos;
  ezVec3 m_vPosition;
  ezVec3 m_vLineDir;
};

EZ_ALWAYS_INLINE static void AddToInterestPoints(ezDeque<PotentialPoI>& interestPoints, ezInt32 iVertexIdx, const ezVec3& pos,
                                                 const ezVec3& vPolyCenter, ezVec3 vLineDir)
{
  ezVec3 toCenter = vPolyCenter - pos;
  toCenter.SetLength(toCenterOffset);

  const ezVec3 posWithOffset = pos + toCenter + vLineDir * alongLineOffset;

  if (iVertexIdx < 0)
  {
    auto& poi = interestPoints.ExpandAndGetRef();
    poi.m_bUsed = true;
    poi.m_vPosition = posWithOffset;
    poi.m_vLineDir = vLineDir;
  }
  else if (!interestPoints[iVertexIdx].m_bUsed)
  {
    auto& poi = interestPoints[iVertexIdx];
    poi.m_bUsed = true;
    poi.m_vPosition = posWithOffset;
    poi.m_vLineDir = vLineDir;
  }
  else
  {
    auto& poi = interestPoints[iVertexIdx];

    ezPlane plane;
    plane.SetFromPoints(poi.m_vVertexPos, poi.m_vVertexPos + poi.m_vLineDir, poi.m_vVertexPos + ezVec3(0, 0, 1.0f));

    ezPositionOnPlane::Enum side1 = plane.GetPointPosition(poi.m_vPosition);
    ezPositionOnPlane::Enum side2 = plane.GetPointPosition(posWithOffset);

    if (side1 == side2)
    {
      // same side, collapse points to average position

      poi.m_vPosition = ezMath::Lerp(poi.m_vPosition, posWithOffset, 0.5f);
    }
    else
    {
      // different sides, keep both points

      auto& poi2 = interestPoints.ExpandAndGetRef();
      poi2.m_bUsed = true;
      poi2.m_vPosition = posWithOffset;
      poi2.m_vLineDir = vLineDir;
    }
  }
}

void ezNavMeshPointOfInterestGraph::ExtractInterestPointsFromMesh(const rcPolyMesh& mesh, bool bReinitialize)
{
  EZ_LOG_BLOCK("Extract NavMesh Points of Interest");

  const ezInt32 iMaxNumVertInPoly = mesh.nvp;
  const float fCellSize = mesh.cs;
  const float fCellHeight = mesh.ch;

  const ezVec3 vMeshOrigin(mesh.bmin[0], mesh.bmin[2], mesh.bmin[1]);

  ezDeque<PotentialPoI> interestPoints;
  interestPoints.SetCount(mesh.nverts);

  for (ezInt32 i = 0; i < mesh.nverts; ++i)
  {
    interestPoints[i].m_bUsed = false;
    interestPoints[i].m_vVertexPos = GetNavMeshVertex(&mesh, i, vMeshOrigin, fCellSize, fCellHeight);
  }

  ezStaticArray<ezVec3, 16> polyVertices;
  ezStaticArray<ezUInt16, 16> polyVertexIndices;
  ezStaticArray<bool, 16> isContourEdge;

  for (ezInt32 i = 0; i < mesh.npolys; ++i)
  {
    const ezUInt32 uiBaseIndex = i * (iMaxNumVertInPoly * 2);
    const ezUInt16* polyVtxIndices = &mesh.polys[uiBaseIndex];
    const ezUInt16* neighborData = &mesh.polys[uiBaseIndex + iMaxNumVertInPoly];

    bool hasAnyContour = false;
    polyVertices.Clear();
    polyVertexIndices.Clear();
    isContourEdge.Clear();

    ezVec3 vPolyCenter;
    vPolyCenter.SetZero();

    for (ezInt32 j = 0; j < iMaxNumVertInPoly; ++j)
    {
      if (polyVtxIndices[j] == RC_MESH_NULL_IDX)
        break;

      const bool isDisconnected = neighborData[j] == 0xffff;
      hasAnyContour |= isDisconnected;

      const ezVec3 pos = GetNavMeshVertex(&mesh, polyVtxIndices[j], vMeshOrigin, fCellSize, fCellHeight);
      vPolyCenter += pos;

      polyVertices.PushBack(pos);
      polyVertexIndices.PushBack(polyVtxIndices[j]);
      isContourEdge.PushBack(isDisconnected);
    }

    if (!hasAnyContour)
      continue;

    vPolyCenter /= (float)polyVertices.GetCount();

    // filter out too short edges
    {
      ezUInt32 uiPrevEdgeIdx = isContourEdge.GetCount() - 2;
      ezUInt32 uiCurEdgeIdx = isContourEdge.GetCount() - 1;

      for (ezUInt32 uiNextEdgeIdx = 0; uiNextEdgeIdx < isContourEdge.GetCount(); ++uiNextEdgeIdx)
      {
        if (isContourEdge[uiCurEdgeIdx])
        {
          const ezVec3 start = polyVertices[uiCurEdgeIdx];
          const ezVec3 end = polyVertices[uiNextEdgeIdx];
          const ezVec3 startToEnd = end - start;
          const float distSqr = startToEnd.GetLengthSquared();

          if (distSqr < ezMath::Square(0.5f))
          {
            isContourEdge[uiCurEdgeIdx] = false;
          }
        }

        uiPrevEdgeIdx = uiCurEdgeIdx;
        uiCurEdgeIdx = uiNextEdgeIdx;
      }
    }

    // filter out medium edges with neighbors
    {
      ezUInt32 uiPrevEdgeIdx = isContourEdge.GetCount() - 2;
      ezUInt32 uiCurEdgeIdx = isContourEdge.GetCount() - 1;

      for (ezUInt32 uiNextEdgeIdx = 0; uiNextEdgeIdx < isContourEdge.GetCount(); ++uiNextEdgeIdx)
      {
        if (isContourEdge[uiCurEdgeIdx])
        {
          const ezVec3 start = polyVertices[uiCurEdgeIdx];
          const ezVec3 end = polyVertices[uiNextEdgeIdx];
          const ezVec3 startToEnd = end - start;
          const float distSqr = startToEnd.GetLengthSquared();

          if (distSqr < ezMath::Square(2.0f))
          {
            // if we have neighbor edges, let them try again
            if (isContourEdge[uiPrevEdgeIdx] || isContourEdge[uiNextEdgeIdx])
            {
              // nothing inserted, so treat this as connected edge
              isContourEdge[uiCurEdgeIdx] = false;
            }
          }
        }

        uiPrevEdgeIdx = uiCurEdgeIdx;
        uiCurEdgeIdx = uiNextEdgeIdx;
      }
    }

    // now insert points of interests along contour edges
    {
      ezUInt32 uiPrevEdgeIdx = isContourEdge.GetCount() - 2;
      ezUInt32 uiCurEdgeIdx = isContourEdge.GetCount() - 1;

      for (ezUInt32 uiNextEdgeIdx = 0; uiNextEdgeIdx < isContourEdge.GetCount(); ++uiNextEdgeIdx)
      {
        if (isContourEdge[uiCurEdgeIdx])
        {
          const ezInt32 startIdx = polyVertexIndices[uiCurEdgeIdx];
          const ezInt32 endIdx = polyVertexIndices[uiNextEdgeIdx];

          const ezVec3 start = polyVertices[uiCurEdgeIdx];
          const ezVec3 end = polyVertices[uiNextEdgeIdx];
          const ezVec3 startToEnd = end - start;
          const float distSqr = startToEnd.GetLengthSquared();

          if (distSqr < ezMath::Square(2.0f))
          {
            AddToInterestPoints(interestPoints, -1, ezMath::Lerp(start, end, 0.5f), vPolyCenter, ezVec3::ZeroVector());
          }
          else
          {
            // to prevent inserting the same point multiple times, only the edge with the larger index is allowed to insert
            // points at connected edges
            // due to the index wrap around, there is no guarantee that uiNextEdgeIdx is always larger than uiCurEdgeIdx etc.

            if (isContourEdge[uiPrevEdgeIdx])
            {
              if (uiCurEdgeIdx > uiPrevEdgeIdx)
              {
                AddToInterestPoints(interestPoints, startIdx, start, vPolyCenter, ezVec3::ZeroVector());
              }
            }
            else
            {
              AddToInterestPoints(interestPoints, startIdx, start, vPolyCenter, startToEnd.GetNormalized());
            }

            if (isContourEdge[uiNextEdgeIdx])
            {
              if (uiCurEdgeIdx > uiNextEdgeIdx)
              {
                AddToInterestPoints(interestPoints, endIdx, end, vPolyCenter, ezVec3::ZeroVector());
              }
            }
            else
            {
              AddToInterestPoints(interestPoints, endIdx, end, vPolyCenter, -startToEnd.GetNormalized());
            }
          }
        }

        uiPrevEdgeIdx = uiCurEdgeIdx;
        uiCurEdgeIdx = uiNextEdgeIdx;
      }
    }
  }

  if (bReinitialize)
  {
    ezBoundingBox box;
    box.SetInvalid();

    // compute bounding box
    {
      for (auto potPoi : interestPoints)
      {
        if (potPoi.m_bUsed)
        {
          box.ExpandToInclude(potPoi.m_vPosition);
        }
      }

      box.Grow(ezVec3(1.0f));
    }

    m_NavMeshPointGraph.Initialize(box.GetCenter(), box.GetHalfExtents());
  }



  // add all points
  {
    ezUInt32 uiNumPoints = 0;

    for (auto potPoi : interestPoints)
    {
      if (potPoi.m_bUsed)
      {
        ++uiNumPoints;

        auto& poi = m_NavMeshPointGraph.AddPoint(potPoi.m_vPosition);
        poi.m_vFloorPosition = potPoi.m_vPosition;
      }
    }

    ezLog::Dev("Num Points of Interest: {0}", uiNumPoints);
  }
}
