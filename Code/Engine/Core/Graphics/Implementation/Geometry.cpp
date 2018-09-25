#include <PCH.h>

#include <Core/Graphics/Geometry.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Math/Quat.h>
#include <ThirdParty/mikktspace/mikktspace.h>

bool ezGeometry::Vertex::operator<(const ezGeometry::Vertex& rhs) const
{
  if (m_vPosition < rhs.m_vPosition)
    return true;

  if (m_vPosition == rhs.m_vPosition)
  {
    if (m_vNormal < rhs.m_vNormal)
      return true;

    if (m_vNormal == rhs.m_vNormal)
    {
      if (m_vTangent < rhs.m_vTangent)
        return true;

      if (m_vTangent == rhs.m_vTangent)
      {
        if (m_vTexCoord < rhs.m_vTexCoord)
          return true;

        if (m_vTexCoord == rhs.m_vTexCoord)
        {
          if (m_Color < rhs.m_Color)
            return true;

          if (m_Color == rhs.m_Color)
          {
            return m_iCustomIndex < rhs.m_iCustomIndex;
          }
        }
      }
    }
  }

  return false;
}

bool ezGeometry::Vertex::operator==(const ezGeometry::Vertex& rhs) const
{
  if (m_vPosition != rhs.m_vPosition)
    return false;
  if (m_vNormal != rhs.m_vNormal)
    return false;
  if (m_vTangent != rhs.m_vTangent)
    return false;
  if (m_vTexCoord != rhs.m_vTexCoord)
    return false;
  if (m_Color != rhs.m_Color)
    return false;
  if (m_iCustomIndex != rhs.m_iCustomIndex)
    return false;

  return true;
}

void ezGeometry::Polygon::FlipWinding()
{
  const ezUInt32 uiCount = m_Vertices.GetCount();
  const ezUInt32 uiHalfCount = uiCount / 2;
  for (ezUInt32 i = 0; i < uiHalfCount; i++)
  {
    ezMath::Swap(m_Vertices[i], m_Vertices[uiCount - i - 1]);
  }
}

void ezGeometry::Clear()
{
  m_Vertices.Clear();
  m_Polygons.Clear();
  m_Lines.Clear();
}

ezUInt32 ezGeometry::AddVertex(const ezVec3& vPos, const ezVec3& vNormal, const ezVec2& vTexCoord, const ezColor& color,
                               ezInt32 iCustomIndex)
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

ezUInt32 ezGeometry::AddVertex(const ezVec3& vPos, const ezVec3& vNormal, const ezVec2& vTexCoord, const ezColor& color,
                               ezInt32 iCustomIndex, const ezMat4& mTransform)
{
  return AddVertex(mTransform.TransformPosition(vPos), mTransform.TransformDirection(vNormal).GetNormalized(), vTexCoord, color,
                   iCustomIndex);
}

void ezGeometry::AddPolygon(const ezArrayPtr<ezUInt32>& Vertices, bool bFlipWinding)
{
  EZ_ASSERT_DEV(Vertices.GetCount() >= 3, "Polygon must have at least 3 vertices, not {0}", Vertices.GetCount());

  for (ezUInt32 v = 0; v < Vertices.GetCount(); ++v)
  {
    EZ_ASSERT_DEV(Vertices[v] < m_Vertices.GetCount(), "Invalid vertex index {0}, geometry only has {1} vertices", Vertices[v],
                  m_Vertices.GetCount());
  }

  m_Polygons.SetCount(m_Polygons.GetCount() + 1); // could use 'ExpandOne' here

  m_Polygons.PeekBack().m_Vertices = Vertices;
  if (bFlipWinding)
  {
    m_Polygons.PeekBack().FlipWinding();
  }
}

void ezGeometry::AddLine(ezUInt32 uiStartVertex, ezUInt32 uiEndVertex)
{
  EZ_ASSERT_DEV(uiStartVertex < m_Vertices.GetCount(), "Invalid vertex index {0}, geometry only has {1} vertices", uiStartVertex,
                m_Vertices.GetCount());
  EZ_ASSERT_DEV(uiEndVertex < m_Vertices.GetCount(), "Invalid vertex index {0}, geometry only has {1} vertices", uiEndVertex,
                m_Vertices.GetCount());

  Line l;
  l.m_uiStartVertex = uiStartVertex;
  l.m_uiEndVertex = uiEndVertex;

  m_Lines.PushBack(l);
}


void ezGeometry::TriangulatePolygons(ezUInt32 uiMaxVerticesInPolygon /*= 3*/)
{
  EZ_ASSERT_DEV(uiMaxVerticesInPolygon >= 3, "Can't triangulate polygons that are already triangles.");
  uiMaxVerticesInPolygon = ezMath::Max<ezUInt32>(uiMaxVerticesInPolygon, 3);

  const ezUInt32 uiNumPolys = m_Polygons.GetCount();

  for (ezUInt32 p = 0; p < uiNumPolys; ++p)
  {
    const auto& poly = m_Polygons[p];

    const ezUInt32 uiNumVerts = poly.m_Vertices.GetCount();
    if (uiNumVerts > uiMaxVerticesInPolygon)
    {
      for (ezUInt32 v = 2; v < uiNumVerts; ++v)
      {
        auto& tri = m_Polygons.ExpandAndGetRef();
        tri.m_vNormal = poly.m_vNormal;
        tri.m_Vertices.SetCountUninitialized(3);
        tri.m_Vertices[0] = poly.m_Vertices[0];
        tri.m_Vertices[1] = poly.m_Vertices[v - 1];
        tri.m_Vertices[2] = poly.m_Vertices[v];
      }

      m_Polygons.RemoveAtSwap(p);
    }
  }
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

struct TangentContext
{
  TangentContext(ezGeometry* pGeom)
      : m_pGeom(pGeom)
  {
    m_Polygons = m_pGeom->GetPolygons();
  }

  static int getNumFaces(const SMikkTSpaceContext* pContext)
  {
    TangentContext& context = *static_cast<TangentContext*>(pContext->m_pUserData);
    return context.m_pGeom->GetPolygons().GetCount();
  }
  static int getNumVerticesOfFace(const SMikkTSpaceContext* pContext, const int iFace)
  {
    TangentContext& context = *static_cast<TangentContext*>(pContext->m_pUserData);
    return context.m_pGeom->GetPolygons()[iFace].m_Vertices.GetCount();
  }
  static void getPosition(const SMikkTSpaceContext* pContext, float fvPosOut[], const int iFace, const int iVert)
  {
    TangentContext& context = *static_cast<TangentContext*>(pContext->m_pUserData);
    ezUInt32 iVertexIndex = context.m_pGeom->GetPolygons()[iFace].m_Vertices[iVert];
    const ezVec3& pos = context.m_pGeom->GetVertices()[iVertexIndex].m_vPosition;
    fvPosOut[0] = pos.x;
    fvPosOut[1] = pos.y;
    fvPosOut[2] = pos.z;
  }
  static void getNormal(const SMikkTSpaceContext* pContext, float fvNormOut[], const int iFace, const int iVert)
  {
    TangentContext& context = *static_cast<TangentContext*>(pContext->m_pUserData);
    ezUInt32 iVertexIndex = context.m_pGeom->GetPolygons()[iFace].m_Vertices[iVert];
    const ezVec3& normal = context.m_pGeom->GetVertices()[iVertexIndex].m_vNormal;
    fvNormOut[0] = normal.x;
    fvNormOut[1] = normal.y;
    fvNormOut[2] = normal.z;
  }
  static void getTexCoord(const SMikkTSpaceContext* pContext, float fvTexcOut[], const int iFace, const int iVert)
  {
    TangentContext& context = *static_cast<TangentContext*>(pContext->m_pUserData);
    ezUInt32 iVertexIndex = context.m_pGeom->GetPolygons()[iFace].m_Vertices[iVert];
    const ezVec2& tex = context.m_pGeom->GetVertices()[iVertexIndex].m_vTexCoord;
    fvTexcOut[0] = tex.x;
    fvTexcOut[1] = tex.y;
  }
  static void setTSpaceBasic(const SMikkTSpaceContext* pContext, const float fvTangent[], const float fSign, const int iFace,
                             const int iVert)
  {
    TangentContext& context = *static_cast<TangentContext*>(pContext->m_pUserData);
    ezUInt32 iVertexIndex = context.m_pGeom->GetPolygons()[iFace].m_Vertices[iVert];
    ezGeometry::Vertex v = context.m_pGeom->GetVertices()[iVertexIndex];
    v.m_vTangent.x = fvTangent[0];
    v.m_vTangent.y = fvTangent[1];
    v.m_vTangent.z = fvTangent[2];
    if (fSign < 0)
    {
      v.m_vTangent *= 1.7320508075688772935274463415059f; // ezMath::Root(3, 2)
    }

    bool existed = false;
    auto it = context.m_VertMap.FindOrAdd(v, &existed);
    if (!existed)
    {
      it.Value() = context.m_Vertices.GetCount();
      context.m_Vertices.PushBack(v);
    }
    ezUInt32 iNewVertexIndex = it.Value();
    context.m_Polygons[iFace].m_Vertices[iVert] = iNewVertexIndex;
  }

  static void setTSpace(const SMikkTSpaceContext* pContext, const float fvTangent[], const float fvBiTangent[], const float fMagS,
                        const float fMagT, const tbool bIsOrientationPreserving, const int iFace, const int iVert)
  {
    int i = 0;
    (void)i;
  }

  ezGeometry* m_pGeom;
  ezMap<ezGeometry::Vertex, ezUInt32> m_VertMap;
  ezDeque<ezGeometry::Vertex> m_Vertices;
  ezDeque<ezGeometry::Polygon> m_Polygons;
};

void ezGeometry::ComputeTangents()
{
  for (ezUInt32 i = 0; i < m_Polygons.GetCount(); ++i)
  {
    if (m_Polygons[i].m_Vertices.GetCount() > 4)
    {
      ezLog::Error("Tangent generation does not support polygons with more than 4 vertices");
      break;
    }
  }

  SMikkTSpaceInterface sMikkTInterface;
  sMikkTInterface.m_getNumFaces = &TangentContext::getNumFaces;
  sMikkTInterface.m_getNumVerticesOfFace = &TangentContext::getNumVerticesOfFace;
  sMikkTInterface.m_getPosition = &TangentContext::getPosition;
  sMikkTInterface.m_getNormal = &TangentContext::getNormal;
  sMikkTInterface.m_getTexCoord = &TangentContext::getTexCoord;
  sMikkTInterface.m_setTSpaceBasic = &TangentContext::setTSpaceBasic;
  sMikkTInterface.m_setTSpace = &TangentContext::setTSpace;
  TangentContext context(this);

  SMikkTSpaceContext sMikkTContext;
  sMikkTContext.m_pInterface = &sMikkTInterface;
  sMikkTContext.m_pUserData = &context;

  genTangSpaceDefault(&sMikkTContext);
  m_Polygons = std::move(context.m_Polygons);
  m_Vertices = std::move(context.m_Vertices);
}

ezUInt32 ezGeometry::CalculateTriangleCount() const
{
  const ezUInt32 numPolys = m_Polygons.GetCount();
  ezUInt32 numTris = 0;

  for (ezUInt32 p = 0; p < numPolys; ++p)
  {
    numTris += m_Polygons[p].m_Vertices.GetCount() - 2;
  }

  return numTris;
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
  bool bFlipWinding = mTransform.GetRotationalPart().GetDeterminant() < 0;

  ezUInt32 idx[4];

  idx[0] = AddVertex(ezVec3(-halfSize.x, -halfSize.y, 0), ezVec3(0, 0, 1), ezVec2(0, 1), color, iCustomIndex, mTransform);
  idx[1] = AddVertex(ezVec3(halfSize.x, -halfSize.y, 0), ezVec3(0, 0, 1), ezVec2(0, 0), color, iCustomIndex, mTransform);
  idx[2] = AddVertex(ezVec3(halfSize.x, halfSize.y, 0), ezVec3(0, 0, 1), ezVec2(1, 0), color, iCustomIndex, mTransform);
  idx[3] = AddVertex(ezVec3(-halfSize.x, halfSize.y, 0), ezVec3(0, 0, 1), ezVec2(1, 1), color, iCustomIndex, mTransform);

  AddPolygon(idx, bFlipWinding);
}

void ezGeometry::AddBox(const ezVec3& size, const ezColor& color, const ezMat4& mTransform, ezInt32 iCustomIndex)
{
  const ezVec3 halfSize = size * 0.5f;
  bool bFlipWinding = mTransform.GetRotationalPart().GetDeterminant() < 0;
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
  AddPolygon(poly, bFlipWinding);

  poly[0] = idx[1];
  poly[1] = idx[5];
  poly[2] = idx[6];
  poly[3] = idx[2];
  AddPolygon(poly, bFlipWinding);

  poly[0] = idx[5];
  poly[1] = idx[4];
  poly[2] = idx[7];
  poly[3] = idx[6];
  AddPolygon(poly, bFlipWinding);

  poly[0] = idx[4];
  poly[1] = idx[0];
  poly[2] = idx[3];
  poly[3] = idx[7];
  AddPolygon(poly, bFlipWinding);

  poly[0] = idx[4];
  poly[1] = idx[5];
  poly[2] = idx[1];
  poly[3] = idx[0];
  AddPolygon(poly, bFlipWinding);

  poly[0] = idx[3];
  poly[1] = idx[2];
  poly[2] = idx[6];
  poly[3] = idx[7];
  AddPolygon(poly, bFlipWinding);
}


void ezGeometry::AddLineBox(const ezVec3& size, const ezColor& color, const ezMat4& mTransform /*= ezMat4::IdentityMatrix()*/,
                            ezInt32 iCustomIndex /*= 0*/)
{
  const ezVec3 halfSize = size * 0.5f;

  AddVertex(ezVec3(-halfSize.x, -halfSize.y, halfSize.z), ezVec3(0, 0, 1), ezVec2(0), color, iCustomIndex, mTransform);
  AddVertex(ezVec3(halfSize.x, -halfSize.y, halfSize.z), ezVec3(0, 0, 1), ezVec2(0), color, iCustomIndex, mTransform);
  AddVertex(ezVec3(halfSize.x, halfSize.y, halfSize.z), ezVec3(0, 0, 1), ezVec2(0), color, iCustomIndex, mTransform);
  AddVertex(ezVec3(-halfSize.x, halfSize.y, halfSize.z), ezVec3(0, 0, 1), ezVec2(0), color, iCustomIndex, mTransform);

  AddVertex(ezVec3(-halfSize.x, -halfSize.y, -halfSize.z), ezVec3(0, 0, -1), ezVec2(0), color, iCustomIndex, mTransform);
  AddVertex(ezVec3(halfSize.x, -halfSize.y, -halfSize.z), ezVec3(0, 0, -1), ezVec2(0), color, iCustomIndex, mTransform);
  AddVertex(ezVec3(halfSize.x, halfSize.y, -halfSize.z), ezVec3(0, 0, -1), ezVec2(0), color, iCustomIndex, mTransform);
  AddVertex(ezVec3(-halfSize.x, halfSize.y, -halfSize.z), ezVec3(0, 0, -1), ezVec2(0), color, iCustomIndex, mTransform);

  AddLine(0, 1);
  AddLine(1, 2);
  AddLine(2, 3);
  AddLine(3, 0);

  AddLine(4, 5);
  AddLine(5, 6);
  AddLine(6, 7);
  AddLine(7, 4);

  AddLine(0, 4);
  AddLine(1, 5);
  AddLine(2, 6);
  AddLine(3, 7);
}


void ezGeometry::AddLineBoxCorners(const ezVec3& size, float fCornerFraction, const ezColor& color,
                                   const ezMat4& mTransform /*= ezMat4::IdentityMatrix()*/, ezInt32 iCustomIndex /*= 0*/)
{
  EZ_ASSERT_DEV(fCornerFraction >= 0.0f && fCornerFraction <= 1.0f, "A fraction value of {0} is invalid", ezArgF(fCornerFraction, 2));

  fCornerFraction *= 0.5f;
  const ezVec3 halfSize = size * 0.5f;

  AddVertex(ezVec3(-halfSize.x, -halfSize.y, halfSize.z), ezVec3(0, 0, 1), ezVec2(0), color, iCustomIndex, mTransform);
  AddVertex(ezVec3(halfSize.x, -halfSize.y, halfSize.z), ezVec3(0, 0, 1), ezVec2(0), color, iCustomIndex, mTransform);
  AddVertex(ezVec3(halfSize.x, halfSize.y, halfSize.z), ezVec3(0, 0, 1), ezVec2(0), color, iCustomIndex, mTransform);
  AddVertex(ezVec3(-halfSize.x, halfSize.y, halfSize.z), ezVec3(0, 0, 1), ezVec2(0), color, iCustomIndex, mTransform);

  AddVertex(ezVec3(-halfSize.x, -halfSize.y, -halfSize.z), ezVec3(0, 0, -1), ezVec2(0), color, iCustomIndex, mTransform);
  AddVertex(ezVec3(halfSize.x, -halfSize.y, -halfSize.z), ezVec3(0, 0, -1), ezVec2(0), color, iCustomIndex, mTransform);
  AddVertex(ezVec3(halfSize.x, halfSize.y, -halfSize.z), ezVec3(0, 0, -1), ezVec2(0), color, iCustomIndex, mTransform);
  AddVertex(ezVec3(-halfSize.x, halfSize.y, -halfSize.z), ezVec3(0, 0, -1), ezVec2(0), color, iCustomIndex, mTransform);

  for (ezUInt32 c = 0; c < 8; ++c)
  {
    const ezVec3& op = m_Vertices[c].m_vPosition;

    const ezVec3 op1 = ezVec3(op.x, op.y, -ezMath::Sign(op.z) * ezMath::Abs(op.z));
    const ezVec3 op2 = ezVec3(op.x, -ezMath::Sign(op.y) * ezMath::Abs(op.y), op.z);
    const ezVec3 op3 = ezVec3(-ezMath::Sign(op.x) * ezMath::Abs(op.x), op.y, op.z);

    const ezUInt32 ix1 = AddVertex(ezMath::Lerp(op, op1, fCornerFraction), m_Vertices[c].m_vPosition, m_Vertices[c].m_vTexCoord, color,
                                   iCustomIndex, mTransform);
    const ezUInt32 ix2 = AddVertex(ezMath::Lerp(op, op2, fCornerFraction), m_Vertices[c].m_vPosition, m_Vertices[c].m_vTexCoord, color,
                                   iCustomIndex, mTransform);
    const ezUInt32 ix3 = AddVertex(ezMath::Lerp(op, op3, fCornerFraction), m_Vertices[c].m_vPosition, m_Vertices[c].m_vTexCoord, color,
                                   iCustomIndex, mTransform);

    AddLine(c, ix1);
    AddLine(c, ix2);
    AddLine(c, ix3);
  }
}

void ezGeometry::AddTexturedBox(const ezVec3& size, const ezColor& color, const ezMat4& mTransform, ezInt32 iCustomIndex)
{
  const ezVec3 halfSize = size * 0.5f;
  bool bFlipWinding = mTransform.GetRotationalPart().GetDeterminant() < 0;
  ezUInt32 idx[4];

  {
    idx[0] = AddVertex(ezVec3(-halfSize.x, -halfSize.y, +halfSize.z), ezVec3(0, 0, 1), ezVec2(0, 1), color, iCustomIndex, mTransform);
    idx[1] = AddVertex(ezVec3(+halfSize.x, -halfSize.y, +halfSize.z), ezVec3(0, 0, 1), ezVec2(0, 0), color, iCustomIndex, mTransform);
    idx[2] = AddVertex(ezVec3(+halfSize.x, +halfSize.y, +halfSize.z), ezVec3(0, 0, 1), ezVec2(1, 0), color, iCustomIndex, mTransform);
    idx[3] = AddVertex(ezVec3(-halfSize.x, +halfSize.y, +halfSize.z), ezVec3(0, 0, 1), ezVec2(1, 1), color, iCustomIndex, mTransform);
    AddPolygon(idx, bFlipWinding);
  }

  {
    idx[0] = AddVertex(ezVec3(-halfSize.x, +halfSize.y, -halfSize.z), ezVec3(0, 0, -1), ezVec2(1, 0), color, iCustomIndex, mTransform);
    idx[1] = AddVertex(ezVec3(+halfSize.x, +halfSize.y, -halfSize.z), ezVec3(0, 0, -1), ezVec2(1, 1), color, iCustomIndex, mTransform);
    idx[2] = AddVertex(ezVec3(+halfSize.x, -halfSize.y, -halfSize.z), ezVec3(0, 0, -1), ezVec2(0, 1), color, iCustomIndex, mTransform);
    idx[3] = AddVertex(ezVec3(-halfSize.x, -halfSize.y, -halfSize.z), ezVec3(0, 0, -1), ezVec2(0, 0), color, iCustomIndex, mTransform);
    AddPolygon(idx, bFlipWinding);
  }

  {
    idx[0] = AddVertex(ezVec3(-halfSize.x, -halfSize.y, -halfSize.z), ezVec3(-1, 0, 0), ezVec2(0, 1), color, iCustomIndex, mTransform);
    idx[1] = AddVertex(ezVec3(-halfSize.x, -halfSize.y, +halfSize.z), ezVec3(-1, 0, 0), ezVec2(0, 0), color, iCustomIndex, mTransform);
    idx[2] = AddVertex(ezVec3(-halfSize.x, +halfSize.y, +halfSize.z), ezVec3(-1, 0, 0), ezVec2(1, 0), color, iCustomIndex, mTransform);
    idx[3] = AddVertex(ezVec3(-halfSize.x, +halfSize.y, -halfSize.z), ezVec3(-1, 0, 0), ezVec2(1, 1), color, iCustomIndex, mTransform);
    AddPolygon(idx, bFlipWinding);
  }

  {
    idx[0] = AddVertex(ezVec3(+halfSize.x, +halfSize.y, -halfSize.z), ezVec3(1, 0, 0), ezVec2(0, 1), color, iCustomIndex, mTransform);
    idx[1] = AddVertex(ezVec3(+halfSize.x, +halfSize.y, +halfSize.z), ezVec3(1, 0, 0), ezVec2(0, 0), color, iCustomIndex, mTransform);
    idx[2] = AddVertex(ezVec3(+halfSize.x, -halfSize.y, +halfSize.z), ezVec3(1, 0, 0), ezVec2(1, 0), color, iCustomIndex, mTransform);
    idx[3] = AddVertex(ezVec3(+halfSize.x, -halfSize.y, -halfSize.z), ezVec3(1, 0, 0), ezVec2(1, 1), color, iCustomIndex, mTransform);
    AddPolygon(idx, bFlipWinding);
  }

  {
    idx[0] = AddVertex(ezVec3(+halfSize.x, -halfSize.y, -halfSize.z), ezVec3(0, -1, 0), ezVec2(0, 1), color, iCustomIndex, mTransform);
    idx[1] = AddVertex(ezVec3(+halfSize.x, -halfSize.y, +halfSize.z), ezVec3(0, -1, 0), ezVec2(0, 0), color, iCustomIndex, mTransform);
    idx[2] = AddVertex(ezVec3(-halfSize.x, -halfSize.y, +halfSize.z), ezVec3(0, -1, 0), ezVec2(1, 0), color, iCustomIndex, mTransform);
    idx[3] = AddVertex(ezVec3(-halfSize.x, -halfSize.y, -halfSize.z), ezVec3(0, -1, 0), ezVec2(1, 1), color, iCustomIndex, mTransform);
    AddPolygon(idx, bFlipWinding);
  }

  {
    idx[0] = AddVertex(ezVec3(-halfSize.x, +halfSize.y, -halfSize.z), ezVec3(0, +1, 0), ezVec2(0, 1), color, iCustomIndex, mTransform);
    idx[1] = AddVertex(ezVec3(-halfSize.x, +halfSize.y, +halfSize.z), ezVec3(0, +1, 0), ezVec2(0, 0), color, iCustomIndex, mTransform);
    idx[2] = AddVertex(ezVec3(+halfSize.x, +halfSize.y, +halfSize.z), ezVec3(0, +1, 0), ezVec2(1, 0), color, iCustomIndex, mTransform);
    idx[3] = AddVertex(ezVec3(+halfSize.x, +halfSize.y, -halfSize.z), ezVec3(0, +1, 0), ezVec2(1, 1), color, iCustomIndex, mTransform);
    AddPolygon(idx, bFlipWinding);
  }
}

void ezGeometry::AddPyramid(const ezVec3& size, bool bCap, const ezColor& color, const ezMat4& mTransform, ezInt32 iCustomIndex)
{
  const ezVec3 halfSize = size * 0.5f;
  bool bFlipWinding = mTransform.GetRotationalPart().GetDeterminant() < 0;
  ezUInt32 quad[4];

  quad[0] = AddVertex(ezVec3(-halfSize.x, halfSize.y, 0), ezVec3(-1, 1, 0).GetNormalized(), ezVec2(0), color, iCustomIndex, mTransform);
  quad[1] = AddVertex(ezVec3(halfSize.x, halfSize.y, 0), ezVec3(1, 1, 0).GetNormalized(), ezVec2(0), color, iCustomIndex, mTransform);
  quad[2] = AddVertex(ezVec3(halfSize.x, -halfSize.y, 0), ezVec3(1, -1, 0).GetNormalized(), ezVec2(0), color, iCustomIndex, mTransform);
  quad[3] = AddVertex(ezVec3(-halfSize.x, -halfSize.y, 0), ezVec3(-1, -1, 0).GetNormalized(), ezVec2(0), color, iCustomIndex, mTransform);

  const ezUInt32 tip = AddVertex(ezVec3(0, 0, size.z), ezVec3(0, 0, 1), ezVec2(0), color, iCustomIndex, mTransform);

  if (bCap)
  {
    AddPolygon(quad, bFlipWinding);
  }

  ezUInt32 tri[3];

  tri[0] = quad[1];
  tri[1] = quad[0];
  tri[2] = tip;
  AddPolygon(tri, bFlipWinding);

  tri[0] = quad[2];
  tri[1] = quad[1];
  tri[2] = tip;
  AddPolygon(tri, bFlipWinding);

  tri[0] = quad[3];
  tri[1] = quad[2];
  tri[2] = tip;
  AddPolygon(tri, bFlipWinding);

  tri[0] = quad[0];
  tri[1] = quad[3];
  tri[2] = tip;
  AddPolygon(tri, bFlipWinding);
}

void ezGeometry::AddGeodesicSphere(float fRadius, ezUInt8 uiSubDivisions, const ezColor& color, const ezMat4& mTransform,
                                   ezInt32 iCustomIndex)
{
  bool bFlipWinding = mTransform.GetRotationalPart().GetDeterminant() < 0;
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
    Edge() {}

    Edge(ezUInt32 id1, ezUInt32 id2)
    {
      m_uiVertex[0] = ezMath::Min(id1, id2);
      m_uiVertex[1] = ezMath::Max(id1, id2);
    }

    bool operator<(const Edge& rhs) const
    {
      if (m_uiVertex[0] < rhs.m_uiVertex[0])
        return true;
      if (m_uiVertex[0] > rhs.m_uiVertex[0])
        return false;
      return m_uiVertex[1] < rhs.m_uiVertex[1];
    }

    bool operator==(const Edge& rhs) const { return m_uiVertex[0] == rhs.m_uiVertex[0] && m_uiVertex[1] == rhs.m_uiVertex[1]; }

    ezUInt32 m_uiVertex[2];
  };

  const ezUInt32 uiFirstVertex = m_Vertices.GetCount();

  ezInt32 iCurrentList = 0;
  ezDeque<Triangle> Tris[2];

  // create icosahedron
  {
    ezMat3 mRotX, mRotZ, mRotZh;
    mRotX.SetRotationMatrixX(ezAngle::Degree(360.0f / 6.0f));
    mRotZ.SetRotationMatrixZ(ezAngle::Degree(-360.0f / 5.0f));
    mRotZh.SetRotationMatrixZ(ezAngle::Degree(-360.0f / 10.0f));

    ezUInt32 vert[12];
    ezVec3 vDir(0, 0, 1);

    vDir.Normalize();
    vert[0] = AddVertex(vDir * fRadius, vDir, ezVec2::ZeroVector(), color, iCustomIndex);

    vDir = mRotX * vDir;

    for (ezInt32 i = 0; i < 5; ++i)
    {
      vDir.Normalize();
      vert[1 + i] = AddVertex(vDir * fRadius, vDir, ezVec2::ZeroVector(), color, iCustomIndex);
      vDir = mRotZ * vDir;
    }

    vDir = mRotX * vDir;
    vDir = mRotZh * vDir;

    for (ezInt32 i = 0; i < 5; ++i)
    {
      vDir.Normalize();
      vert[6 + i] = AddVertex(vDir * fRadius, vDir, ezVec2::ZeroVector(), color, iCustomIndex);
      vDir = mRotZ * vDir;
    }

    vDir.Set(0, 0, -1);
    vDir.Normalize();
    vert[11] = AddVertex(vDir * fRadius, vDir, ezVec2::ZeroVector(), color, iCustomIndex);


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
      ezUInt32 uiVert[3] = {Tris[iPrevList][tri].m_uiIndex[0], Tris[iPrevList][tri].m_uiIndex[1], Tris[iPrevList][tri].m_uiIndex[2]};

      Edge Edges[3] = {Edge(uiVert[0], uiVert[1]), Edge(uiVert[1], uiVert[2]), Edge(uiVert[2], uiVert[0])};

      ezUInt32 uiNewVert[3];

      // split each edge of the triangle in half
      for (ezUInt32 i = 0; i < 3; ++i)
      {
        // do not split an edge that was split before, we want shared vertices everywhere
        if (NewVertices.Find(Edges[i]).IsValid())
          uiNewVert[i] = NewVertices[Edges[i]];
        else
        {
          const ezVec3 vCenter =
              (m_Vertices[Edges[i].m_uiVertex[0]].m_vPosition + m_Vertices[Edges[i].m_uiVertex[1]].m_vPosition).GetNormalized();
          uiNewVert[i] = AddVertex(vCenter * fRadius, vCenter, ezVec2::ZeroVector(), color, iCustomIndex);

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
    AddPolygon(Tris[iCurrentList][tri].m_uiIndex, bFlipWinding);
  }

  // finally apply the user transformation on the new vertices
  TransformVertices(mTransform, uiFirstVertex);
}

void ezGeometry::AddCylinder(float fRadiusTop, float fRadiusBottom, float fHeight, bool bCapTop, bool bCapBottom, ezUInt16 uiSegments,
                             const ezColor& color, const ezMat4& mTransform, ezInt32 iCustomIndex, ezAngle fraction)
{
  EZ_ASSERT_DEV(uiSegments >= 3, "Cannot create a cylinder with only {0} segments", uiSegments);
  EZ_ASSERT_DEV(fraction.GetDegree() >= 0.0f, "A cylinder cannot be built with more less than 0 degree");
  EZ_ASSERT_DEV(fraction.GetDegree() <= 360.0f, "A cylinder cannot be built with more than 360 degree");

  bool bFlipWinding = mTransform.GetRotationalPart().GetDeterminant() < 0;
  const bool bIsFraction = fraction.GetDegree() < 360.0f;
  const ezAngle fDegStep = ezAngle::Degree(fraction.GetDegree() / uiSegments);

  const ezVec3 vTopCenter(0, 0, fHeight * 0.5f);
  const ezVec3 vBottomCenter(0, 0, -fHeight * 0.5f);

  // cylinder wall
  {
    ezHybridArray<ezUInt32, 512> VertsTop;
    ezHybridArray<ezUInt32, 512> VertsBottom;

    for (ezInt32 i = 0; i <= uiSegments; ++i)
    {
      const ezAngle deg = (float)i * fDegStep;

      float fU = 4.0f - deg.GetDegree() / 90.0f;

      const float fX = ezMath::Cos(deg);
      const float fY = ezMath::Sin(deg);

      const ezVec3 vDir(fX, fY, 0);

      VertsTop.PushBack(AddVertex(vTopCenter + vDir * fRadiusTop, vDir, ezVec2(fU, 0), color, iCustomIndex, mTransform));
      VertsBottom.PushBack(AddVertex(vBottomCenter + vDir * fRadiusBottom, vDir, ezVec2(fU, 1), color, iCustomIndex, mTransform));
    }

    for (ezUInt32 i = 1; i <= uiSegments; ++i)
    {
      ezUInt32 quad[4];
      quad[0] = VertsBottom[i - 1];
      quad[1] = VertsBottom[i];
      quad[2] = VertsTop[i];
      quad[3] = VertsTop[i - 1];


      AddPolygon(quad, bFlipWinding);
    }
  }

  // walls for fractional cylinders
  if (bIsFraction)
  {
    const ezVec3 vDir0(1, 0, 0);
    const ezVec3 vDir1(ezMath::Cos(fraction), ezMath::Sin(fraction), 0);

    ezUInt32 quad[4];

    const ezVec3 vNrm0 = -ezVec3(0, 0, 1).CrossRH(vDir0).GetNormalized();
    quad[0] = AddVertex(vTopCenter + vDir0 * fRadiusTop, vNrm0, ezVec2(0, 0), color, iCustomIndex, mTransform);
    quad[1] = AddVertex(vTopCenter, vNrm0, ezVec2(1, 0), color, iCustomIndex, mTransform);
    quad[2] = AddVertex(vBottomCenter, vNrm0, ezVec2(1, 1), color, iCustomIndex, mTransform);
    quad[3] = AddVertex(vBottomCenter + vDir0 * fRadiusBottom, vNrm0, ezVec2(0, 1), color, iCustomIndex, mTransform);


    AddPolygon(quad, bFlipWinding);

    const ezVec3 vNrm1 = ezVec3(0, 0, 1).CrossRH(vDir1).GetNormalized();
    quad[0] = AddVertex(vTopCenter, vNrm1, ezVec2(0, 0), color, iCustomIndex, mTransform);
    quad[1] = AddVertex(vTopCenter + vDir1 * fRadiusTop, vNrm1, ezVec2(1, 0), color, iCustomIndex, mTransform);
    quad[2] = AddVertex(vBottomCenter + vDir1 * fRadiusBottom, vNrm1, ezVec2(1, 1), color, iCustomIndex, mTransform);
    quad[3] = AddVertex(vBottomCenter, vNrm1, ezVec2(0, 1), color, iCustomIndex, mTransform);

    AddPolygon(quad, bFlipWinding);
  }

  if (bCapBottom)
  {
    ezHybridArray<ezUInt32, 512> VertsBottom;

    if (bIsFraction)
    {
      VertsBottom.PushBack(AddVertex(vBottomCenter, ezVec3(0, 0, -1), ezVec2(0), color, iCustomIndex, mTransform));
    }

    for (ezInt32 i = uiSegments - 1; i >= 0; --i)
    {
      const ezAngle deg = (float)i * fDegStep;

      const float fX = ezMath::Cos(deg);
      const float fY = ezMath::Sin(deg);

      const ezVec3 vDir(fX, fY, 0);

      VertsBottom.PushBack(
          AddVertex(vBottomCenter + vDir * fRadiusBottom, ezVec3(0, 0, -1), ezVec2(fY, fX), color, iCustomIndex, mTransform));
    }

    AddPolygon(VertsBottom, bFlipWinding);
  }

  if (bCapTop)
  {
    ezHybridArray<ezUInt32, 512> VertsTop;

    if (bIsFraction)
    {
      VertsTop.PushBack(AddVertex(vTopCenter, ezVec3(0, 0, 1), ezVec2(0), color, iCustomIndex, mTransform));
    }

    for (ezInt32 i = 0; i < uiSegments; ++i)
    {
      const ezAngle deg = (float)i * fDegStep;

      const float fX = ezMath::Cos(deg);
      const float fY = ezMath::Sin(deg);

      const ezVec3 vDir(fX, fY, 0);

      VertsTop.PushBack(AddVertex(vTopCenter + vDir * fRadiusTop, ezVec3(0, 0, 1), ezVec2(fY, -fX), color, iCustomIndex, mTransform));
    }

    AddPolygon(VertsTop, bFlipWinding);
  }
}

void ezGeometry::AddCylinderOnePiece(float fRadiusTop, float fRadiusBottom, float fHeight, ezUInt16 uiSegments, const ezColor& color,
                                     const ezMat4& mTransform /*= ezMat4::IdentityMatrix()*/, ezInt32 iCustomIndex /*= 0*/)
{
  EZ_ASSERT_DEV(uiSegments >= 3, "Cannot create a cylinder with only {0} segments", uiSegments);

  bool bFlipWinding = mTransform.GetRotationalPart().GetDeterminant() < 0;
  const ezAngle fDegStep = ezAngle::Degree(360.0f / uiSegments);

  const ezVec3 vTopCenter(0, 0, fHeight * 0.5f);
  const ezVec3 vBottomCenter(0, 0, -fHeight * 0.5f);

  // cylinder wall
  {
    ezHybridArray<ezUInt32, 512> VertsTop;
    ezHybridArray<ezUInt32, 512> VertsBottom;

    for (ezInt32 i = 0; i < uiSegments; ++i)
    {
      const ezAngle deg = (float)i * fDegStep;

      float fU = 4.0f - deg.GetDegree() / 90.0f;

      const float fX = ezMath::Cos(deg);
      const float fY = ezMath::Sin(deg);

      const ezVec3 vDir(fX, fY, 0);

      VertsTop.PushBack(AddVertex(vTopCenter + vDir * fRadiusTop, vDir, ezVec2(fU, 0), color, iCustomIndex, mTransform));
      VertsBottom.PushBack(AddVertex(vBottomCenter + vDir * fRadiusBottom, vDir, ezVec2(fU, 1), color, iCustomIndex, mTransform));
    }

    for (ezUInt32 i = 1; i <= uiSegments; ++i)
    {
      ezUInt32 quad[4];
      quad[0] = VertsBottom[i - 1];
      quad[1] = VertsBottom[i % uiSegments];
      quad[2] = VertsTop[i % uiSegments];
      quad[3] = VertsTop[i - 1];

      AddPolygon(quad, bFlipWinding);
    }

    AddPolygon(VertsTop, bFlipWinding);
    AddPolygon(VertsBottom, !bFlipWinding);
  }
}

void ezGeometry::AddCone(float fRadius, float fHeight, bool bCap, ezUInt16 uiSegments, const ezColor& color, const ezMat4& mTransform,
                         ezInt32 iCustomIndex)
{
  EZ_ASSERT_DEV(uiSegments >= 3, "Cannot create a cone with only {0} segments", uiSegments);

  bool bFlipWinding = mTransform.GetRotationalPart().GetDeterminant() < 0;

  ezHybridArray<ezUInt32, 512> VertsBottom;

  const ezAngle fDegStep = ezAngle::Degree(360.0f / uiSegments);

  const ezUInt32 uiTip = AddVertex(ezVec3(0, 0, fHeight), ezVec3(0, 0, 1), ezVec2(0), color, iCustomIndex, mTransform);

  for (ezInt32 i = uiSegments - 1; i >= 0; --i)
  {
    const ezAngle deg = (float)i * fDegStep;

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

    AddPolygon(tri, bFlipWinding);
  }

  if (bCap)
  {
    AddPolygon(VertsBottom, bFlipWinding);
  }
}

void ezGeometry::AddSphere(float fRadius, ezUInt16 uiSegments, ezUInt16 uiStacks, const ezColor& color, const ezMat4& mTransform,
                           ezInt32 iCustomIndex)
{
  EZ_ASSERT_DEV(uiSegments >= 3, "Sphere must have at least 3 segments");
  EZ_ASSERT_DEV(uiStacks >= 2, "Sphere must have at least 2 stacks");

  bool bFlipWinding = mTransform.GetRotationalPart().GetDeterminant() < 0;
  const ezAngle fDegreeDiffSegments = ezAngle::Degree(360.0f / (float)(uiSegments));
  const ezAngle fDegreeDiffStacks = ezAngle::Degree(180.0f / (float)(uiStacks));

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
      vPos.y = -ezMath::Sin(fDegree) * fRadius * fCosDS;
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

    AddPolygon(tri, bFlipWinding);
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

      AddPolygon(quad, bFlipWinding);
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

    AddPolygon(tri, bFlipWinding);
  }
}

void ezGeometry::AddHalfSphere(float fRadius, ezUInt16 uiSegments, ezUInt16 uiStacks, bool bCap, const ezColor& color,
                               const ezMat4& mTransform, ezInt32 iCustomIndex)
{
  EZ_ASSERT_DEV(uiSegments >= 3, "Sphere must have at least 3 segments");
  EZ_ASSERT_DEV(uiStacks >= 1, "Sphere must have at least 1 stacks");

  bool bFlipWinding = mTransform.GetRotationalPart().GetDeterminant() < 0;
  const ezAngle fDegreeDiffSegments = ezAngle::Degree(360.0f / (float)(uiSegments));
  const ezAngle fDegreeDiffStacks = ezAngle::Degree(90.0f / (float)(uiStacks));

  const ezUInt32 uiFirstVertex = m_Vertices.GetCount();

  // first create all the vertex positions
  for (ezUInt32 st = 0; st < uiStacks; ++st)
  {
    const ezAngle fDegreeStack = ezAngle::Degree(-90.0f + ((st + 1) * fDegreeDiffStacks.GetDegree()));
    const float fCosDS = ezMath::Cos(fDegreeStack);
    const float fSinDS = ezMath::Sin(fDegreeStack);
    const float fY = -fSinDS * fRadius;

    const float fV = (float)(st + 1) / (float)uiStacks;

    for (ezUInt32 sp = 0; sp <= uiSegments; ++sp)
    {
      float fU = ((float)sp / (float)(uiSegments)) * 2.0f;

      if (fU > 1.0f)
        fU = 2.0f - fU;

      // the vertices for the bottom disk
      const ezAngle fDegree = (float)sp * fDegreeDiffSegments;

      ezVec3 vPos;
      vPos.x = ezMath::Cos(fDegree) * fRadius * fCosDS;
      vPos.y = ezMath::Sin(fDegree) * fRadius * fCosDS;
      vPos.z = fY;

      AddVertex(vPos, vPos.GetNormalized(), ezVec2(fU, fV), color, iCustomIndex, mTransform);
    }
  }

  ezUInt32 uiTopVertex = AddVertex(ezVec3(0, 0, fRadius), ezVec3(0, 0, 1), ezVec2(0.0f), color, iCustomIndex, mTransform);

  ezUInt32 tri[3];
  ezUInt32 quad[4];

  // now create the top cone
  for (ezUInt32 p = 0; p < uiSegments; ++p)
  {
    tri[0] = uiTopVertex;
    tri[1] = uiFirstVertex + p;
    tri[2] = uiFirstVertex + ((p + 1) % (uiSegments + 1));

    AddPolygon(tri, bFlipWinding);
  }

  // now create the stacks in the middle

  for (ezUInt16 st = 0; st < uiStacks - 1; ++st)
  {
    const ezUInt32 uiRowBottom = (uiSegments + 1) * st;
    const ezUInt32 uiRowTop = (uiSegments + 1) * (st + 1);

    for (ezInt32 i = 0; i < uiSegments; ++i)
    {
      quad[0] = uiFirstVertex + (uiRowTop + ((i + 1) % (uiSegments + 1)));
      quad[1] = uiFirstVertex + (uiRowBottom + ((i + 1) % (uiSegments + 1)));
      quad[2] = uiFirstVertex + (uiRowBottom + i);
      quad[3] = uiFirstVertex + (uiRowTop + i);

      AddPolygon(quad, bFlipWinding);
    }
  }

  if (bCap)
  {
    ezHybridArray<ezUInt32, 256> uiCap;

    for (ezUInt32 i = uiTopVertex - 1; i >= uiTopVertex - uiSegments; --i)
      uiCap.PushBack(i);

    AddPolygon(uiCap, bFlipWinding);
  }
}

void ezGeometry::AddCapsule(float fRadius, float fHeight, ezUInt16 uiSegments, ezUInt16 uiStacks, const ezColor& color,
                            const ezMat4& mTransform, ezInt32 iCustomIndex)
{
  EZ_ASSERT_DEV(uiSegments >= 3, "Capsule must have at least 3 segments");
  EZ_ASSERT_DEV(uiStacks >= 1, "Capsule must have at least 1 stacks");
  EZ_ASSERT_DEV(fHeight >= 0.0f, "Height must be positive");

  bool bFlipWinding = mTransform.GetRotationalPart().GetDeterminant() < 0;
  const ezAngle fDegreeDiffStacks = ezAngle::Degree(90.0f / (float)(uiStacks));

  const ezUInt32 uiFirstVertex = m_Vertices.GetCount();

  // first create all the vertex positions
  const float fDegreeStepSlices = 360.0f / (float)(uiSegments);

  float fOffset = fHeight * 0.5f;

  // for (ezUInt32 h = 0; h < 2; ++h)
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
        vPos.z = fY + fOffset;
        vPos.y = ezMath::Sin(fDegree) * fRadius * fCosDS;

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
        vPos.z = fY + fOffset;
        vPos.y = ezMath::Sin(fDegree) * fRadius * fCosDS;

        AddVertex(vPos, vPos.GetNormalized(), ezVec2(0), color, iCustomIndex, mTransform);
      }
    }
  }

  ezUInt32 uiTopVertex = AddVertex(ezVec3(0, 0, fRadius + fHeight * 0.5f), ezVec3(0, 0, 1), ezVec2(0), color, iCustomIndex, mTransform);
  ezUInt32 uiBottomVertex =
      AddVertex(ezVec3(0, 0, -fRadius - fHeight * 0.5f), ezVec3(0, 0, -1), ezVec2(0), color, iCustomIndex, mTransform);

  ezUInt32 tri[3];
  ezUInt32 quad[4];

  // now create the top cone
  for (ezUInt32 p = 0; p < uiSegments; ++p)
  {
    tri[0] = uiTopVertex;
    tri[2] = uiFirstVertex + ((p + 1) % uiSegments);
    tri[1] = uiFirstVertex + p;

    AddPolygon(tri, bFlipWinding);
  }

  // now create the stacks in the middle

  for (ezUInt16 st = 0; st < uiStacks * 2 - 1; ++st)
  {
    const ezUInt32 uiRowBottom = uiSegments * st;
    const ezUInt32 uiRowTop = uiSegments * (st + 1);

    for (ezInt32 i = 0; i < uiSegments; ++i)
    {
      quad[0] = uiFirstVertex + (uiRowTop + ((i + 1) % uiSegments));
      quad[3] = uiFirstVertex + (uiRowTop + i);
      quad[2] = uiFirstVertex + (uiRowBottom + i);
      quad[1] = uiFirstVertex + (uiRowBottom + ((i + 1) % uiSegments));

      AddPolygon(quad, bFlipWinding);
    }
  }

  const ezInt32 iBottomStack = uiSegments * (uiStacks * 2 - 1);

  // now create the bottom cone
  for (ezUInt32 p = 0; p < uiSegments; ++p)
  {
    tri[0] = uiBottomVertex;
    tri[2] = uiFirstVertex + (iBottomStack + p);
    tri[1] = uiFirstVertex + (iBottomStack + ((p + 1) % uiSegments));

    AddPolygon(tri, bFlipWinding);
  }
}

void ezGeometry::AddTorus(float fInnerRadius, float fOuterRadius, ezUInt16 uiSegments, ezUInt16 uiSegmentDetail, const ezColor& color,
                          const ezMat4& mTransform, ezInt32 iCustomIndex)
{
  EZ_ASSERT_DEV(fInnerRadius < fOuterRadius, "Inner radius must be smaller than outer radius. Doh!");
  EZ_ASSERT_DEV(uiSegments >= 3, "Invalid number of segments.");
  EZ_ASSERT_DEV(uiSegmentDetail >= 3, "Invalid segment detail value.");

  bool bFlipWinding = mTransform.GetRotationalPart().GetDeterminant() < 0;
  const float fCylinderRadius = (fOuterRadius - fInnerRadius) * 0.5f;
  const float fLoopRadius = fInnerRadius + fCylinderRadius;

  const ezAngle fAngleStepSegment = ezAngle::Degree(360.0f / uiSegments);
  const ezAngle fAngleStepCylinder = ezAngle::Degree(360.0f / uiSegmentDetail);

  const ezUInt32 uiFirstVertex = m_Vertices.GetCount();

  // this is the loop for the torus ring
  for (ezUInt16 seg = 0; seg < uiSegments; ++seg)
  {
    const ezAngle fAngle = seg * fAngleStepSegment;

    const float fSinAngle = ezMath::Sin(fAngle);
    const float fCosAngle = ezMath::Cos(fAngle);

    const ezVec3 vLoopPos = ezVec3(fSinAngle, fCosAngle, 0) * fLoopRadius;

    // this is the loop to go round the cylinder
    for (ezUInt16 p = 0; p < uiSegmentDetail; ++p)
    {
      const ezAngle fCylinderAngle = p * fAngleStepCylinder;

      const ezVec3 vDir(ezMath::Cos(fCylinderAngle) * fSinAngle, ezMath::Cos(fCylinderAngle) * fCosAngle, ezMath::Sin(fCylinderAngle));

      const ezVec3 vPos = vLoopPos + fCylinderRadius * vDir;

      AddVertex(vPos, vDir, ezVec2(0), color, iCustomIndex, mTransform);
    }
  }


  for (ezUInt16 seg = 0; seg < uiSegments; ++seg)
  {
    const ezUInt16 rs0 = uiFirstVertex + seg * uiSegmentDetail;
    const ezUInt16 rs1 = uiFirstVertex + ((seg + 1) % uiSegments) * uiSegmentDetail;

    for (ezUInt16 p = 0; p < uiSegmentDetail; ++p)
    {
      const ezUInt16 p1 = (p + 1) % uiSegmentDetail;

      ezUInt32 quad[4];

      quad[0] = rs1 + p;
      quad[3] = rs1 + p1;
      quad[2] = rs0 + p1;
      quad[1] = rs0 + p;

      AddPolygon(quad, bFlipWinding);
    }
  }
}

void ezGeometry::AddTexturedRamp(const ezVec3& size, const ezColor& color, const ezMat4& mTransform /*= ezMat4::IdentityMatrix()*/,
                                 ezInt32 iCustomIndex /*= 0*/)
{
  const ezVec3 halfSize = size * 0.5f;
  bool bFlipWinding = mTransform.GetRotationalPart().GetDeterminant() < 0;
  ezUInt32 idx[4];
  ezUInt32 idx3[3];

  {
    idx[0] = AddVertex(ezVec3(-halfSize.x, -halfSize.y, -halfSize.z), ezVec3(0, 0, 1), ezVec2(0, 1), color, iCustomIndex, mTransform);
    idx[1] = AddVertex(ezVec3(+halfSize.x, -halfSize.y, +halfSize.z), ezVec3(0, 0, 1), ezVec2(0, 0), color, iCustomIndex, mTransform);
    idx[2] = AddVertex(ezVec3(+halfSize.x, +halfSize.y, +halfSize.z), ezVec3(0, 0, 1), ezVec2(1, 0), color, iCustomIndex, mTransform);
    idx[3] = AddVertex(ezVec3(-halfSize.x, +halfSize.y, -halfSize.z), ezVec3(0, 0, 1), ezVec2(1, 1), color, iCustomIndex, mTransform);
    AddPolygon(idx, bFlipWinding);
  }

  {
    idx[0] = AddVertex(ezVec3(-halfSize.x, +halfSize.y, -halfSize.z), ezVec3(0, 0, -1), ezVec2(1, 0), color, iCustomIndex, mTransform);
    idx[1] = AddVertex(ezVec3(+halfSize.x, +halfSize.y, -halfSize.z), ezVec3(0, 0, -1), ezVec2(1, 1), color, iCustomIndex, mTransform);
    idx[2] = AddVertex(ezVec3(+halfSize.x, -halfSize.y, -halfSize.z), ezVec3(0, 0, -1), ezVec2(0, 1), color, iCustomIndex, mTransform);
    idx[3] = AddVertex(ezVec3(-halfSize.x, -halfSize.y, -halfSize.z), ezVec3(0, 0, -1), ezVec2(0, 0), color, iCustomIndex, mTransform);
    AddPolygon(idx, bFlipWinding);
  }

  {
    idx[0] = AddVertex(ezVec3(+halfSize.x, +halfSize.y, -halfSize.z), ezVec3(1, 0, 0), ezVec2(0, 1), color, iCustomIndex, mTransform);
    idx[1] = AddVertex(ezVec3(+halfSize.x, +halfSize.y, +halfSize.z), ezVec3(1, 0, 0), ezVec2(0, 0), color, iCustomIndex, mTransform);
    idx[2] = AddVertex(ezVec3(+halfSize.x, -halfSize.y, +halfSize.z), ezVec3(1, 0, 0), ezVec2(1, 0), color, iCustomIndex, mTransform);
    idx[3] = AddVertex(ezVec3(+halfSize.x, -halfSize.y, -halfSize.z), ezVec3(1, 0, 0), ezVec2(1, 1), color, iCustomIndex, mTransform);
    AddPolygon(idx, bFlipWinding);
  }

  {
    idx3[0] = AddVertex(ezVec3(+halfSize.x, -halfSize.y, -halfSize.z), ezVec3(0, -1, 0), ezVec2(0, 1), color, iCustomIndex, mTransform);
    idx3[1] = AddVertex(ezVec3(+halfSize.x, -halfSize.y, +halfSize.z), ezVec3(0, -1, 0), ezVec2(0, 0), color, iCustomIndex, mTransform);
    idx3[2] = AddVertex(ezVec3(-halfSize.x, -halfSize.y, -halfSize.z), ezVec3(0, -1, 0), ezVec2(1, 1), color, iCustomIndex, mTransform);
    AddPolygon(idx3, bFlipWinding);
  }

  {
    idx3[0] = AddVertex(ezVec3(-halfSize.x, +halfSize.y, -halfSize.z), ezVec3(0, +1, 0), ezVec2(0, 1), color, iCustomIndex, mTransform);
    idx3[1] = AddVertex(ezVec3(+halfSize.x, +halfSize.y, +halfSize.z), ezVec3(0, +1, 0), ezVec2(1, 0), color, iCustomIndex, mTransform);
    idx3[2] = AddVertex(ezVec3(+halfSize.x, +halfSize.y, -halfSize.z), ezVec3(0, +1, 0), ezVec2(1, 1), color, iCustomIndex, mTransform);
    AddPolygon(idx3, bFlipWinding);
  }
}

void ezGeometry::AddStairs(const ezVec3& size, ezUInt32 uiNumSteps, ezAngle curvature, bool bSmoothSloped, const ezColor& color,
                           const ezMat4& mTransform /*= ezMat4::IdentityMatrix()*/, ezInt32 iCustomIndex /*= 0*/)
{
  const bool bFlipWinding = false; // TODO

  curvature = ezMath::Clamp(curvature, -ezAngle::Degree(360), ezAngle::Degree(360));
  const ezAngle curveStep = curvature / (float)uiNumSteps;

  const float fStepDiv = 1.0f / uiNumSteps;
  const float fStepDepth = size.x / uiNumSteps;
  const float fStepHeight = size.z / uiNumSteps;

  ezVec3 vMoveFwd(fStepDepth, 0, 0);
  const ezVec3 vMoveUp(0, 0, fStepHeight);
  ezVec3 vMoveUpFwd(fStepDepth, 0, fStepHeight);

  ezVec3 vBaseL0(-size.x * 0.5f, -size.y * 0.5f, -size.z * 0.5f);
  ezVec3 vBaseL1(-size.x * 0.5f, +size.y * 0.5f, -size.z * 0.5f);
  ezVec3 vBaseR0 = vBaseL0 + vMoveFwd;
  ezVec3 vBaseR1 = vBaseL1 + vMoveFwd;

  ezVec3 vTopL0 = vBaseL0 + vMoveUp;
  ezVec3 vTopL1 = vBaseL1 + vMoveUp;
  ezVec3 vTopR0 = vBaseR0 + vMoveUp;
  ezVec3 vTopR1 = vBaseR1 + vMoveUp;

  ezVec3 vPrevTopR0 = vBaseL0;
  ezVec3 vPrevTopR1 = vBaseL1;

  float fTexU0 = 0;
  float fTexU1 = fStepDiv;

  ezVec3 vSideNormal0(0, 1, 0);
  ezVec3 vSideNormal1(0, 1, 0);
  ezVec3 vStepFrontNormal(-1, 0, 0);

  ezQuat qRot;
  qRot.SetFromAxisAndAngle(ezVec3(0, 0, 1), curveStep);

  for (ezUInt32 step = 0; step < uiNumSteps; ++step)
  {
    {
      const ezVec3 vAvg = (vTopL0 + vTopL1 + vTopR0 + vTopR1) / 4.0f;

      vTopR0 = vAvg + qRot * (vTopR0 - vAvg);
      vTopR1 = vAvg + qRot * (vTopR1 - vAvg);
      vBaseR0 = vAvg + qRot * (vBaseR0 - vAvg);
      vBaseR1 = vAvg + qRot * (vBaseR1 - vAvg);

      vMoveFwd = qRot * vMoveFwd;
      vMoveUpFwd = vMoveFwd;
      vMoveUpFwd.z = fStepHeight;

      vSideNormal1 = qRot * vSideNormal1;
    }

    if (bSmoothSloped)
    {
      // don't care about exact normals for the top surfaces
      vTopL0 = vPrevTopR0;
      vTopL1 = vPrevTopR1;
    }

    ezUInt32 poly[4];

    // top
    poly[0] = AddVertex(vTopL0, ezVec3(0, 0, 1), ezVec2(fTexU0, 0), color, iCustomIndex, mTransform);
    poly[3] = AddVertex(vTopL1, ezVec3(0, 0, 1), ezVec2(fTexU0, 1), color, iCustomIndex, mTransform);
    poly[1] = AddVertex(vTopR0, ezVec3(0, 0, 1), ezVec2(fTexU1, 0), color, iCustomIndex, mTransform);
    poly[2] = AddVertex(vTopR1, ezVec3(0, 0, 1), ezVec2(fTexU1, 1), color, iCustomIndex, mTransform);
    AddPolygon(poly, bFlipWinding);

    // bottom
    poly[0] = AddVertex(vBaseL0, ezVec3(0, 0, -1), ezVec2(fTexU0, 0), color, iCustomIndex, mTransform);
    poly[1] = AddVertex(vBaseL1, ezVec3(0, 0, -1), ezVec2(fTexU0, 1), color, iCustomIndex, mTransform);
    poly[3] = AddVertex(vBaseR0, ezVec3(0, 0, -1), ezVec2(fTexU1, 0), color, iCustomIndex, mTransform);
    poly[2] = AddVertex(vBaseR1, ezVec3(0, 0, -1), ezVec2(fTexU1, 1), color, iCustomIndex, mTransform);
    AddPolygon(poly, bFlipWinding);

    // step front
    if (!bSmoothSloped)
    {
      poly[0] = AddVertex(vPrevTopR0, ezVec3(-1, 0, 0), ezVec2(0, fTexU0), color, iCustomIndex, mTransform);
      poly[3] = AddVertex(vPrevTopR1, ezVec3(-1, 0, 0), ezVec2(1, fTexU0), color, iCustomIndex, mTransform);
      poly[1] = AddVertex(vTopL0, ezVec3(-1, 0, 0), ezVec2(0, fTexU1), color, iCustomIndex, mTransform);
      poly[2] = AddVertex(vTopL1, ezVec3(-1, 0, 0), ezVec2(1, fTexU1), color, iCustomIndex, mTransform);
      AddPolygon(poly, bFlipWinding);
    }

    // side 1
    poly[0] = AddVertex(vBaseL0, -vSideNormal0, ezVec2(fTexU0, 0), color, iCustomIndex, mTransform);
    poly[1] = AddVertex(vBaseR0, -vSideNormal1, ezVec2(fTexU1, 0), color, iCustomIndex, mTransform);
    poly[3] = AddVertex(vTopL0, -vSideNormal0, ezVec2(fTexU0, fTexU1), color, iCustomIndex, mTransform);
    poly[2] = AddVertex(vTopR0, -vSideNormal1, ezVec2(fTexU1, fTexU1), color, iCustomIndex, mTransform);
    AddPolygon(poly, bFlipWinding);

    // side 2
    poly[0] = AddVertex(vBaseL1, vSideNormal0, ezVec2(fTexU0, 0), color, iCustomIndex, mTransform);
    poly[3] = AddVertex(vBaseR1, vSideNormal1, ezVec2(fTexU1, 0), color, iCustomIndex, mTransform);
    poly[1] = AddVertex(vTopL1, vSideNormal0, ezVec2(fTexU0, fTexU1), color, iCustomIndex, mTransform);
    poly[2] = AddVertex(vTopR1, vSideNormal1, ezVec2(fTexU1, fTexU1), color, iCustomIndex, mTransform);
    AddPolygon(poly, bFlipWinding);

    vPrevTopR0 = vTopR0;
    vPrevTopR1 = vTopR1;

    vBaseL0 = vBaseR0;
    vBaseL1 = vBaseR1;
    vBaseR0 += vMoveFwd;
    vBaseR1 += vMoveFwd;

    vTopL0 = vTopR0 + vMoveUp;
    vTopL1 = vTopR1 + vMoveUp;
    vTopR0 += vMoveUpFwd;
    vTopR1 += vMoveUpFwd;

    fTexU0 = fTexU1;
    fTexU1 += fStepDiv;

    vSideNormal0 = vSideNormal1;
    vStepFrontNormal = qRot * vStepFrontNormal;
  }

  // back
  {
    ezUInt32 poly[4];
    poly[0] = AddVertex(vBaseL0, -vStepFrontNormal, ezVec2(0, 0), color, iCustomIndex, mTransform);
    poly[1] = AddVertex(vBaseL1, -vStepFrontNormal, ezVec2(1, 0), color, iCustomIndex, mTransform);
    poly[3] = AddVertex(vPrevTopR0, -vStepFrontNormal, ezVec2(0, 1), color, iCustomIndex, mTransform);
    poly[2] = AddVertex(vPrevTopR1, -vStepFrontNormal, ezVec2(1, 1), color, iCustomIndex, mTransform);
    AddPolygon(poly, bFlipWinding);
  }
}


void ezGeometry::AddArch(const ezVec3& size, ezUInt32 uiNumSegments, float fThickness, ezAngle angle, bool bMakeSteps, bool bSmoothBottom,
                         bool bSmoothTop, const ezColor& color, const ezMat4& mTransform /*= ezMat4::IdentityMatrix()*/,
                         ezInt32 iCustomIndex /*= 0*/)
{
  // sanitize input values
  {
    if (angle.GetRadian() == 0.0f)
      angle = ezAngle::Degree(360);

    angle = ezMath::Clamp(angle, ezAngle::Degree(-360.0f), ezAngle::Degree(360.0f));

    fThickness = ezMath::Clamp(fThickness, 0.01f, ezMath::Min(size.x, size.y) * 0.45f);

    bSmoothBottom = bMakeSteps && bSmoothBottom;
    bSmoothTop = bMakeSteps && bSmoothTop;
  }

  bool bFlipWinding = mTransform.GetRotationalPart().GetDeterminant() < 0;
  if (angle.GetRadian() < 0)
    bFlipWinding = !bFlipWinding;

  const ezAngle angleStep = angle / (float)uiNumSegments;
  const float fScaleX = size.x * 0.5f;
  const float fScaleY = size.y * 0.5f;
  const float fHalfHeight = size.z * 0.5f;
  const float fStepHeight = size.z / (float)uiNumSegments;

  float fBottomZ = -fHalfHeight;
  float fTopZ = +fHalfHeight;

  if (bMakeSteps)
  {
    fTopZ = fBottomZ + fStepHeight;
  }

  // mutable variables
  ezAngle nextAngle;
  ezVec3 vCurDirOutwards, vNextDirOutwards;
  ezVec3 vCurBottomOuter, vCurBottomInner, vCurTopOuter, vCurTopInner;
  ezVec3 vNextBottomOuter, vNextBottomInner, vNextTopOuter, vNextTopInner;

  // Setup first round
  {
    vNextDirOutwards.Set(ezMath::Cos(nextAngle), ezMath::Sin(nextAngle), 0);
    vNextBottomOuter.Set(ezMath::Cos(nextAngle) * fScaleX, ezMath::Sin(nextAngle) * fScaleY, fBottomZ);
    vNextTopOuter.Set(vNextBottomOuter.x, vNextBottomOuter.y, fTopZ);

    const ezVec3 vNextThickness = vNextDirOutwards * fThickness;
    vNextBottomInner = vNextBottomOuter - vNextThickness;
    vNextTopInner = vNextTopOuter - vNextThickness;

    if (bSmoothBottom)
    {
      vNextBottomInner.z += fStepHeight * 0.5f;
      vNextBottomOuter.z += fStepHeight * 0.5f;
    }

    if (bSmoothTop)
    {
      vNextTopInner.z += fStepHeight * 0.5f;
      vNextTopOuter.z += fStepHeight * 0.5f;
    }
  }

  const float fOuterUstep = 3.0f / uiNumSegments;
  for (ezUInt32 segment = 0; segment < uiNumSegments; ++segment)
  {
    // step values
    {
      nextAngle = angleStep * (segment + 1.0f);

      vCurDirOutwards = vNextDirOutwards;

      vCurBottomOuter = vNextBottomOuter;
      vCurBottomInner = vNextBottomInner;
      vCurTopOuter = vNextTopOuter;
      vCurTopInner = vNextTopInner;

      vNextDirOutwards.Set(ezMath::Cos(nextAngle), ezMath::Sin(nextAngle), 0);

      vNextBottomOuter.Set(vNextDirOutwards.x * fScaleX, vNextDirOutwards.y * fScaleY, fBottomZ);
      vNextTopOuter.Set(vNextBottomOuter.x, vNextBottomOuter.y, fTopZ);

      const ezVec3 vNextThickness = vNextDirOutwards * fThickness;
      vNextBottomInner = vNextBottomOuter - vNextThickness;
      vNextTopInner = vNextTopOuter - vNextThickness;

      if (bSmoothBottom)
      {
        vCurBottomInner.z -= fStepHeight;
        vCurBottomOuter.z -= fStepHeight;

        vNextBottomInner.z += fStepHeight * 0.5f;
        vNextBottomOuter.z += fStepHeight * 0.5f;
      }

      if (bSmoothTop)
      {
        vCurTopInner.z -= fStepHeight;
        vCurTopOuter.z -= fStepHeight;

        vNextTopInner.z += fStepHeight * 0.5f;
        vNextTopOuter.z += fStepHeight * 0.5f;
      }
    }

    const float fCurOuterU = segment * fOuterUstep;
    const float fNextOuterU = (1 + segment) * fOuterUstep;

    ezUInt32 poly[4];

    // Outside
    {
      poly[0] = AddVertex(vCurBottomOuter, vCurDirOutwards, ezVec2(fCurOuterU, 0), color, iCustomIndex, mTransform);
      poly[1] = AddVertex(vNextBottomOuter, vNextDirOutwards, ezVec2(fNextOuterU, 0), color, iCustomIndex, mTransform);
      poly[3] = AddVertex(vCurTopOuter, vCurDirOutwards, ezVec2(fCurOuterU, 1), color, iCustomIndex, mTransform);
      poly[2] = AddVertex(vNextTopOuter, vNextDirOutwards, ezVec2(fNextOuterU, 1), color, iCustomIndex, mTransform);
      AddPolygon(poly, bFlipWinding);
    }

    // Inside
    {
      poly[0] = AddVertex(vCurBottomInner, -vCurDirOutwards, ezVec2(fCurOuterU, 0), color, iCustomIndex, mTransform);
      poly[3] = AddVertex(vNextBottomInner, -vNextDirOutwards, ezVec2(fNextOuterU, 0), color, iCustomIndex, mTransform);
      poly[1] = AddVertex(vCurTopInner, -vCurDirOutwards, ezVec2(fCurOuterU, 1), color, iCustomIndex, mTransform);
      poly[2] = AddVertex(vNextTopInner, -vNextDirOutwards, ezVec2(fNextOuterU, 1), color, iCustomIndex, mTransform);
      AddPolygon(poly, bFlipWinding);
    }

    // Bottom
    {
      poly[0] = AddVertex(vCurBottomInner, ezVec3(0, 0, -1), vCurBottomInner.GetAsVec2(), color, iCustomIndex, mTransform);
      poly[1] = AddVertex(vNextBottomInner, ezVec3(0, 0, -1), vNextBottomInner.GetAsVec2(), color, iCustomIndex, mTransform);
      poly[3] = AddVertex(vCurBottomOuter, ezVec3(0, 0, -1), vCurBottomOuter.GetAsVec2(), color, iCustomIndex, mTransform);
      poly[2] = AddVertex(vNextBottomOuter, ezVec3(0, 0, -1), vNextBottomOuter.GetAsVec2(), color, iCustomIndex, mTransform);
      AddPolygon(poly, bFlipWinding);
    }

    // Top
    {
      poly[0] = AddVertex(vCurTopInner, ezVec3(0, 0, 1), vCurTopInner.GetAsVec2(), color, iCustomIndex, mTransform);
      poly[3] = AddVertex(vNextTopInner, ezVec3(0, 0, 1), vNextTopInner.GetAsVec2(), color, iCustomIndex, mTransform);
      poly[1] = AddVertex(vCurTopOuter, ezVec3(0, 0, 1), vCurTopOuter.GetAsVec2(), color, iCustomIndex, mTransform);
      poly[2] = AddVertex(vNextTopOuter, ezVec3(0, 0, 1), vNextTopOuter.GetAsVec2(), color, iCustomIndex, mTransform);
      AddPolygon(poly, bFlipWinding);
    }

    // Front
    {
      const ezVec3 vNormal = (bFlipWinding ? -1.0f : 1.0f) * vCurDirOutwards.CrossRH(ezVec3(0, 0, 1));
      poly[0] = AddVertex(vCurBottomInner, vNormal, ezVec2(0, 0), color, iCustomIndex, mTransform);
      poly[1] = AddVertex(vCurBottomOuter, vNormal, ezVec2(1, 0), color, iCustomIndex, mTransform);
      poly[3] = AddVertex(vCurTopInner, vNormal, ezVec2(0, 1), color, iCustomIndex, mTransform);
      poly[2] = AddVertex(vCurTopOuter, vNormal, ezVec2(1, 1), color, iCustomIndex, mTransform);
      AddPolygon(poly, bFlipWinding);
    }

    // Back
    {
      const ezVec3 vNormal = (bFlipWinding ? -1.0f : 1.0f) * -vNextDirOutwards.CrossRH(ezVec3(0, 0, 1));
      poly[0] = AddVertex(vNextBottomInner, vNormal, ezVec2(0, 0), color, iCustomIndex, mTransform);
      poly[3] = AddVertex(vNextBottomOuter, vNormal, ezVec2(1, 0), color, iCustomIndex, mTransform);
      poly[1] = AddVertex(vNextTopInner, vNormal, ezVec2(0, 1), color, iCustomIndex, mTransform);
      poly[2] = AddVertex(vNextTopOuter, vNormal, ezVec2(1, 1), color, iCustomIndex, mTransform);
      AddPolygon(poly, bFlipWinding);
    }

    if (bMakeSteps)
    {
      vNextTopOuter.z += fStepHeight;
      vNextTopInner.z += fStepHeight;
      vNextBottomOuter.z += fStepHeight;
      vNextBottomInner.z += fStepHeight;

      fBottomZ = fTopZ;
      fTopZ += fStepHeight;
    }
  }
}

EZ_STATICLINK_FILE(Core, Core_Graphics_Implementation_Geometry);
