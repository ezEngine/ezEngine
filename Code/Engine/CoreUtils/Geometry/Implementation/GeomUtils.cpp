#include <CoreUtils/PCH.h>
#include <CoreUtils/Geometry/GeomUtils.h>

void ezGeometry::Clear()
{
  m_Vertices.Clear();
  m_Polygons.Clear();
  m_Lines.Clear();
}

ezUInt32 ezGeometry::AddVertex(const ezVec3& vPos, const ezVec3& vNormal, const ezColor8UNorm& color, ezInt32 iCustomIndex)
{
  Vertex v;
  v.m_vPosition = vPos;
  v.m_vNormal = vNormal;
  v.m_Color = color;
  v.m_iCustomIndex = iCustomIndex;

  m_Vertices.PushBack(v);

  return m_Vertices.GetCount() - 1;
}

ezUInt32 ezGeometry::AddVertex(const ezVec3& vPos, const ezVec3& vNormal, const ezColor8UNorm& color, ezInt32 iCustomIndex, const ezMat4& mTransform)
{
  return AddVertex(mTransform.TransformPosition(vPos), mTransform.TransformDirection(vNormal), color, iCustomIndex);
}

void ezGeometry::AddPolygon(const ezArrayPtr<ezUInt32>& Vertices)
{
  EZ_ASSERT(Vertices.GetCount() >= 3, "Polygon must have at least 3 vertices, not %u", Vertices.GetCount());

  for (ezUInt32 v = 0; v < Vertices.GetCount(); ++v)
  {
    EZ_ASSERT(Vertices[v] < m_Vertices.GetCount(), "Invalid vertex index %u, geometry only has %u vertices", Vertices[v], m_Vertices.GetCount());
  }

  m_Polygons.SetCount(m_Polygons.GetCount() + 1); // could use 'ExpandOne' here

  m_Polygons.PeekBack().m_Vertices = Vertices;
}

void ezGeometry::AddLine(ezUInt32 uiStartVertex, ezUInt32 uiEndVertex)
{
  EZ_ASSERT(uiStartVertex < m_Vertices.GetCount(), "Invalid vertex index %u, geometry only has %u vertices", uiStartVertex, m_Vertices.GetCount());
  EZ_ASSERT(uiEndVertex   < m_Vertices.GetCount(), "Invalid vertex index %u, geometry only has %u vertices", uiEndVertex  , m_Vertices.GetCount());

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

void ezGeometry::SetAllVertexColor(const ezColor8UNorm& color, ezUInt32 uiFirstVertex)
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
    m_Vertices[v].m_vNormal   = mTransform.TransformDirection(m_Vertices[v].m_vNormal);
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

void ezGeometry::AddRectXY(float fSizeX, float fSizeY, const ezColor8UNorm& color, const ezMat4& mTransform, ezInt32 iCustomIndex)
{
  const float fHalfSizeX = fSizeX * 0.5f;
  const float fHalfSizeY = fSizeY * 0.5f;

  ezUInt32 idx[4];

  idx[0] = AddVertex(ezVec3(-fHalfSizeX, -fHalfSizeY, 0), ezVec3(0, 0, 1), color, iCustomIndex, mTransform);
  idx[1] = AddVertex(ezVec3( fHalfSizeX, -fHalfSizeY, 0), ezVec3(0, 0, 1), color, iCustomIndex, mTransform);
  idx[2] = AddVertex(ezVec3( fHalfSizeX,  fHalfSizeY, 0), ezVec3(0, 0, 1), color, iCustomIndex, mTransform);
  idx[3] = AddVertex(ezVec3(-fHalfSizeX,  fHalfSizeY, 0), ezVec3(0, 0, 1), color, iCustomIndex, mTransform);

  AddPolygon(idx);
}

void ezGeometry::AddGeodesicSphere(ezUInt8 uiSubDivisions, const ezColor8UNorm& color, const ezMat4& mTransform, ezInt32 iCustomIndex)
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

  // create ikosaedron
  {
    ezMat3 mRotZ, mRotY, mRotYh;
    mRotZ.SetRotationMatrixZ(ezAngle::Degree(60.0f));
    mRotY.SetRotationMatrixY(ezAngle::Degree(-360.0f / 5.0f));
    mRotYh.SetRotationMatrixY(ezAngle::Degree(-360.0f / 10.0f));

    ezUInt32 vert[12];
    ezVec3 vDir(0, 1, 0);

    vDir.Normalize();
    vert[0] = AddVertex(vDir, vDir, color, iCustomIndex);

    vDir = mRotZ * vDir;

    for (ezInt32 i = 0; i < 5; ++i)
    {
      vDir.Normalize();
      vert[1 + i] = AddVertex(vDir, vDir, color, iCustomIndex);
      vDir = mRotY * vDir;
    }

    vDir = mRotZ * vDir;
    vDir = mRotYh * vDir;

    for (ezInt32 i = 0; i < 5; ++i)
    {
      vDir.Normalize();
      vert[6 + i] = AddVertex(vDir, vDir, color, iCustomIndex);
      vDir = mRotY * vDir;
    }

    vDir.Set(0, -1, 0);
    vDir.Normalize();
    vert[11] = AddVertex(vDir, vDir, color, iCustomIndex);


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

  // subdivide the ikosaeder n times (spliting every triangle into 4 new triangles)
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
          uiNewVert[i] = AddVertex(vCenter, vCenter, color, iCustomIndex);

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






