#include <PCH.h>
#include <RecastPlugin/NavMeshBuilder/NavMeshPointsOfInterest.h>
#include <ThirdParty/Recast/Recast.h>

ezNavMeshPointOfInterestGraph::ezNavMeshPointOfInterestGraph()
{

}

ezNavMeshPointOfInterestGraph::~ezNavMeshPointOfInterestGraph()
{

}

EZ_ALWAYS_INLINE static ezVec3 GetNavMeshVertex(const rcPolyMesh* pMesh, ezUInt16 uiVertex, const ezVec3& vMeshOrigin, float fCellSize, float fCellHeight)
{
  const ezUInt16* v = &pMesh->verts[uiVertex * 3];
  const float x = vMeshOrigin.x + v[0] * fCellSize;
  const float y = vMeshOrigin.y + v[2] * fCellSize;
  const float z = vMeshOrigin.z + v[1] * fCellHeight;

  return ezVec3(x, y, z);
}

const float toCenterOffset = 0.1f;
const float alongLineOffset = 0.5f;

EZ_ALWAYS_INLINE static void AddToInterestPoints(ezDeque<ezVec3>& interestPoints, const ezVec3& pos, const ezVec3& vPolyCenter, ezVec3 vLineDir)
{
  ezVec3 toCenter = vPolyCenter - pos;
  toCenter.SetLength(toCenterOffset);

  const ezVec3 posWithOffset = pos + toCenter + vLineDir * alongLineOffset;

  interestPoints.PushBack(posWithOffset);
}

void ezNavMeshPointOfInterestGraph::ExtractInterestPointsFromMesh(const rcPolyMesh& mesh, bool bReinitialize)
{
  EZ_LOG_BLOCK("Extract NavMesh Points of Interest");

  const ezInt32 iMaxNumVertInPoly = mesh.nvp;
  const float fCellSize = mesh.cs;
  const float fCellHeight = mesh.ch;

  const ezVec3 vMeshOrigin(mesh.bmin[0], mesh.bmin[2], mesh.bmin[1]);

  ezDeque<ezVec3> interestPoints;

  ezStaticArray<ezVec3, 16> polyVertices;
  ezStaticArray<bool, 16> isContourEdge;

  for (ezInt32 i = 0; i < mesh.npolys; ++i)
  {
    const ezUInt32 uiBaseIndex = i * (iMaxNumVertInPoly * 2);
    const ezUInt16* polyVtxIndices = &mesh.polys[uiBaseIndex];
    const ezUInt16* neighborData = &mesh.polys[uiBaseIndex + iMaxNumVertInPoly];

    bool hasAnyContour = false;
    polyVertices.Clear();
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
      isContourEdge.PushBack(isDisconnected);
    }

    if (!hasAnyContour)
      continue;

    vPolyCenter /= (float)polyVertices.GetCount();

    ezUInt32 uiPrevEdgeIdx = isContourEdge.GetCount() - 2;
    ezUInt32 uiCurEdgeIdx = isContourEdge.GetCount() - 1;

    for (ezUInt32 uiNextEdgeIdx = 0; uiNextEdgeIdx < isContourEdge.GetCount(); ++uiNextEdgeIdx)
    {
      if (!isContourEdge[uiCurEdgeIdx])
        goto skip;

      {
        const ezVec3 start = polyVertices[uiCurEdgeIdx];
        const ezVec3 end = polyVertices[uiNextEdgeIdx];
        const ezVec3 startToEnd = end - start;
        const float distSqr = startToEnd.GetLengthSquared();

        if (distSqr < ezMath::Square(0.5f))
        {
          // nothing inserted, so treat this as connected edge
          isContourEdge[uiCurEdgeIdx] = false;
          goto skip;
        }

        if (distSqr < ezMath::Square(2.0f))
        {
          // if we have neighbor edges, let them try again
          if (isContourEdge[uiPrevEdgeIdx] || isContourEdge[uiNextEdgeIdx])
          {
            // nothing inserted, so treat this as connected edge
            isContourEdge[uiCurEdgeIdx] = false;
            goto skip;
          }

          AddToInterestPoints(interestPoints, ezMath::Lerp(start, end, 0.5f), vPolyCenter, ezVec3::ZeroVector());
          goto skip;
        }

        // to prevent inserting the same point multiple times, only the edge with the larger index is allowed to insert
        // points at connected edges
        // due to the index wrap around, there is no guarantee that uiNextEdgeIdx is always larger than uiCurEdgeIdx etc.

        if (isContourEdge[uiPrevEdgeIdx])
        {
          if (uiCurEdgeIdx > uiPrevEdgeIdx)
          {
            AddToInterestPoints(interestPoints, start, vPolyCenter, ezVec3::ZeroVector());
          }
        }
        else
        {
          AddToInterestPoints(interestPoints, start, vPolyCenter, startToEnd.GetNormalized());
        }

        if (isContourEdge[uiNextEdgeIdx])
        {
          if (uiCurEdgeIdx > uiNextEdgeIdx)
          {
            AddToInterestPoints(interestPoints, end, vPolyCenter, ezVec3::ZeroVector());
          }
        }
        else
        {
          AddToInterestPoints(interestPoints, end, vPolyCenter, -startToEnd.GetNormalized());
        }
      }

    skip:

      uiPrevEdgeIdx = uiCurEdgeIdx;
      uiCurEdgeIdx = uiNextEdgeIdx;
    }
  }

  if (bReinitialize)
  {
    ezBoundingBox box;
    box.SetInvalid();

    // compute bounding box
    {
      for (auto pos : interestPoints)
      {
        box.ExpandToInclude(pos);
      }

      box.Grow(ezVec3(1.0f));
    }

    m_NavMeshPointGraph.Initialize(box.GetCenter(), box.GetHalfExtents());
  }

  ezLog::Dev("Num Points of Interest: {0}", interestPoints.GetCount());

  // add all points
  {
    for (auto pos : interestPoints)
    {
      auto& poi = m_NavMeshPointGraph.AddPoint(pos);
      poi.m_vFloorPosition = pos;
    }
  }
}

