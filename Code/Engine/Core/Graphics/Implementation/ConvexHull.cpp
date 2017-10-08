#include <PCH.h>
#include <Core/Graphics/ConvexHull.h>
#include <Foundation/Containers/Bitfield.h>
#include <Foundation/Math/BoundingBox.h>
#include <Foundation/Math/Plane.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>

ezConvexHullGenerator::ezConvexHullGenerator()
{
  m_MinTriangleAngle = ezAngle::Degree(22.0f);
  m_FlatVertexNormalThreshold = ezAngle::Degree(5);
  m_fMinTriangleEdgeLength = 0.05;
}

ezResult ezConvexHullGenerator::ComputeCenterAndScale(const ezArrayPtr<const ezVec3> vertices)
{
  if (vertices.IsEmpty())
    return EZ_FAILURE;

  ezBoundingBox box;
  box.SetFromPoints(vertices.GetPtr(), vertices.GetCount());

  const ezVec3 c = box.GetCenter();
  m_vCenter.Set(c.x, c.y, c.z);

  const ezVec3 ext = box.GetHalfExtents();

  const double minExt = ezMath::Min(ext.x, ext.y, ext.z);

  if (minExt <= 0.000001)
    return EZ_FAILURE;

  const double maxExt = ezMath::Max(ext.x, ext.y, ext.z);

  m_fScale = 1.0 / maxExt;

  return EZ_SUCCESS;
}

ezResult ezConvexHullGenerator::StoreNormalizedVertices(const ezArrayPtr<const ezVec3> vertices)
{
  struct Comparer
  {
    EZ_ALWAYS_INLINE bool Less(const ezVec3d& a, const ezVec3d& b) const
    {
      constexpr double eps = 0.01;

      if (a.x < b.x - eps)
        return true;
      if (a.x > b.x + eps)
        return false;

      if (a.y < b.y - eps)
        return true;
      if (a.y > b.y + eps)
        return false;

      return (a.z < b.z - eps);
    }
  };

  ezSet<ezVec3d, Comparer> used;

  m_Vertices.Clear();
  m_Vertices.Reserve(vertices.GetCount());

  for (ezVec3 v : vertices)
  {
    ezVec3d norm;
    norm.Set(v.x, v.y, v.z);

    // bring into [-1; +1] range for normalized precision
    norm -= m_vCenter;
    norm *= m_fScale;

    if (!used.Contains(norm))
    {
      m_Vertices.PushBack(norm);
      used.Insert(norm);
    }
  }

  if (m_Vertices.GetCount() < 4)
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

void ezConvexHullGenerator::StoreTriangle(int i, int j, int k)
{
  Triangle& triangle = m_Triangles.ExpandAndGetRef();

  EZ_ASSERT_DEBUG((i < j) && (i < k) && (j < k), "Invalid Triangle");

  ezVec3d tangent1 = m_Vertices[k] - m_Vertices[i];
  ezVec3d tangent2 = m_Vertices[j] - m_Vertices[i];

  triangle.m_vNormal = tangent1.Cross(tangent2);
  triangle.m_bIsDegenerate = triangle.m_vNormal.IsZero(0.0000001);

  if (triangle.m_bIsDegenerate)
  {
    // triangle has degenerated to a line
    // use some made up normal to pretend it has some direction

    const ezVec3d orth = m_Vertices[i] - m_vInside;
    tangent2 = tangent1.Cross(orth);
    triangle.m_vNormal = tangent1.Cross(tangent2);

    EZ_ASSERT_DEBUG(!triangle.m_vNormal.IsZero(0.0000001), "Normal is still invalid");
  }

  // needs to be normalized for later pruning
  triangle.m_vNormal.Normalize();
  triangle.m_fPlaneDistance = triangle.m_vNormal.Dot(m_Vertices[i]);

  triangle.m_uiVertexIdx[0] = i;
  triangle.m_uiVertexIdx[1] = j;
  triangle.m_uiVertexIdx[2] = k;

  const ezUInt32 uiMaxVertices = m_Vertices.GetCount();

  m_Edges[i * uiMaxVertices + j].Add(k);
  m_Edges[i * uiMaxVertices + k].Add(j);
  m_Edges[j * uiMaxVertices + k].Add(i);

  triangle.m_bFlip = triangle.m_vNormal.Dot(m_vInside) > triangle.m_fPlaneDistance;

  if (triangle.m_bFlip)
  {
    triangle.m_vNormal = -triangle.m_vNormal;
    triangle.m_fPlaneDistance = -triangle.m_fPlaneDistance;
  }
}

ezResult ezConvexHullGenerator::InitializeHull()
{
  ezVec3d minV = m_Vertices[0];
  ezVec3d maxV = m_Vertices[0];
  ezUInt32 minIx = 0;
  ezUInt32 minIy = 0;
  ezUInt32 minIz = 0;
  ezUInt32 maxIx = 0;
  ezUInt32 maxIy = 0;
  ezUInt32 maxIz = 0;

  for (ezUInt32 i = 0; i < m_Vertices.GetCount(); ++i)
  {
    const auto& v = m_Vertices[i];

    if (v.x < minV.x)
    {
      minV.x = v.x;
      minIx = i;
    }

    if (v.x > maxV.x)
    {
      maxV.x = v.x;
      maxIx = i;
    }

    if (v.y < minV.y)
    {
      minV.y = v.y;
      minIy = i;
    }

    if (v.y > maxV.y)
    {
      maxV.y = v.y;
      maxIy = i;
    }

    if (v.z < minV.z)
    {
      minV.z = v.z;
      minIz = i;
    }

    if (v.z > maxV.z)
    {
      maxV.z = v.z;
      maxIz = i;
    }
  }

  const ezVec3d extents = maxV - minV;
  ezUInt32 uiMainAxis1;
  ezUInt32 uiMainAxis2;

  if (extents.x >= extents.y && extents.x >= extents.z)
  {
    uiMainAxis1 = minIx;
    uiMainAxis2 = maxIx;
  }
  else if (extents.y >= extents.x && extents.y >= extents.z)
  {
    uiMainAxis1 = minIy;
    uiMainAxis2 = maxIy;
  }
  else
  {
    uiMainAxis1 = minIz;
    uiMainAxis2 = maxIz;
  }

  if (uiMainAxis1 == uiMainAxis2)
    return EZ_FAILURE;

  ezHybridArray<ezUInt32, 6> testIdx;
  testIdx.PushBack(uiMainAxis1);
  testIdx.PushBack(uiMainAxis2);

  if (!testIdx.Contains(minIx))
    testIdx.PushBack(minIx);
  if (!testIdx.Contains(minIy))
    testIdx.PushBack(minIy);
  if (!testIdx.Contains(minIz))
    testIdx.PushBack(minIz);
  if (!testIdx.Contains(maxIx))
    testIdx.PushBack(maxIx);
  if (!testIdx.Contains(maxIy))
    testIdx.PushBack(maxIy);
  if (!testIdx.Contains(maxIz))
    testIdx.PushBack(maxIz);

  if (testIdx.GetCount() < 4)
  {
    // if we could not find enough vertices for the initial shape,
    // we will look at a couple more, even if those might not be the best candidates

    ezUInt32 uiMaxVts = ezMath::Min(50U, m_Vertices.GetCount());
    for (ezUInt32 i = 0; i < uiMaxVts; ++i)
    {
      testIdx.PushBack(i);
    }
  }

  ezVec3d planePoints[3];
  planePoints[0] = m_Vertices[testIdx[0]];
  planePoints[1] = m_Vertices[testIdx[1]];

  double maxDist = 0;
  ezUInt32 uiIx1 = 0xFFFFFFFF, uiIx2 = 0xFFFFFFFF;

  for (ezUInt32 i = 2; i < testIdx.GetCount(); ++i)
  {
    planePoints[2] = m_Vertices[testIdx[i]];

    ezPlaned p;
    if (p.SetFromPoints(planePoints).Failed())
      continue;

    for (ezUInt32 j = 2; j < testIdx.GetCount(); ++j)
    {
      if (i == j)
        continue;

      const double thisDist = ezMath::Abs(p.GetDistanceTo(m_Vertices[testIdx[j]]));
      if (thisDist > maxDist)
      {
        maxDist = thisDist;
        uiIx1 = testIdx[i];
        uiIx2 = testIdx[j];
      }
    }
  }

  if (uiIx1 == 0xFFFFFFFF || uiIx2 == 0xFFFFFFFF)
    return EZ_FAILURE;

  // move the four chosen ones to the front of the queue
  testIdx.Clear();
  testIdx.PushBack(uiMainAxis1);
  testIdx.PushBack(uiMainAxis2);
  testIdx.PushBack(uiIx1);
  testIdx.PushBack(uiIx2);
  testIdx.Sort();

  for (int i = 0; i < 4; ++i)
  {
    if (i > 0)
    {
      EZ_ASSERT_DEBUG(testIdx[i - 1] != testIdx[i], "Same index used twice");
    }

    ezMath::Swap(m_Vertices[i], m_Vertices[testIdx[i]]);
  }

  // precompute the 'inside' position
  {
    m_vInside.SetZero();
    for (ezUInt32 v = 0; v < 4; ++v)
      m_vInside += m_Vertices[v];
    m_vInside /= 4.0;
  }

  // construct the hull as containing only the first four points
  for (int i = 0; i < 4; i++)
  {
    for (int j = i + 1; j < 4; j++)
    {
      for (int k = j + 1; k < 4; k++)
      {
        StoreTriangle(i, j, k);
      }
    }
  }

  return EZ_SUCCESS;
}

ezResult ezConvexHullGenerator::ComputeHull()
{
  const ezUInt32 uiMaxVertices = m_Vertices.GetCount();

  m_Edges.Clear();
  m_Edges.SetCount(ezMath::Square(uiMaxVertices));
  m_Triangles.Clear();
  m_Triangles.Reserve(512);

  EZ_SUCCEED_OR_RETURN(InitializeHull());

  // Add the points to the hull, one at a time.
  for (ezUInt32 vtxId = 4; vtxId < uiMaxVertices; ++vtxId)
  {
    if (IsInside(vtxId))
      continue;

    // Find and delete all faces with their outside 'illuminated' by this point.
    RemoveVisibleFaces(vtxId);

    // Now for any edge still in the hull that is only part of one face
    // add another face containing the new point and that edge to the hull.
    PatchHole(vtxId);
  }

  if (m_Triangles.GetCount() < 4)
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

bool ezConvexHullGenerator::IsInside(ezUInt32 vtxId) const
{
  const ezVec3d pos = m_Vertices[vtxId];

  const ezInt32 iNumTriangles = m_Triangles.GetCount();
  for (ezInt32 j = 0; j < iNumTriangles; j++)
  {
    const auto& tri = m_Triangles[j];

    const double dist = tri.m_vNormal.Dot(pos);
    if (dist > tri.m_fPlaneDistance + 0.01)
      return false;
  }

  return true;
}

void ezConvexHullGenerator::RemoveVisibleFaces(ezUInt32 vtxId)
{
  const ezUInt32 uiMaxVertices = m_Vertices.GetCount();
  const ezVec3d pos = m_Vertices[vtxId];

  ezInt32 iNumTriangles = m_Triangles.GetCount();
  for (ezInt32 j = 0; j < iNumTriangles; j++)
  {
    const auto& tri = m_Triangles[j];

    const double dist = tri.m_vNormal.Dot(pos);
    if (dist <= tri.m_fPlaneDistance)
      continue;

    const ezUInt16 vtx0 = tri.m_uiVertexIdx[0];
    const ezUInt16 vtx1 = tri.m_uiVertexIdx[1];
    const ezUInt16 vtx2 = tri.m_uiVertexIdx[2];

    m_Edges[vtx0 * uiMaxVertices + vtx1].Remove(vtx2);
    m_Edges[vtx0 * uiMaxVertices + vtx2].Remove(vtx1);
    m_Edges[vtx1 * uiMaxVertices + vtx2].Remove(vtx0);

    m_Triangles.RemoveAtSwap(j);

    --j;
    --iNumTriangles;
  }
}

void ezConvexHullGenerator::PatchHole(ezUInt32 vtxId)
{
  const ezUInt32 uiMaxVertices = m_Vertices.GetCount();

  const ezUInt32 uiNumFaces = m_Triangles.GetCount();
  for (ezUInt32 j = 0; j < uiNumFaces; j++)
  {
    const auto& tri = m_Triangles[j];

    for (ezInt32 a = 0; a < 3; a++)
    {
      for (ezInt32 b = a + 1; b < 3; b++)
      {
        const ezUInt16 vtxA = tri.m_uiVertexIdx[a];
        const ezUInt16 vtxB = tri.m_uiVertexIdx[b];

        if (m_Edges[vtxA * uiMaxVertices + vtxB].GetSize() == 2)
          continue;

        StoreTriangle(vtxA, vtxB, vtxId);
      }
    }
  }
}

bool ezConvexHullGenerator::PruneFlatVertices(double fNormalThreshold)
{
  struct VertexNormals
  {
    ezVec3d m_vNormals[2];
    ezInt32 m_iDifferentNormals = 0;
  };

  ezDynamicArray<VertexNormals> VtxNorms;
  VtxNorms.SetCount(m_Vertices.GetCount());

  ezUInt32 uiNumVerticesRemaining = 0;

  for (const auto& tri : m_Triangles)
  {
    if (tri.m_bIsDegenerate)
      continue;

    const ezVec3d planeNorm = tri.m_vNormal;

    for (int v = 0; v < 3; ++v)
    {
      VertexNormals& norms = VtxNorms[tri.m_uiVertexIdx[v]];

      if (norms.m_iDifferentNormals > 2)
        continue;

      for (int d = 0; d < norms.m_iDifferentNormals; ++d)
      {

        if (norms.m_vNormals[d].Dot(planeNorm) > fNormalThreshold)
          goto same;
      }

      if (norms.m_iDifferentNormals == 2)
      {
        uiNumVerticesRemaining++;
        norms.m_iDifferentNormals = 3;
      }
      else
      {
        norms.m_vNormals[norms.m_iDifferentNormals] = planeNorm;
        ++norms.m_iDifferentNormals;
      }

    same:
      continue;
    }
  }

  if (uiNumVerticesRemaining < 4)
    return false;

  if (uiNumVerticesRemaining == m_Vertices.GetCount())
    return false;

  ezDynamicArray<ezVec3d> remaining;
  remaining.Reserve(uiNumVerticesRemaining);

  // now only keep the vertices that have at least 3 different normals
  for (ezUInt32 v = 0; v < m_Vertices.GetCount(); ++v)
  {
    if (VtxNorms[v].m_iDifferentNormals < 3)
      continue;

    remaining.PushBack(m_Vertices[v]);
  }

  m_Vertices = remaining;

  return true;
}


bool ezConvexHullGenerator::PruneDegenerateTriangles(double fMaxAngle)
{
  bool bChanged = false;

  ezDynamicBitfield discardVtx;
  discardVtx.SetCount(m_Vertices.GetCount(), false);

  for (const auto& tri : m_Triangles)
  {
    const ezUInt32 idx0 = tri.m_uiVertexIdx[0];
    const ezUInt32 idx1 = tri.m_uiVertexIdx[1];
    const ezUInt32 idx2 = tri.m_uiVertexIdx[2];
    const ezVec3d v0 = m_Vertices[idx0];
    const ezVec3d v1 = m_Vertices[idx1];
    const ezVec3d v2 = m_Vertices[idx2];
    const ezVec3d e0 = (v1 - v0).GetNormalized();
    const ezVec3d e1 = (v2 - v1).GetNormalized();
    const ezVec3d e2 = (v0 - v2).GetNormalized();

    if (e0.Dot(e1) > fMaxAngle)
    {
      discardVtx.SetBit(idx1);
      bChanged = true;
    }

    if (e1.Dot(e2) > fMaxAngle)
    {
      discardVtx.SetBit(idx2);
      bChanged = true;
    }

    if (e2.Dot(e0) > fMaxAngle)
    {
      discardVtx.SetBit(idx0);
      bChanged = true;
    }
  }

  if (bChanged)
  {
    for (ezUInt32 n = discardVtx.GetCount(); n > 0; --n)
    {
      if (discardVtx.IsSet(n - 1))
      {
        m_Vertices.RemoveAtSwap(n - 1);
      }
    }
  }

  return bChanged;
}

bool ezConvexHullGenerator::PruneSmallTriangles(double fMaxEdgeLen)
{
  bool bChanged = false;

  ezDynamicBitfield discardVtx;
  discardVtx.SetCount(m_Vertices.GetCount(), false);

  for (const auto& tri : m_Triangles)
  {
    if (tri.m_bIsDegenerate)
      continue;

    const ezUInt32 idx0 = tri.m_uiVertexIdx[0];
    const ezUInt32 idx1 = tri.m_uiVertexIdx[1];
    const ezUInt32 idx2 = tri.m_uiVertexIdx[2];
    const ezVec3d v0 = m_Vertices[idx0];
    const ezVec3d v1 = m_Vertices[idx1];
    const ezVec3d v2 = m_Vertices[idx2];
    const double len0 = (v1 - v0).GetLength();
    const double len1 = (v2 - v1).GetLength();
    const double len2 = (v0 - v2).GetLength();

    if (len0 < fMaxEdgeLen && len1 < fMaxEdgeLen && len2 < fMaxEdgeLen)
    {
      discardVtx.SetBit(idx0);
      discardVtx.SetBit(idx1);
      discardVtx.SetBit(idx2);

      const ezVec3d center = (v0 + v1 + v2) / 3.0;
      m_Vertices.PushBack(center);

      bChanged = true;

      continue;
    }

    if (len0 < fMaxEdgeLen)
    {
      discardVtx.SetBit(idx0);
      discardVtx.SetBit(idx1);

      const ezVec3d center = (v0 + v1) / 2.0;
      m_Vertices.PushBack(center);

      bChanged = true;
    }

    if (len1 < fMaxEdgeLen)
    {
      discardVtx.SetBit(idx2);
      discardVtx.SetBit(idx1);

      const ezVec3d center = (v2 + v1) / 2.0;
      m_Vertices.PushBack(center);

      bChanged = true;
    }

    if (len2 < fMaxEdgeLen)
    {
      discardVtx.SetBit(idx0);
      discardVtx.SetBit(idx2);

      const ezVec3d center = (v0 + v2) / 2.0;
      m_Vertices.PushBack(center);

      bChanged = true;
    }
  }

  if (bChanged)
  {
    for (ezUInt32 n = discardVtx.GetCount(); n > 0; --n)
    {
      if (discardVtx.IsSet(n - 1))
      {
        m_Vertices.RemoveAtSwap(n - 1);
      }
    }
  }

  return bChanged;
}

ezResult ezConvexHullGenerator::ProcessVertices(const ezArrayPtr<const ezVec3> vertices)
{
  ezUInt32 uiFirstVertex = 0;
  ezUInt32 uiNumVerticesLeft = vertices.GetCount();
  const ezUInt32 uiVerticesPerBatch = 1000;

  ezDynamicArray<ezVec3> workingSet;

  while (uiNumVerticesLeft > 0)
  {
    RetrieveVertices(workingSet);

    const ezUInt32 uiAdd = ezMath::Min(uiNumVerticesLeft, uiVerticesPerBatch);
    const ezArrayPtr<const ezVec3> range = vertices.GetSubArray(uiFirstVertex, uiAdd);
    workingSet.PushBackRange(range);

    uiFirstVertex += uiAdd;
    uiNumVerticesLeft -= uiAdd;

    EZ_SUCCEED_OR_RETURN(StoreNormalizedVertices(workingSet));

    if (m_Vertices.GetCount() >= 16384)
      return EZ_FAILURE;

    EZ_SUCCEED_OR_RETURN(ComputeHull());
  }

  if (m_Triangles.GetCount() < 4)
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

ezResult ezConvexHullGenerator::Build(const ezArrayPtr<const ezVec3> vertices)
{
  m_Vertices.Clear();

  EZ_SUCCEED_OR_RETURN(ComputeCenterAndScale(vertices));

  EZ_SUCCEED_OR_RETURN(ProcessVertices(vertices));

  bool prune = true;
  while (prune)
  {
    prune = false;

    if (PruneDegenerateTriangles(ezMath::Cos(m_MinTriangleAngle)))
    {
      EZ_SUCCEED_OR_RETURN(ComputeHull());
      prune = true;
    }

    if (PruneFlatVertices(ezMath::Cos(m_FlatVertexNormalThreshold)))
    {
      EZ_SUCCEED_OR_RETURN(ComputeHull());
      prune = true;
    }

    if (PruneSmallTriangles(m_fMinTriangleEdgeLength))
    {
      EZ_SUCCEED_OR_RETURN(ComputeHull());
      prune = true;
    }
  }

  return EZ_SUCCESS;
}

void ezConvexHullGenerator::Retrieve(ezDynamicArray<ezVec3>& out_Vertices, ezDynamicArray<Face>& out_Faces)
{
  out_Vertices.Clear();
  out_Faces.Clear();

  out_Vertices.Reserve(m_Triangles.GetCount() * 2);
  out_Faces.Reserve(m_Triangles.GetCount());

  ezMap<ezUInt32, ezUInt32> vtxMap;

  const double fScaleBack = 1.0 / m_fScale;

  for (const auto& tri : m_Triangles)
  {
    auto& face = out_Faces.ExpandAndGetRef();

    for (int v = 0; v < 3; ++v)
    {
      const ezUInt32 orgIdx = tri.m_uiVertexIdx[v];

      bool bExisted = false;
      auto it = vtxMap.FindOrAdd(orgIdx, &bExisted);
      if (!bExisted)
      {
        it.Value() = out_Vertices.GetCount();

        const ezVec3d pos = (m_Vertices[orgIdx] * fScaleBack) + m_vCenter;

        ezVec3& vtx = out_Vertices.ExpandAndGetRef();
        vtx.Set((float)pos.x, (float)pos.y, (float)pos.z);
      }

      face.m_uiVertexIdx[v] = it.Value();
    }

    if (tri.m_bFlip)
    {
      ezMath::Swap(face.m_uiVertexIdx[1], face.m_uiVertexIdx[2]);
    }
  }
}

void ezConvexHullGenerator::RetrieveVertices(ezDynamicArray<ezVec3>& out_Vertices)
{
  out_Vertices.Clear();
  out_Vertices.Reserve(m_Triangles.GetCount() * 2);

  ezMap<ezUInt32, ezUInt32> vtxMap;

  const double fScaleBack = 1.0 / m_fScale;

  for (const auto& tri : m_Triangles)
  {
    for (int v = 0; v < 3; ++v)
    {
      const ezUInt32 orgIdx = tri.m_uiVertexIdx[v];

      bool bExisted = false;
      auto it = vtxMap.FindOrAdd(orgIdx, &bExisted);
      if (!bExisted)
      {
        it.Value() = out_Vertices.GetCount();

        const ezVec3d pos = (m_Vertices[orgIdx] * fScaleBack) + m_vCenter;

        ezVec3& vtx = out_Vertices.ExpandAndGetRef();
        vtx.Set((float)pos.x, (float)pos.y, (float)pos.z);
      }
    }
  }
}
