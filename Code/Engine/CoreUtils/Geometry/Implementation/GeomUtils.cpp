#include <CoreUtils/PCH.h>
#include <CoreUtils/Geometry/GeomUtils.h>

void ezGeometry::Clear()
{
  m_Vertices.Clear();
  m_Polygons.Clear();
  m_Lines.Clear();
}

ezUInt32 ezGeometry::AddVertex(const ezVec3& vPos, const ezVec3& vNormal, const ezVec2& vTexCoord, const ezColor& color, ezInt32 iCustomIndex)
{
  Vertex v;
  v.m_vPosition = vPos;
  v.m_vNormal = vNormal;
  v.m_vTexCoord = vTexCoord;
  v.m_Color = color;
  v.m_iCustomIndex = iCustomIndex;

  m_Vertices.PushBack(v);

  return m_Vertices.GetCount() - 1;
}

ezUInt32 ezGeometry::AddVertex(const ezVec3& vPos, const ezVec3& vNormal, const ezVec2& vTexCoord, const ezColor& color, ezInt32 iCustomIndex, const ezMat4& mTransform)
{
  return AddVertex(mTransform.TransformPosition(vPos), mTransform.TransformDirection(vNormal).GetNormalized(), vTexCoord, color, iCustomIndex);
}

void ezGeometry::AddPolygon(const ezArrayPtr<ezUInt32>& Vertices)
{
  EZ_ASSERT_DEV(Vertices.GetCount() >= 3, "Polygon must have at least 3 vertices, not %u", Vertices.GetCount());

  for (ezUInt32 v = 0; v < Vertices.GetCount(); ++v)
  {
    EZ_ASSERT_DEV(Vertices[v] < m_Vertices.GetCount(), "Invalid vertex index %u, geometry only has %u vertices", Vertices[v], m_Vertices.GetCount());
  }

  m_Polygons.SetCount(m_Polygons.GetCount() + 1); // could use 'ExpandOne' here

  m_Polygons.PeekBack().m_Vertices = Vertices;
}

void ezGeometry::AddLine(ezUInt32 uiStartVertex, ezUInt32 uiEndVertex)
{
  EZ_ASSERT_DEV(uiStartVertex < m_Vertices.GetCount(), "Invalid vertex index %u, geometry only has %u vertices", uiStartVertex, m_Vertices.GetCount());
  EZ_ASSERT_DEV(uiEndVertex < m_Vertices.GetCount(), "Invalid vertex index %u, geometry only has %u vertices", uiEndVertex, m_Vertices.GetCount());

  Line l;
  l.m_uiStartVertex = uiStartVertex;
  l.m_uiEndVertex = uiEndVertex;

  m_Lines.PushBack(l);
}

void ezGeometry::ComputeFaceNormals()
{
  for (ezUInt32 p = 0; p < m_Polygons.GetCount(); ++p)
  {
    Polygon& poly = m_Polygons[p];

    const ezVec3& v1 = m_Vertices[poly.m_Vertices[0]].m_vPosition;
    const ezVec3& v2 = m_Vertices[poly.m_Vertices[1]].m_vPosition;
    const ezVec3& v3 = m_Vertices[poly.m_Vertices[2]].m_vPosition;

    poly.m_vNormal.CalculateNormal(v1, v2, v3);
  }
}

void ezGeometry::ComputeSmoothVertexNormals()
{
  // reset all vertex normals
  for (ezUInt32 v = 0; v < m_Vertices.GetCount(); ++v)
  {
    m_Vertices[v].m_vNormal.SetZero();
  }

  // add face normal of all adjacent faces to each vertex
  for (ezUInt32 p = 0; p < m_Polygons.GetCount(); ++p)
  {
    Polygon& poly = m_Polygons[p];

    for (ezUInt32 v = 0; v < poly.m_Vertices.GetCount(); ++v)
    {
      m_Vertices[poly.m_Vertices[v]].m_vNormal += poly.m_vNormal;
    }
  }

  // normalize all vertex normals
  for (ezUInt32 v = 0; v < m_Vertices.GetCount(); ++v)
  {
    m_Vertices[v].m_vNormal.NormalizeIfNotZero(ezVec3(0, 1, 0));
  }
}

void ezGeometry::SetAllVertexCustomIndex(ezInt32 iCustomIndex, ezUInt32 uiFirstVertex)
{
  for (ezUInt32 v = uiFirstVertex; v < m_Vertices.GetCount(); ++v)
    m_Vertices[v].m_iCustomIndex = iCustomIndex;
}

void ezGeometry::SetAllVertexColor(const ezColor& color, ezUInt32 uiFirstVertex)
{
  for (ezUInt32 v = uiFirstVertex; v < m_Vertices.GetCount(); ++v)
    m_Vertices[v].m_Color = color;
}

void ezGeometry::TransformVertices(const ezMat4& mTransform, ezUInt32 uiFirstVertex)
{
  if (mTransform.IsIdentity(ezMath::BasicType<float>::SmallEpsilon()))
    return;

  for (ezUInt32 v = uiFirstVertex; v < m_Vertices.GetCount(); ++v)
  {
    m_Vertices[v].m_vPosition = mTransform.TransformPosition(m_Vertices[v].m_vPosition);
    m_Vertices[v].m_vNormal = mTransform.TransformDirection(m_Vertices[v].m_vNormal);
  }
}

void ezGeometry::Transform(const ezMat4& mTransform, bool bTransformPolyNormals)
{
  TransformVertices(mTransform, 0);

  if (bTransformPolyNormals)
  {
    for (ezUInt32 p = 0; p < m_Polygons.GetCount(); ++p)
    {
      m_Polygons[p].m_vNormal = mTransform.TransformDirection(m_Polygons[p].m_vNormal);
    }
  }
}

void ezGeometry::Merge(const ezGeometry& other)
{
  const ezUInt32 uiVertexOffset = m_Vertices.GetCount();

  for (ezUInt32 v = 0; v < other.m_Vertices.GetCount(); ++v)
  {
    m_Vertices.PushBack(other.m_Vertices[v]);
  }

  for (ezUInt32 p = 0; p < other.m_Polygons.GetCount(); ++p)
  {
    m_Polygons.PushBack(other.m_Polygons[p]);
    Polygon& poly = m_Polygons.PeekBack();

    for (ezUInt32 pv = 0; pv < poly.m_Vertices.GetCount(); ++pv)
    {
      poly.m_Vertices[pv] += uiVertexOffset;
    }
  }

  for (ezUInt32 l = 0; l < other.m_Lines.GetCount(); ++l)
  {
    Line line;
    line.m_uiStartVertex = other.m_Lines[l].m_uiStartVertex + uiVertexOffset;
    line.m_uiEndVertex = other.m_Lines[l].m_uiEndVertex + uiVertexOffset;

    m_Lines.PushBack(line);
  }
}

void ezGeometry::AddRectXY(const ezVec2& size, const ezColor& color, const ezMat4& mTransform, ezInt32 iCustomIndex)
{
  const ezVec2 halfSize = size * 0.5f;

  ezUInt32 idx[4];

  idx[0] = AddVertex(ezVec3(-halfSize.x, -halfSize.y, 0), ezVec3(0, 0, 1), ezVec2(0, 1), color, iCustomIndex, mTransform);
  idx[1] = AddVertex(ezVec3( halfSize.x, -halfSize.y, 0), ezVec3(0, 0, 1), ezVec2(0, 0), color, iCustomIndex, mTransform);
  idx[2] = AddVertex(ezVec3( halfSize.x,  halfSize.y, 0), ezVec3(0, 0, 1), ezVec2(1, 0), color, iCustomIndex, mTransform);
  idx[3] = AddVertex(ezVec3(-halfSize.x,  halfSize.y, 0), ezVec3(0, 0, 1), ezVec2(1, 1), color, iCustomIndex, mTransform);

  AddPolygon(idx);
}

void ezGeometry::AddBox(const ezVec3& size, const ezColor& color, const ezMat4& mTransform, ezInt32 iCustomIndex)
{
  const ezVec3 halfSize = size * 0.5f;

  ezUInt32 idx[8];

  idx[0] = AddVertex(ezVec3(-halfSize.x, -halfSize.y, halfSize.z), ezVec3(0, 0, 1), ezVec2(0), color, iCustomIndex, mTransform);
  idx[1] = AddVertex(ezVec3(halfSize.x, -halfSize.y, halfSize.z), ezVec3(0, 0, 1), ezVec2(0), color, iCustomIndex, mTransform);
  idx[2] = AddVertex(ezVec3(halfSize.x, halfSize.y, halfSize.z), ezVec3(0, 0, 1), ezVec2(0), color, iCustomIndex, mTransform);
  idx[3] = AddVertex(ezVec3(-halfSize.x, halfSize.y, halfSize.z), ezVec3(0, 0, 1), ezVec2(0), color, iCustomIndex, mTransform);

  idx[4] = AddVertex(ezVec3(-halfSize.x, -halfSize.y, -halfSize.z), ezVec3(0, 0, -1), ezVec2(0), color, iCustomIndex, mTransform);
  idx[5] = AddVertex(ezVec3(halfSize.x, -halfSize.y, -halfSize.z), ezVec3(0, 0, -1), ezVec2(0), color, iCustomIndex, mTransform);
  idx[6] = AddVertex(ezVec3(halfSize.x, halfSize.y, -halfSize.z), ezVec3(0, 0, -1), ezVec2(0), color, iCustomIndex, mTransform);
  idx[7] = AddVertex(ezVec3(-halfSize.x, halfSize.y, -halfSize.z), ezVec3(0, 0, -1), ezVec2(0), color, iCustomIndex, mTransform);

  ezUInt32 poly[4];

  poly[0] = idx[0];
  poly[1] = idx[1];
  poly[2] = idx[2];
  poly[3] = idx[3];
  AddPolygon(poly);

  poly[0] = idx[1];
  poly[1] = idx[5];
  poly[2] = idx[6];
  poly[3] = idx[2];
  AddPolygon(poly);

  poly[0] = idx[5];
  poly[1] = idx[4];
  poly[2] = idx[7];
  poly[3] = idx[6];
  AddPolygon(poly);

  poly[0] = idx[4];
  poly[1] = idx[0];
  poly[2] = idx[3];
  poly[3] = idx[7];
  AddPolygon(poly);

  poly[0] = idx[4];
  poly[1] = idx[5];
  poly[2] = idx[1];
  poly[3] = idx[0];
  AddPolygon(poly);

  poly[0] = idx[3];
  poly[1] = idx[2];
  poly[2] = idx[6];
  poly[3] = idx[7];
  AddPolygon(poly);
}

void ezGeometry::AddTexturedBox(const ezVec3& size, const ezColor& color, const ezMat4& mTransform, ezInt32 iCustomIndex)
{
  const ezVec3 halfSize = size * 0.5f;

  ezUInt32 idx[4];

  {
    idx[0] = AddVertex(ezVec3(-halfSize.x, -halfSize.y, +halfSize.z), ezVec3(0, 0, 1), ezVec2(0, 1), color, iCustomIndex, mTransform);
    idx[1] = AddVertex(ezVec3(+halfSize.x, -halfSize.y, +halfSize.z), ezVec3(0, 0, 1), ezVec2(0, 0), color, iCustomIndex, mTransform);
    idx[2] = AddVertex(ezVec3(+halfSize.x, +halfSize.y, +halfSize.z), ezVec3(0, 0, 1), ezVec2(1, 0), color, iCustomIndex, mTransform);
    idx[3] = AddVertex(ezVec3(-halfSize.x, +halfSize.y, +halfSize.z), ezVec3(0, 0, 1), ezVec2(1, 1), color, iCustomIndex, mTransform);
    AddPolygon(idx);
  }

  {
    idx[0] = AddVertex(ezVec3(-halfSize.x, +halfSize.y, -halfSize.z), ezVec3(0, 0, -1), ezVec2(1, 0), color, iCustomIndex, mTransform);
    idx[1] = AddVertex(ezVec3(+halfSize.x, +halfSize.y, -halfSize.z), ezVec3(0, 0, -1), ezVec2(1, 1), color, iCustomIndex, mTransform);
    idx[2] = AddVertex(ezVec3(+halfSize.x, -halfSize.y, -halfSize.z), ezVec3(0, 0, -1), ezVec2(0, 1), color, iCustomIndex, mTransform);
    idx[3] = AddVertex(ezVec3(-halfSize.x, -halfSize.y, -halfSize.z), ezVec3(0, 0, -1), ezVec2(0, 0), color, iCustomIndex, mTransform);
    AddPolygon(idx);
  }

  {
    idx[0] = AddVertex(ezVec3(-halfSize.x, -halfSize.y, -halfSize.z), ezVec3(-1, 0, 0), ezVec2(0, 1), color, iCustomIndex, mTransform);
    idx[1] = AddVertex(ezVec3(-halfSize.x, -halfSize.y, +halfSize.z), ezVec3(-1, 0, 0), ezVec2(0, 0), color, iCustomIndex, mTransform);
    idx[2] = AddVertex(ezVec3(-halfSize.x, +halfSize.y, +halfSize.z), ezVec3(-1, 0, 0), ezVec2(1, 0), color, iCustomIndex, mTransform);
    idx[3] = AddVertex(ezVec3(-halfSize.x, +halfSize.y, -halfSize.z), ezVec3(-1, 0, 0), ezVec2(1, 1), color, iCustomIndex, mTransform);
    AddPolygon(idx);
  }

  {
    idx[0] = AddVertex(ezVec3(+halfSize.x, +halfSize.y, -halfSize.z), ezVec3(1, 0, 0), ezVec2(0, 1), color, iCustomIndex, mTransform);
    idx[1] = AddVertex(ezVec3(+halfSize.x, +halfSize.y, +halfSize.z), ezVec3(1, 0, 0), ezVec2(0, 0), color, iCustomIndex, mTransform);
    idx[2] = AddVertex(ezVec3(+halfSize.x, -halfSize.y, +halfSize.z), ezVec3(1, 0, 0), ezVec2(1, 0), color, iCustomIndex, mTransform);
    idx[3] = AddVertex(ezVec3(+halfSize.x, -halfSize.y, -halfSize.z), ezVec3(1, 0, 0), ezVec2(1, 1), color, iCustomIndex, mTransform);
    AddPolygon(idx);
  }

  {
    idx[0] = AddVertex(ezVec3(+halfSize.x, -halfSize.y, -halfSize.z), ezVec3(0, -1, 0), ezVec2(0, 1), color, iCustomIndex, mTransform);
    idx[1] = AddVertex(ezVec3(+halfSize.x, -halfSize.y, +halfSize.z), ezVec3(0, -1, 0), ezVec2(0, 0), color, iCustomIndex, mTransform);
    idx[2] = AddVertex(ezVec3(-halfSize.x, -halfSize.y, +halfSize.z), ezVec3(0, -1, 0), ezVec2(1, 0), color, iCustomIndex, mTransform);
    idx[3] = AddVertex(ezVec3(-halfSize.x, -halfSize.y, -halfSize.z), ezVec3(0, -1, 0), ezVec2(1, 1), color, iCustomIndex, mTransform);
    AddPolygon(idx);
  }

  {
    idx[0] = AddVertex(ezVec3(-halfSize.x, +halfSize.y, -halfSize.z), ezVec3(0, +1, 0), ezVec2(0, 1), color, iCustomIndex, mTransform);
    idx[1] = AddVertex(ezVec3(-halfSize.x, +halfSize.y, +halfSize.z), ezVec3(0, +1, 0), ezVec2(0, 0), color, iCustomIndex, mTransform);
    idx[2] = AddVertex(ezVec3(+halfSize.x, +halfSize.y, +halfSize.z), ezVec3(0, +1, 0), ezVec2(1, 0), color, iCustomIndex, mTransform);
    idx[3] = AddVertex(ezVec3(+halfSize.x, +halfSize.y, -halfSize.z), ezVec3(0, +1, 0), ezVec2(1, 1), color, iCustomIndex, mTransform);
    AddPolygon(idx);
  }
}

void ezGeometry::AddPyramid(const ezVec3& size, bool bCap, const ezColor& color, const ezMat4& mTransform, ezInt32 iCustomIndex)
{
  const ezVec3 halfSize = size * 0.5f;

  ezUInt32 quad[4];

  quad[0] = AddVertex(ezVec3(-halfSize.x, halfSize.y, 0), ezVec3(-1, 1, 0).GetNormalized(), ezVec2(0), color, iCustomIndex, mTransform);
  quad[1] = AddVertex(ezVec3(halfSize.x, halfSize.y, 0), ezVec3(1, 1, 0).GetNormalized(), ezVec2(0), color, iCustomIndex, mTransform);
  quad[2] = AddVertex(ezVec3(halfSize.x, -halfSize.y, 0), ezVec3(1, -1, 0).GetNormalized(), ezVec2(0), color, iCustomIndex, mTransform);
  quad[3] = AddVertex(ezVec3(-halfSize.x, -halfSize.y, 0), ezVec3(-1, -1, 0).GetNormalized(), ezVec2(0), color, iCustomIndex, mTransform);
  
  const ezUInt32 tip = AddVertex(ezVec3(0, 0, size.z), ezVec3(0, 0, 1), ezVec2(0), color, iCustomIndex, mTransform);

  if (bCap)
  {
    AddPolygon(quad);
  }

  ezUInt32 tri[3];

  tri[0] = quad[1];
  tri[1] = quad[0];
  tri[2] = tip;
  AddPolygon(tri);

  tri[0] = quad[2];
  tri[1] = quad[1];
  tri[2] = tip;
  AddPolygon(tri);

  tri[0] = quad[3];
  tri[1] = quad[2];
  tri[2] = tip;
  AddPolygon(tri);

  tri[0] = quad[0];
  tri[1] = quad[3];
  tri[2] = tip;
  AddPolygon(tri);
}

void ezGeometry::AddGeodesicSphere(float fRadius, ezUInt8 uiSubDivisions, const ezColor& color, const ezMat4& mTransform, ezInt32 iCustomIndex)
{
  struct Triangle
  {
    Triangle(ezUInt32 i1, ezUInt32 i2, ezUInt32 i3)
    {
      m_uiIndex[0] = i1;
      m_uiIndex[1] = i2;
      m_uiIndex[2] = i3;
    }

    ezUInt32 m_uiIndex[3];
  };

  struct Edge
  {
    Edge() { }

    Edge(ezUInt32 id1, ezUInt32 id2)
    {
      m_uiVertex[0] = ezMath::Min(id1, id2);
      m_uiVertex[1] = ezMath::Max(id1, id2);
    }

    bool operator< (const Edge& rhs) const
    {
      if (m_uiVertex[0] < rhs.m_uiVertex[0])
        return true;
      if (m_uiVertex[0] > rhs.m_uiVertex[0])
        return false;
      return m_uiVertex[1] < rhs.m_uiVertex[1];
    }

    bool operator== (const Edge& rhs) const
    {
      return m_uiVertex[0] == rhs.m_uiVertex[0] && m_uiVertex[1] == rhs.m_uiVertex[1];
    }

    ezUInt32 m_uiVertex[2];
  };

  const ezUInt32 uiFirstVertex = m_Vertices.GetCount();

  ezInt32 iCurrentList = 0;
  ezDeque<Triangle> Tris[2];

  // create icosahedron
  {
    ezMat3 mRotZ, mRotY, mRotYh;
    mRotZ.SetRotationMatrixZ(ezAngle::Degree(60.0f));
    mRotY.SetRotationMatrixY(ezAngle::Degree(-360.0f / 5.0f));
    mRotYh.SetRotationMatrixY(ezAngle::Degree(-360.0f / 10.0f));

    ezUInt32 vert[12];
    ezVec3 vDir(0, 1, 0);

    vDir.Normalize();
    vert[0] = AddVertex(vDir * fRadius, vDir, ezVec2(0), color, iCustomIndex);

    vDir = mRotZ * vDir;

    for (ezInt32 i = 0; i < 5; ++i)
    {
      vDir.Normalize();
      vert[1 + i] = AddVertex(vDir * fRadius, vDir, ezVec2(0), color, iCustomIndex);
      vDir = mRotY * vDir;
    }

    vDir = mRotZ * vDir;
    vDir = mRotYh * vDir;

    for (ezInt32 i = 0; i < 5; ++i)
    {
      vDir.Normalize();
      vert[6 + i] = AddVertex(vDir * fRadius, vDir, ezVec2(0), color, iCustomIndex);
      vDir = mRotY * vDir;
    }

    vDir.Set(0, -1, 0);
    vDir.Normalize();
    vert[11] = AddVertex(vDir * fRadius, vDir, ezVec2(0), color, iCustomIndex);


    Tris[0].PushBack(Triangle(vert[0], vert[2], vert[1]));
    Tris[0].PushBack(Triangle(vert[0], vert[3], vert[2]));
    Tris[0].PushBack(Triangle(vert[0], vert[4], vert[3]));
    Tris[0].PushBack(Triangle(vert[0], vert[5], vert[4]));
    Tris[0].PushBack(Triangle(vert[0], vert[1], vert[5]));

    Tris[0].PushBack(Triangle(vert[1], vert[2], vert[6]));
    Tris[0].PushBack(Triangle(vert[2], vert[3], vert[7]));
    Tris[0].PushBack(Triangle(vert[3], vert[4], vert[8]));
    Tris[0].PushBack(Triangle(vert[4], vert[5], vert[9]));
    Tris[0].PushBack(Triangle(vert[5], vert[1], vert[10]));

    Tris[0].PushBack(Triangle(vert[2], vert[7], vert[6]));
    Tris[0].PushBack(Triangle(vert[3], vert[8], vert[7]));
    Tris[0].PushBack(Triangle(vert[4], vert[9], vert[8]));
    Tris[0].PushBack(Triangle(vert[5], vert[10], vert[9]));
    Tris[0].PushBack(Triangle(vert[6], vert[10], vert[1]));

    Tris[0].PushBack(Triangle(vert[7], vert[11], vert[6]));
    Tris[0].PushBack(Triangle(vert[8], vert[11], vert[7]));
    Tris[0].PushBack(Triangle(vert[9], vert[11], vert[8]));
    Tris[0].PushBack(Triangle(vert[10], vert[11], vert[9]));
    Tris[0].PushBack(Triangle(vert[6], vert[11], vert[10]));
  }

  ezMap<Edge, ezUInt32> NewVertices;

  // subdivide the icosahedron n times (splitting every triangle into 4 new triangles)
  for (ezUInt32 div = 0; div < uiSubDivisions; ++div)
  {
    // switch the last result and the new result
    const ezInt32 iPrevList = iCurrentList;
    iCurrentList = (iCurrentList + 1) % 2;

    Tris[iCurrentList].Clear();
    NewVertices.Clear();

    for (ezUInt32 tri = 0; tri < Tris[iPrevList].GetCount(); ++tri)
    {
      ezUInt32 uiVert[3] = { Tris[iPrevList][tri].m_uiIndex[0], Tris[iPrevList][tri].m_uiIndex[1], Tris[iPrevList][tri].m_uiIndex[2] };

      Edge Edges[3] = { Edge(uiVert[0], uiVert[1]), Edge(uiVert[1], uiVert[2]), Edge(uiVert[2], uiVert[0]) };

      ezUInt32 uiNewVert[3];

      // split each edge of the triangle in half
      for (ezUInt32 i = 0; i < 3; ++i)
      {
        // do not split an edge that was split before, we want shared vertices everywhere
        if (NewVertices.Find(Edges[i]).IsValid())
          uiNewVert[i] = NewVertices[Edges[i]];
        else
        {
          const ezVec3 vCenter = (m_Vertices[Edges[i].m_uiVertex[0]].m_vPosition + m_Vertices[Edges[i].m_uiVertex[1]].m_vPosition).GetNormalized();
          uiNewVert[i] = AddVertex(vCenter * fRadius, vCenter, ezVec2(0), color, iCustomIndex);

          NewVertices[Edges[i]] = uiNewVert[i];
        }
      }

      // now we turn one triangle into 4 smaller ones
      Tris[iCurrentList].PushBack(Triangle(uiVert[0], uiNewVert[0], uiNewVert[2]));
      Tris[iCurrentList].PushBack(Triangle(uiNewVert[0], uiVert[1], uiNewVert[1]));
      Tris[iCurrentList].PushBack(Triangle(uiNewVert[1], uiVert[2], uiNewVert[2]));

      Tris[iCurrentList].PushBack(Triangle(uiNewVert[0], uiNewVert[1], uiNewVert[2]));
    }
  }

  // add the final list of triangles to the output
  for (ezUInt32 tri = 0; tri < Tris[iCurrentList].GetCount(); ++tri)
  {
    AddPolygon(Tris[iCurrentList][tri].m_uiIndex);
  }

  // finally apply the user transformation on the new vertices
  TransformVertices(mTransform, uiFirstVertex);
}

void ezGeometry::AddCylinder(float fRadiusTop, float fRadiusBottom, float fHeight, bool bCapTop, bool bCapBottom, ezUInt16 uiSegments, const ezColor& color, const ezMat4& mTransform, ezInt32 iCustomIndex, ezAngle fraction)
{
  EZ_ASSERT_DEV(uiSegments >= 3, "Cannot create a cylinder with only %u segments", uiSegments);
  EZ_ASSERT_DEV(fraction.GetDegree() >= 0.0f, "A cylinder cannot be built with more less than 0 degree");
  EZ_ASSERT_DEV(fraction.GetDegree() <= 360.0f, "A cylinder cannot be built with more than 360 degree");

  const bool bIsFraction = fraction.GetDegree() < 360.0f;

  ezHybridArray<ezUInt32, 512> VertsTop;
  ezHybridArray<ezUInt32, 512> VertsBottom;

  const ezVec3 vTopCenter(0, 0, fHeight * 0.5f);
  const ezVec3 vBottomCenter(0, 0, -fHeight * 0.5f);

  const ezAngle fDegStep = ezAngle::Degree(fraction.GetDegree() / uiSegments);

  if (bIsFraction)
  {
    VertsTop.PushBack(AddVertex(vTopCenter, ezVec3(0, 1, 0), ezVec2(0), color, iCustomIndex, mTransform));
    VertsBottom.PushBack(AddVertex(vBottomCenter, ezVec3(0, -1, 0), ezVec2(0), color, iCustomIndex, mTransform));

    ++uiSegments;
  }

  for (ezInt32 i = uiSegments - 1; i >= 0; --i)
  {
    const ezAngle deg = (float) i * fDegStep;

    float fU = ((float)i / (float)(uiSegments)) * 2.0f;

    if (fU > 1.0f)
      fU = 2.0f - fU;

    const float fX = ezMath::Cos(deg);
    const float fY = ezMath::Sin(deg);

    ezVec3 vDir(fX, fY, 0);

    VertsTop.PushBack(AddVertex(vTopCenter + vDir * fRadiusTop, vDir, ezVec2(fU, 0), color, iCustomIndex, mTransform));
    VertsBottom.PushBack(AddVertex(vBottomCenter + vDir * fRadiusBottom, vDir, ezVec2(fU, 1), color, iCustomIndex, mTransform));
  }

  if (bIsFraction)
    ++uiSegments;

  ezUInt32 uiPrevSeg = VertsBottom.GetCount() - 1;

  for (ezUInt32 i = 0; i < uiSegments; ++i)
  {
    ezUInt32 quad[4];
    quad[0] = VertsBottom[uiPrevSeg];
    quad[1] = VertsTop[uiPrevSeg];
    quad[2] = VertsTop[i];
    quad[3] = VertsBottom[i];

    uiPrevSeg = i;

    AddPolygon(quad);
  }

  if (bCapBottom)
  {
    AddPolygon(VertsBottom);
  }

  if (bCapTop)
  {
    VertsBottom.Clear();

    VertsBottom.PushBack(VertsTop[0]);

    for (ezUInt32 i = VertsTop.GetCount(); i > 1; --i)
      VertsBottom.PushBack(VertsTop[i - 1]);

    AddPolygon(VertsBottom);
  }

}

void ezGeometry::AddCone(float fRadius, float fHeight, bool bCap, ezUInt16 uiSegments, const ezColor& color, const ezMat4& mTransform, ezInt32 iCustomIndex)
{
  EZ_ASSERT_DEV(uiSegments >= 3, "Cannot create a cone with only %u segments", uiSegments);

  ezHybridArray<ezUInt32, 512> VertsBottom;

  const ezAngle fDegStep = ezAngle::Degree(360.0f / uiSegments);

  const ezUInt32 uiTip = AddVertex(ezVec3(0, 0, fHeight), ezVec3(0, 0, 1), ezVec2(0), color, iCustomIndex, mTransform);

  for (ezUInt32 i = 0; i < uiSegments; ++i)
  {
    const ezAngle deg = (float) i * fDegStep;

    ezVec3 vDir(ezMath::Cos(deg), ezMath::Sin(deg), 0);

    VertsBottom.PushBack(AddVertex(vDir * fRadius, vDir, ezVec2(0), color, iCustomIndex, mTransform));
  }

  ezUInt32 uiPrevSeg = uiSegments - 1;

  for (ezUInt32 i = 0; i < uiSegments; ++i)
  {
    ezUInt32 tri[3];
    tri[0] = VertsBottom[uiPrevSeg];
    tri[1] = uiTip;
    tri[2] = VertsBottom[i];

    uiPrevSeg = i;

    AddPolygon(tri);
  }

  if (bCap)
  {
    AddPolygon(VertsBottom);
  }
}

void ezGeometry::AddSphere(float fRadius, ezUInt16 uiSegments, ezUInt16 uiStacks, const ezColor& color, const ezMat4& mTransform, ezInt32 iCustomIndex)
{
  EZ_ASSERT_DEV(uiSegments >= 3, "Sphere must have at least 3 segments");
  EZ_ASSERT_DEV(uiStacks >= 2, "Sphere must have at least 2 stacks");

  const ezAngle fDegreeDiffSegments = ezAngle::Degree(360.0f / (float) (uiSegments));
  const ezAngle fDegreeDiffStacks = ezAngle::Degree(180.0f / (float) (uiStacks));

  const ezUInt32 uiFirstVertex = m_Vertices.GetCount();

  // first create all the vertex positions
  for (ezUInt32 st = 1; st < uiStacks; ++st)
  {
    const ezAngle fDegreeStack = ezAngle::Degree(-90.0f + (st * fDegreeDiffStacks.GetDegree()));
    const float fCosDS = ezMath::Cos(fDegreeStack);
    const float fSinDS = ezMath::Sin(fDegreeStack);
    const float fY = -fSinDS * fRadius;

    const float fV = (float)st / (float)uiStacks;

    for (ezUInt32 sp = 0; sp < uiSegments; ++sp)
    {
      float fU = ((float)sp / (float)(uiSegments)) * 2.0f;

      if (fU > 1.0f)
        fU = 2.0f - fU;

      const ezAngle fDegree = (float)sp * fDegreeDiffSegments;

      ezVec3 vPos;
      vPos.x = ezMath::Cos(fDegree) * fRadius * fCosDS;
      vPos.y =-ezMath::Sin(fDegree) * fRadius * fCosDS;
      vPos.z = fY;

      AddVertex(vPos, vPos.GetNormalized(), ezVec2(fU, fV), color, iCustomIndex, mTransform);
    }
  }

  ezUInt32 uiTopVertex = AddVertex(ezVec3(0, 0, fRadius), ezVec3(0, 0, 1), ezVec2(0.5f, 0), color, iCustomIndex, mTransform);

  ezUInt32 tri[3];
  ezUInt32 quad[4];

  // now create the top cone
  for (ezUInt32 p = 0; p < uiSegments; ++p)
  {
    tri[0] = uiTopVertex;
    tri[1] = uiFirstVertex + ((p + 1) % uiSegments);
    tri[2] = uiFirstVertex + p;

    AddPolygon(tri);
  }

  // now create the stacks in the middle

  for (ezUInt16 st = 0; st < uiStacks - 2; ++st)
  {
    const ezUInt32 uiRowBottom = uiSegments * st;
    const ezUInt32 uiRowTop = uiSegments * (st + 1);

    for (ezInt32 i = 0; i < uiSegments; ++i)
    {
      quad[0] = uiFirstVertex + (uiRowTop + ((i + 1) % uiSegments));
      quad[1] = uiFirstVertex + (uiRowTop + i);
      quad[2] = uiFirstVertex + (uiRowBottom + i);
      quad[3] = uiFirstVertex + (uiRowBottom + ((i + 1) % uiSegments));

      AddPolygon(quad);
    }
  }

  ezUInt32 uiBottomVertex = AddVertex(ezVec3(0, 0, -fRadius), ezVec3(0, 0, -1), ezVec2(0.5f, 1), color, iCustomIndex, mTransform);
  const ezInt32 iTopStack = uiSegments * (uiStacks - 2);

  // now create the bottom cone
  for (ezUInt32 p = 0; p < uiSegments; ++p)
  {
    tri[0] = uiBottomVertex;
    tri[1] = uiFirstVertex + (iTopStack + p);
    tri[2] = uiFirstVertex + (iTopStack + ((p + 1) % uiSegments));

    AddPolygon(tri);
  }
}

void ezGeometry::AddHalfSphere(float fRadius, ezUInt16 uiSegments, ezUInt16 uiStacks, bool bCap, const ezColor& color, const ezMat4& mTransform, ezInt32 iCustomIndex)
{
  EZ_ASSERT_DEV(uiSegments >= 3, "Sphere must have at least 3 segments");
  EZ_ASSERT_DEV(uiStacks >= 1, "Sphere must have at least 1 stacks");

  const ezAngle fDegreeDiffSegments = ezAngle::Degree(360.0f / (float) (uiSegments));
  const ezAngle fDegreeDiffStacks = ezAngle::Degree(90.0f / (float) (uiStacks));

  const ezUInt32 uiFirstVertex = m_Vertices.GetCount();

  // first create all the vertex positions
  for (ezUInt32 st = 0; st < uiStacks; ++st)
  {
    const ezAngle fDegreeStack = ezAngle::Degree(-90.0f + ((st+1) * fDegreeDiffStacks.GetDegree()));
    const float fCosDS = ezMath::Cos(fDegreeStack);
    const float fSinDS = ezMath::Sin(fDegreeStack);
    const float fY = -fSinDS * fRadius;

    for (ezUInt32 sp = 0; sp < uiSegments; ++sp)
    {
      // the vertices for the bottom disk
      const ezAngle fDegree = (float)sp * fDegreeDiffSegments;

      ezVec3 vPos;
      vPos.x = ezMath::Cos(fDegree) * fRadius * fCosDS;
      vPos.y = fY;
      vPos.z = ezMath::Sin(fDegree) * fRadius * fCosDS;

      AddVertex(vPos, vPos.GetNormalized(), ezVec2(0), color, iCustomIndex, mTransform);
    }
  }

  ezUInt32 uiTopVertex = AddVertex(ezVec3(0, fRadius, 0), ezVec3(0, 1, 0), ezVec2(0), color, iCustomIndex, mTransform);

  ezUInt32 tri[3];
  ezUInt32 quad[4];

  // now create the top cone
  for (ezUInt32 p = 0; p < uiSegments; ++p)
  {
    tri[0] = uiTopVertex;
    tri[1] = uiFirstVertex + ((p + 1) % uiSegments);
    tri[2] = uiFirstVertex + p;

    AddPolygon(tri);
  }

  // now create the stacks in the middle

  for (ezUInt16 st = 0; st < uiStacks - 1; ++st)
  {
    const ezUInt32 uiRowBottom = uiSegments * st;
    const ezUInt32 uiRowTop = uiSegments * (st + 1);

    for (ezInt32 i = 0; i < uiSegments; ++i)
    {
      quad[0] = uiFirstVertex + (uiRowTop + ((i + 1) % uiSegments));
      quad[1] = uiFirstVertex + (uiRowTop + i);
      quad[2] = uiFirstVertex + (uiRowBottom + i);
      quad[3] = uiFirstVertex + (uiRowBottom + ((i + 1) % uiSegments));

      AddPolygon(quad);
    }
  }

  if (bCap)
  {
    ezHybridArray<ezUInt32, 256> uiCap;

    for (ezUInt32 i = uiTopVertex - uiSegments; i < uiTopVertex; ++i)
      uiCap.PushBack(i);

    AddPolygon(uiCap);
  }

}

void ezGeometry::AddCapsule(float fRadius, float fHeight, ezUInt16 uiSegments, ezUInt16 uiStacks, const ezColor& color, const ezMat4& mTransform, ezInt32 iCustomIndex)
{
  EZ_ASSERT_DEV(uiSegments >= 3, "Capsule must have at least 3 segments");
  EZ_ASSERT_DEV(uiStacks >= 1, "Capsule must have at least 1 stacks");
  EZ_ASSERT_DEV(fHeight >= 0.0f, "Height must be positive");

  const ezAngle fDegreeDiffSegments = ezAngle::Degree(360.0f / (float) (uiSegments));
  const ezAngle fDegreeDiffStacks = ezAngle::Degree(90.0f / (float) (uiStacks));

  const ezUInt32 uiFirstVertex = m_Vertices.GetCount();

  // first create all the vertex positions
  const float fDegreeStepSlices = 360.0f / (float) (uiSegments);

  float fOffset = fHeight * 0.5f;

  //for (ezUInt32 h = 0; h < 2; ++h)
  {
    for (ezUInt32 st = 0; st < uiStacks; ++st)
    {
      const ezAngle fDegreeStack = ezAngle::Degree(-90.0f + ((st + 1) * fDegreeDiffStacks.GetDegree()));
      const float fCosDS = ezMath::Cos(fDegreeStack);
      const float fSinDS = ezMath::Sin(fDegreeStack);
      const float fY = -fSinDS * fRadius;

      for (ezUInt32 sp = 0; sp < uiSegments; ++sp)
      {
        const ezAngle fDegree = ezAngle::Degree(sp * fDegreeStepSlices);

        ezVec3 vPos;
        vPos.x = ezMath::Cos(fDegree) * fRadius * fCosDS;
        vPos.y = fY + fOffset;
        vPos.z = ezMath::Sin(fDegree) * fRadius * fCosDS;

        AddVertex(vPos, vPos.GetNormalized(), ezVec2(0), color, iCustomIndex, mTransform);
      }
    }

    fOffset -= fHeight;

    for (ezUInt32 st = 0; st < uiStacks; ++st)
    {
      const ezAngle fDegreeStack = ezAngle::Degree(0.0f - (st * fDegreeDiffStacks.GetDegree()));
      const float fCosDS = ezMath::Cos(fDegreeStack);
      const float fSinDS = ezMath::Sin(fDegreeStack);
      const float fY = fSinDS * fRadius;

      for (ezUInt32 sp = 0; sp < uiSegments; ++sp)
      {
        const ezAngle fDegree = ezAngle::Degree(sp * fDegreeStepSlices);

        ezVec3 vPos;
        vPos.x = ezMath::Cos(fDegree) * fRadius * fCosDS;
        vPos.y = fY + fOffset;
        vPos.z = ezMath::Sin(fDegree) * fRadius * fCosDS;

        AddVertex(vPos, vPos.GetNormalized(), ezVec2(0), color, iCustomIndex, mTransform);
      }
    }
  }

  ezUInt32 uiTopVertex = AddVertex(ezVec3(0, fRadius + fHeight * 0.5f, 0), ezVec3(0, 1, 0), ezVec2(0), color, iCustomIndex, mTransform);
  ezUInt32 uiBottomVertex = AddVertex(ezVec3(0, -fRadius - fHeight * 0.5f, 0), ezVec3(0, -1, 0), ezVec2(0), color, iCustomIndex, mTransform);

  ezUInt32 tri[3];
  ezUInt32 quad[4];

  // now create the top cone
  for (ezUInt32 p = 0; p < uiSegments; ++p)
  {
    tri[0] = uiTopVertex;
    tri[1] = uiFirstVertex + ((p + 1) % uiSegments);
    tri[2] = uiFirstVertex + p;

    AddPolygon(tri);
  }

  // now create the stacks in the middle

  for (ezUInt16 st = 0; st < uiStacks * 2 - 1; ++st)
  {
    const ezUInt32 uiRowBottom = uiSegments * st;
    const ezUInt32 uiRowTop = uiSegments * (st + 1);

    for (ezInt32 i = 0; i < uiSegments; ++i)
    {
      quad[0] = uiFirstVertex + (uiRowTop + ((i + 1) % uiSegments));
      quad[1] = uiFirstVertex + (uiRowTop + i);
      quad[2] = uiFirstVertex + (uiRowBottom + i);
      quad[3] = uiFirstVertex + (uiRowBottom + ((i + 1) % uiSegments));

      AddPolygon(quad);
    }
  }

  const ezInt32 iBottomStack = uiSegments * (uiStacks * 2 - 1);

  // now create the bottom cone
  for (ezUInt32 p = 0; p < uiSegments; ++p)
  {
    tri[0] = uiBottomVertex;
    tri[1] = uiFirstVertex + (iBottomStack + p);
    tri[2] = uiFirstVertex + (iBottomStack + ((p + 1) % uiSegments));

    AddPolygon(tri);
  }

}

void ezGeometry::AddTorus(float fInnerRadius, float fOuterRadius, ezUInt16 uiSegments, ezUInt16 uiSegmentDetail, const ezColor& color, const ezMat4& mTransform, ezInt32 iCustomIndex)
{
  EZ_ASSERT_DEV(fInnerRadius < fOuterRadius, "Inner radius must be smaller than outer radius. Doh!");
  EZ_ASSERT_DEV(uiSegments >= 3, "Invalid number of segments.");
  EZ_ASSERT_DEV(uiSegmentDetail >= 3, "Invalid segment detail value.");

  const float fCylinderRadius = (fOuterRadius - fInnerRadius) * 0.5f;
  const float fLoopRadius = fInnerRadius + fCylinderRadius;

  const ezAngle fAngleStepSegment = ezAngle::Degree(360.0f / uiSegments);
  const ezAngle fAngleStepCylinder = ezAngle::Degree(360.0f / uiSegmentDetail);

  // this is the loop for the torus ring
  for (ezUInt16 seg = 0; seg < uiSegments; ++seg)
  {
    const ezAngle fAngle = seg * fAngleStepSegment;

    const float fSinAngle = ezMath::Sin(fAngle);
    const float fCosAngle = ezMath::Cos(fAngle);

    const ezVec3 vLoopPos = ezVec3(fSinAngle, 0, fCosAngle) * fLoopRadius;

    // this is the loop to go round the cylinder
    for (ezUInt16 p = 0; p < uiSegmentDetail; ++p)
    {
      const ezAngle fCylinderAngle = p * fAngleStepCylinder;

      const ezVec3 vDir(ezMath::Cos(fCylinderAngle) * fSinAngle, ezMath::Sin(fCylinderAngle), ezMath::Cos(fCylinderAngle) * fCosAngle);

      const ezVec3 vPos = vLoopPos + fCylinderRadius * vDir;

      AddVertex(vPos, vDir, ezVec2(0), color, iCustomIndex, mTransform);
    }
  }


  for (ezUInt16 seg = 0; seg < uiSegments; ++seg)
  {
    const ezUInt16 rs0 = seg * uiSegmentDetail;
    const ezUInt16 rs1 = ((seg + 1) % uiSegments) * uiSegmentDetail;

    for (ezUInt16 p = 0; p < uiSegmentDetail; ++p)
    {
      const ezUInt16 p1 = (p + 1) % uiSegmentDetail;

      ezUInt32 quad[4];

      quad[0] = rs1 + p;
      quad[1] = rs1 + p1;
      quad[2] = rs0 + p1;
      quad[3] = rs0 + p;

      AddPolygon(quad);
    }
  }
}





EZ_STATICLINK_FILE(CoreUtils, CoreUtils_Geometry_Implementation_GeomUtils);

