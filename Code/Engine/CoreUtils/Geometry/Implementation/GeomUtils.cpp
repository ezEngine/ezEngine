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

void ezGeometry::SetAllVertexCustomIndex(ezInt32 iCustomIndex)
{
  for (ezUInt32 v = 0; v < m_Vertices.GetCount(); ++v)
    m_Vertices[v].m_iCustomIndex = iCustomIndex;
}

void ezGeometry::SetAllVertexColor(const ezColor8UNorm& color)
{
  for (ezUInt32 v = 0; v < m_Vertices.GetCount(); ++v)
    m_Vertices[v].m_Color = color;
}

void ezGeometry::Transform(const ezMat4& mTransform, bool bTransformPolyNormals)
{
  for (ezUInt32 v = 0; v < m_Vertices.GetCount(); ++v)
  {
    m_Vertices[v].m_vPosition = mTransform.TransformPosition(m_Vertices[v].m_vPosition);
    m_Vertices[v].m_vNormal   = mTransform.TransformDirection(m_Vertices[v].m_vNormal);
  }

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

void ezGeometry::AddRectXY(const ezColor8UNorm& color, const ezMat4& mTransform, ezInt32 iCustomIndex)
{
  ezUInt32 idx[4];

  idx[0] = AddVertex(ezVec3(-1, -1, 0), ezVec3(0, 0, -1), color, iCustomIndex, mTransform);
  idx[1] = AddVertex(ezVec3( 1, -1, 0), ezVec3(0, 0, -1), color, iCustomIndex, mTransform);
  idx[2] = AddVertex(ezVec3( 1,  1, 0), ezVec3(0, 0, -1), color, iCustomIndex, mTransform);
  idx[3] = AddVertex(ezVec3(-1,  1, 0), ezVec3(0, 0, -1), color, iCustomIndex, mTransform);

  AddPolygon(idx);
}

