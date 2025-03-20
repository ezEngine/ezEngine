#include <Core/CorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Math/Quat.h>
#include <mikktspace/mikktspace.h>

bool ezGeometry::GeoOptions::IsFlipWindingNecessary() const
{
  return m_Transform.GetRotationalPart().GetDeterminant() < 0;
}

bool ezGeometry::Vertex::operator<(const ezGeometry::Vertex& rhs) const
{
  if (m_vPosition != rhs.m_vPosition)
    return m_vPosition < rhs.m_vPosition;

  if (m_vNormal != rhs.m_vNormal)
    return m_vNormal < rhs.m_vNormal;

  if (m_vTangent != rhs.m_vTangent)
    return m_vTangent < rhs.m_vTangent;

  if (m_fBiTangentSign != rhs.m_fBiTangentSign)
    return m_fBiTangentSign < rhs.m_fBiTangentSign;

  if (m_vTexCoord != rhs.m_vTexCoord)
    return m_vTexCoord < rhs.m_vTexCoord;

  if (m_Color != rhs.m_Color)
    return m_Color < rhs.m_Color;

  if (m_BoneIndices != rhs.m_BoneIndices)
    return m_BoneIndices < rhs.m_BoneIndices;

  return m_BoneWeights < rhs.m_BoneWeights;
}

bool ezGeometry::Vertex::operator==(const ezGeometry::Vertex& rhs) const
{
  return m_vPosition == rhs.m_vPosition &&
         m_vNormal == rhs.m_vNormal &&
         m_vTangent == rhs.m_vTangent &&
         m_fBiTangentSign == rhs.m_fBiTangentSign &&
         m_vTexCoord == rhs.m_vTexCoord &&
         m_Color == rhs.m_Color &&
         m_BoneIndices == rhs.m_BoneIndices &&
         m_BoneWeights == rhs.m_BoneWeights;
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

ezUInt32 ezGeometry::AddVertex(const ezVec3& vPos, const ezVec3& vNormal, const ezVec2& vTexCoord, const ezColor& color, const ezVec4U16& vBoneIndices /*= ezVec4U16::MakeZero()*/, const ezColorLinearUB& boneWeights /*= ezColorLinearUB(255, 0, 0, 0)*/)
{
  Vertex& v = m_Vertices.ExpandAndGetRef();
  v.m_vPosition = vPos;
  v.m_vNormal = vNormal;
  v.m_vTangent.SetZero();
  v.m_fBiTangentSign = 0;
  v.m_vTexCoord = vTexCoord;
  v.m_Color = color;
  v.m_BoneIndices = vBoneIndices;
  v.m_BoneWeights = boneWeights;

  return m_Vertices.GetCount() - 1;
}

void ezGeometry::AddPolygon(const ezArrayPtr<ezUInt32>& vertices, bool bFlipWinding)
{
  EZ_ASSERT_DEV(vertices.GetCount() >= 3, "Polygon must have at least 3 vertices, not {0}", vertices.GetCount());

  for (ezUInt32 v = 0; v < vertices.GetCount(); ++v)
  {
    EZ_ASSERT_DEBUG(vertices[v] < m_Vertices.GetCount(), "Invalid vertex index {0}, geometry only has {1} vertices", vertices[v], m_Vertices.GetCount());
  }

  m_Polygons.ExpandAndGetRef().m_Vertices = vertices;

  if (bFlipWinding)
  {
    m_Polygons.PeekBack().FlipWinding();
  }
}

void ezGeometry::AddLine(ezUInt32 uiStartVertex, ezUInt32 uiEndVertex)
{
  EZ_ASSERT_DEV(uiStartVertex < m_Vertices.GetCount(), "Invalid vertex index {0}, geometry only has {1} vertices", uiStartVertex, m_Vertices.GetCount());
  EZ_ASSERT_DEV(uiEndVertex < m_Vertices.GetCount(), "Invalid vertex index {0}, geometry only has {1} vertices", uiEndVertex, m_Vertices.GetCount());

  Line l;
  l.m_uiStartVertex = uiStartVertex;
  l.m_uiEndVertex = uiEndVertex;

  m_Lines.PushBack(l);
}


void ezGeometry::TriangulatePolygons(ezUInt32 uiMaxVerticesInPolygon /*= 3*/)
{
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

      m_Polygons.RemoveAtAndSwap(p);
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

    poly.m_vNormal.CalculateNormal(v1, v2, v3).IgnoreResult();
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
    m_Vertices[v].m_vNormal.NormalizeIfNotZero(ezVec3(0, 1, 0)).IgnoreResult();
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
  static void getPosition(const SMikkTSpaceContext* pContext, float pPosOut[], const int iFace, const int iVert)
  {
    TangentContext& context = *static_cast<TangentContext*>(pContext->m_pUserData);
    ezUInt32 iVertexIndex = context.m_pGeom->GetPolygons()[iFace].m_Vertices[iVert];
    const ezVec3& pos = context.m_pGeom->GetVertices()[iVertexIndex].m_vPosition;
    pPosOut[0] = pos.x;
    pPosOut[1] = pos.y;
    pPosOut[2] = pos.z;
  }
  static void getNormal(const SMikkTSpaceContext* pContext, float pNormOut[], const int iFace, const int iVert)
  {
    TangentContext& context = *static_cast<TangentContext*>(pContext->m_pUserData);
    ezUInt32 iVertexIndex = context.m_pGeom->GetPolygons()[iFace].m_Vertices[iVert];
    const ezVec3& normal = context.m_pGeom->GetVertices()[iVertexIndex].m_vNormal;
    pNormOut[0] = normal.x;
    pNormOut[1] = normal.y;
    pNormOut[2] = normal.z;
  }
  static void getTexCoord(const SMikkTSpaceContext* pContext, float pTexcOut[], const int iFace, const int iVert)
  {
    TangentContext& context = *static_cast<TangentContext*>(pContext->m_pUserData);
    ezUInt32 iVertexIndex = context.m_pGeom->GetPolygons()[iFace].m_Vertices[iVert];
    const ezVec2& tex = context.m_pGeom->GetVertices()[iVertexIndex].m_vTexCoord;
    pTexcOut[0] = tex.x;
    pTexcOut[1] = tex.y;
  }
  static void setTSpaceBasic(const SMikkTSpaceContext* pContext, const float pTangent[], const float fSign, const int iFace, const int iVert)
  {
    TangentContext& context = *static_cast<TangentContext*>(pContext->m_pUserData);
    ezUInt32 iVertexIndex = context.m_pGeom->GetPolygons()[iFace].m_Vertices[iVert];
    ezGeometry::Vertex v = context.m_pGeom->GetVertices()[iVertexIndex];
    v.m_vTangent.x = pTangent[0];
    v.m_vTangent.y = pTangent[1];
    v.m_vTangent.z = pTangent[2];
    v.m_fBiTangentSign = fSign;

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

  static void setTSpace(const SMikkTSpaceContext* pContext, const float pTangent[], const float pBiTangent[], const float fMagS, const float fMagT, const tbool isOrientationPreserving, const int iFace, const int iVert)
  {
    EZ_IGNORE_UNUSED(pContext);
    EZ_IGNORE_UNUSED(pTangent);
    EZ_IGNORE_UNUSED(pBiTangent);
    EZ_IGNORE_UNUSED(fMagS);
    EZ_IGNORE_UNUSED(fMagT);
    EZ_IGNORE_UNUSED(isOrientationPreserving);
    EZ_IGNORE_UNUSED(iFace);
    EZ_IGNORE_UNUSED(iVert);
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

void ezGeometry::ValidateTangents(float fEpsilon)
{
  for (auto& vertex : m_Vertices)
  {
    // checking for orthogonality to the normal and for squared unit length (standard case) or 3 (magic number for binormal inversion)
    if (!ezMath::IsEqual(vertex.m_vNormal.GetLengthSquared(), 1.f, fEpsilon) || !ezMath::IsEqual(vertex.m_vNormal.Dot(vertex.m_vTangent), 0.f, fEpsilon) || !(ezMath::IsEqual(vertex.m_vTangent.GetLengthSquared(), 1.f, fEpsilon) || ezMath::IsEqual(vertex.m_vTangent.GetLengthSquared(), 3.f, fEpsilon)))
    {
      vertex.m_vTangent.SetZero();
    }
  }
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

void ezGeometry::SetAllVertexBoneIndices(const ezVec4U16& vBoneIndices, ezUInt32 uiFirstVertex)
{
  for (ezUInt32 v = uiFirstVertex; v < m_Vertices.GetCount(); ++v)
    m_Vertices[v].m_BoneIndices = vBoneIndices;
}

void ezGeometry::SetAllVertexColor(const ezColor& color, ezUInt32 uiFirstVertex)
{
  for (ezUInt32 v = uiFirstVertex; v < m_Vertices.GetCount(); ++v)
    m_Vertices[v].m_Color = color;
}


void ezGeometry::SetAllVertexTexCoord(const ezVec2& vTexCoord, ezUInt32 uiFirstVertex /*= 0*/)
{
  for (ezUInt32 v = uiFirstVertex; v < m_Vertices.GetCount(); ++v)
    m_Vertices[v].m_vTexCoord = vTexCoord;
}

void ezGeometry::TransformVertices(const ezMat4& mTransform, ezUInt32 uiFirstVertex)
{
  if (mTransform.IsIdentity(ezMath::SmallEpsilon<float>()))
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

void ezGeometry::AddRect(const ezVec2& vSize, ezUInt32 uiTesselationX, ezUInt32 uiTesselationY, const GeoOptions& options)
{
  if (uiTesselationX == 0)
    uiTesselationX = 1;
  if (uiTesselationY == 0)
    uiTesselationY = 1;

  const ezVec2 halfSize = vSize * 0.5f;
  const bool bFlipWinding = options.IsFlipWindingNecessary();

  const ezQuat mainDir = ezBasisAxis::GetBasisRotation(ezBasisAxis::PositiveZ, options.m_MainAxis);

  const ezVec2 sizeFraction = vSize.CompDiv(ezVec2(static_cast<float>(uiTesselationX), static_cast<float>(uiTesselationY)));

  for (ezUInt32 vy = 0; vy < uiTesselationY + 1; ++vy)
  {
    for (ezUInt32 vx = 0; vx < uiTesselationX + 1; ++vx)
    {
      const ezVec2 tc((float)vx / (float)uiTesselationX, (float)vy / (float)uiTesselationY);

      AddVertex(options, mainDir * ezVec3(-halfSize.x + vx * sizeFraction.x, -halfSize.y + vy * sizeFraction.y, 0), mainDir * ezVec3(0, 0, 1), tc);
    }
  }

  ezUInt32 idx[4];

  ezUInt32 uiFirstIndex = 0;

  for (ezUInt32 vy = 0; vy < uiTesselationY; ++vy)
  {
    for (ezUInt32 vx = 0; vx < uiTesselationX; ++vx)
    {

      idx[0] = uiFirstIndex;
      idx[1] = uiFirstIndex + 1;
      idx[2] = uiFirstIndex + uiTesselationX + 2;
      idx[3] = uiFirstIndex + uiTesselationX + 1;

      AddPolygon(idx, bFlipWinding);

      ++uiFirstIndex;
    }

    ++uiFirstIndex;
  }
}

void ezGeometry::AddBox(const ezVec3& vFullExtents, bool bExtraVerticesForTexturing, const GeoOptions& options)
{
  const ezVec3 halfSize = vFullExtents * 0.5f;
  const bool bFlipWinding = options.IsFlipWindingNecessary();

  if (bExtraVerticesForTexturing)
  {
    ezUInt32 idx[4];

    {
      idx[0] = AddVertex(options, ezVec3(-halfSize.x, -halfSize.y, +halfSize.z), ezVec3(0, 0, 1), ezVec2(0, 1));
      idx[1] = AddVertex(options, ezVec3(+halfSize.x, -halfSize.y, +halfSize.z), ezVec3(0, 0, 1), ezVec2(0, 0));
      idx[2] = AddVertex(options, ezVec3(+halfSize.x, +halfSize.y, +halfSize.z), ezVec3(0, 0, 1), ezVec2(1, 0));
      idx[3] = AddVertex(options, ezVec3(-halfSize.x, +halfSize.y, +halfSize.z), ezVec3(0, 0, 1), ezVec2(1, 1));
      AddPolygon(idx, bFlipWinding);
    }

    {
      idx[0] = AddVertex(options, ezVec3(-halfSize.x, +halfSize.y, -halfSize.z), ezVec3(0, 0, -1), ezVec2(1, 0));
      idx[1] = AddVertex(options, ezVec3(+halfSize.x, +halfSize.y, -halfSize.z), ezVec3(0, 0, -1), ezVec2(1, 1));
      idx[2] = AddVertex(options, ezVec3(+halfSize.x, -halfSize.y, -halfSize.z), ezVec3(0, 0, -1), ezVec2(0, 1));
      idx[3] = AddVertex(options, ezVec3(-halfSize.x, -halfSize.y, -halfSize.z), ezVec3(0, 0, -1), ezVec2(0, 0));
      AddPolygon(idx, bFlipWinding);
    }

    {
      idx[0] = AddVertex(options, ezVec3(-halfSize.x, -halfSize.y, -halfSize.z), ezVec3(-1, 0, 0), ezVec2(0, 1));
      idx[1] = AddVertex(options, ezVec3(-halfSize.x, -halfSize.y, +halfSize.z), ezVec3(-1, 0, 0), ezVec2(0, 0));
      idx[2] = AddVertex(options, ezVec3(-halfSize.x, +halfSize.y, +halfSize.z), ezVec3(-1, 0, 0), ezVec2(1, 0));
      idx[3] = AddVertex(options, ezVec3(-halfSize.x, +halfSize.y, -halfSize.z), ezVec3(-1, 0, 0), ezVec2(1, 1));
      AddPolygon(idx, bFlipWinding);
    }

    {
      idx[0] = AddVertex(options, ezVec3(+halfSize.x, +halfSize.y, -halfSize.z), ezVec3(1, 0, 0), ezVec2(0, 1));
      idx[1] = AddVertex(options, ezVec3(+halfSize.x, +halfSize.y, +halfSize.z), ezVec3(1, 0, 0), ezVec2(0, 0));
      idx[2] = AddVertex(options, ezVec3(+halfSize.x, -halfSize.y, +halfSize.z), ezVec3(1, 0, 0), ezVec2(1, 0));
      idx[3] = AddVertex(options, ezVec3(+halfSize.x, -halfSize.y, -halfSize.z), ezVec3(1, 0, 0), ezVec2(1, 1));
      AddPolygon(idx, bFlipWinding);
    }

    {
      idx[0] = AddVertex(options, ezVec3(+halfSize.x, -halfSize.y, -halfSize.z), ezVec3(0, -1, 0), ezVec2(0, 1));
      idx[1] = AddVertex(options, ezVec3(+halfSize.x, -halfSize.y, +halfSize.z), ezVec3(0, -1, 0), ezVec2(0, 0));
      idx[2] = AddVertex(options, ezVec3(-halfSize.x, -halfSize.y, +halfSize.z), ezVec3(0, -1, 0), ezVec2(1, 0));
      idx[3] = AddVertex(options, ezVec3(-halfSize.x, -halfSize.y, -halfSize.z), ezVec3(0, -1, 0), ezVec2(1, 1));
      AddPolygon(idx, bFlipWinding);
    }

    {
      idx[0] = AddVertex(options, ezVec3(-halfSize.x, +halfSize.y, -halfSize.z), ezVec3(0, +1, 0), ezVec2(0, 1));
      idx[1] = AddVertex(options, ezVec3(-halfSize.x, +halfSize.y, +halfSize.z), ezVec3(0, +1, 0), ezVec2(0, 0));
      idx[2] = AddVertex(options, ezVec3(+halfSize.x, +halfSize.y, +halfSize.z), ezVec3(0, +1, 0), ezVec2(1, 0));
      idx[3] = AddVertex(options, ezVec3(+halfSize.x, +halfSize.y, -halfSize.z), ezVec3(0, +1, 0), ezVec2(1, 1));
      AddPolygon(idx, bFlipWinding);
    }
  }
  else
  {
    ezUInt32 idx[8];

    idx[0] = AddVertex(options, ezVec3(-halfSize.x, -halfSize.y, halfSize.z), ezVec3(0, 0, 1), ezVec2(0));
    idx[1] = AddVertex(options, ezVec3(halfSize.x, -halfSize.y, halfSize.z), ezVec3(0, 0, 1), ezVec2(0));
    idx[2] = AddVertex(options, ezVec3(halfSize.x, halfSize.y, halfSize.z), ezVec3(0, 0, 1), ezVec2(0));
    idx[3] = AddVertex(options, ezVec3(-halfSize.x, halfSize.y, halfSize.z), ezVec3(0, 0, 1), ezVec2(0));

    idx[4] = AddVertex(options, ezVec3(-halfSize.x, -halfSize.y, -halfSize.z), ezVec3(0, 0, -1), ezVec2(0));
    idx[5] = AddVertex(options, ezVec3(halfSize.x, -halfSize.y, -halfSize.z), ezVec3(0, 0, -1), ezVec2(0));
    idx[6] = AddVertex(options, ezVec3(halfSize.x, halfSize.y, -halfSize.z), ezVec3(0, 0, -1), ezVec2(0));
    idx[7] = AddVertex(options, ezVec3(-halfSize.x, halfSize.y, -halfSize.z), ezVec3(0, 0, -1), ezVec2(0));

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
}

void ezGeometry::AddLineBox(const ezVec3& vSize, const GeoOptions& options)
{
  const ezVec3 halfSize = vSize * 0.5f;

  AddVertex(options, ezVec3(-halfSize.x, -halfSize.y, halfSize.z), ezVec3(0, 0, 1), ezVec2(0));
  AddVertex(options, ezVec3(halfSize.x, -halfSize.y, halfSize.z), ezVec3(0, 0, 1), ezVec2(0));
  AddVertex(options, ezVec3(halfSize.x, halfSize.y, halfSize.z), ezVec3(0, 0, 1), ezVec2(0));
  AddVertex(options, ezVec3(-halfSize.x, halfSize.y, halfSize.z), ezVec3(0, 0, 1), ezVec2(0));

  AddVertex(options, ezVec3(-halfSize.x, -halfSize.y, -halfSize.z), ezVec3(0, 0, -1), ezVec2(0));
  AddVertex(options, ezVec3(halfSize.x, -halfSize.y, -halfSize.z), ezVec3(0, 0, -1), ezVec2(0));
  AddVertex(options, ezVec3(halfSize.x, halfSize.y, -halfSize.z), ezVec3(0, 0, -1), ezVec2(0));
  AddVertex(options, ezVec3(-halfSize.x, halfSize.y, -halfSize.z), ezVec3(0, 0, -1), ezVec2(0));

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

void ezGeometry::AddLineBoxCorners(const ezVec3& vSize, float fCornerFraction, const GeoOptions& options)
{
  fCornerFraction = ezMath::Clamp(fCornerFraction, 0.0f, 1.0f);
  fCornerFraction *= 0.5f;
  const ezVec3 halfSize = vSize * 0.5f;

  AddVertex(options, ezVec3(-halfSize.x, -halfSize.y, halfSize.z), ezVec3(0, 0, 1), ezVec2(0));
  AddVertex(options, ezVec3(halfSize.x, -halfSize.y, halfSize.z), ezVec3(0, 0, 1), ezVec2(0));
  AddVertex(options, ezVec3(halfSize.x, halfSize.y, halfSize.z), ezVec3(0, 0, 1), ezVec2(0));
  AddVertex(options, ezVec3(-halfSize.x, halfSize.y, halfSize.z), ezVec3(0, 0, 1), ezVec2(0));

  AddVertex(options, ezVec3(-halfSize.x, -halfSize.y, -halfSize.z), ezVec3(0, 0, -1), ezVec2(0));
  AddVertex(options, ezVec3(halfSize.x, -halfSize.y, -halfSize.z), ezVec3(0, 0, -1), ezVec2(0));
  AddVertex(options, ezVec3(halfSize.x, halfSize.y, -halfSize.z), ezVec3(0, 0, -1), ezVec2(0));
  AddVertex(options, ezVec3(-halfSize.x, halfSize.y, -halfSize.z), ezVec3(0, 0, -1), ezVec2(0));

  for (ezUInt32 c = 0; c < 8; ++c)
  {
    const ezVec3& op = m_Vertices[c].m_vPosition;

    const ezVec3 op1 = ezVec3(op.x, op.y, -ezMath::Sign(op.z) * ezMath::Abs(op.z));
    const ezVec3 op2 = ezVec3(op.x, -ezMath::Sign(op.y) * ezMath::Abs(op.y), op.z);
    const ezVec3 op3 = ezVec3(-ezMath::Sign(op.x) * ezMath::Abs(op.x), op.y, op.z);

    const ezUInt32 ix1 = AddVertex(options, ezMath::Lerp(op, op1, fCornerFraction), m_Vertices[c].m_vPosition, m_Vertices[c].m_vTexCoord);
    const ezUInt32 ix2 = AddVertex(options, ezMath::Lerp(op, op2, fCornerFraction), m_Vertices[c].m_vPosition, m_Vertices[c].m_vTexCoord);
    const ezUInt32 ix3 = AddVertex(options, ezMath::Lerp(op, op3, fCornerFraction), m_Vertices[c].m_vPosition, m_Vertices[c].m_vTexCoord);

    AddLine(c, ix1);
    AddLine(c, ix2);
    AddLine(c, ix3);
  }
}

void ezGeometry::AddPyramid(float fBaseSize, float fHeight, bool bCap, const GeoOptions& options)
{
  const ezQuat tilt = ezBasisAxis::GetBasisRotation(options.m_MainAxis, ezBasisAxis::PositiveZ);
  const ezMat4 trans = options.m_Transform * tilt.GetAsMat4();

  const float halfSize = fBaseSize * 0.5f;
  const bool bFlipWinding = options.IsFlipWindingNecessary();
  ezUInt32 quad[4];

  quad[0] = AddVertex(trans, options, ezVec3(-halfSize, halfSize, 0), ezVec3(-1, 1, 0).GetNormalized(), ezVec2(0));
  quad[1] = AddVertex(trans, options, ezVec3(halfSize, halfSize, 0), ezVec3(1, 1, 0).GetNormalized(), ezVec2(0));
  quad[2] = AddVertex(trans, options, ezVec3(halfSize, -halfSize, 0), ezVec3(1, -1, 0).GetNormalized(), ezVec2(0));
  quad[3] = AddVertex(trans, options, ezVec3(-halfSize, -halfSize, 0), ezVec3(-1, -1, 0).GetNormalized(), ezVec2(0));

  const ezUInt32 tip = AddVertex(trans, options, ezVec3(0, 0, fHeight), ezVec3(0, 0, 1), ezVec2(0));

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

void ezGeometry::AddGeodesicSphere(float fRadius, ezUInt8 uiSubDivisions, const GeoOptions& options)
{
  const bool bFlipWinding = options.IsFlipWindingNecessary();
  struct Triangle
  {
    Triangle(ezUInt32 ui1, ezUInt32 ui2, ezUInt32 ui3)
    {
      m_uiIndex[0] = ui1;
      m_uiIndex[1] = ui2;
      m_uiIndex[2] = ui3;
    }

    ezUInt32 m_uiIndex[3];
  };

  struct Edge
  {
    Edge() = default;

    Edge(ezUInt32 uiId1, ezUInt32 uiId2)
    {
      m_uiVertex[0] = ezMath::Min(uiId1, uiId2);
      m_uiVertex[1] = ezMath::Max(uiId1, uiId2);
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
  ezVec4U16 boneIndices(options.m_uiBoneIndex, 0, 0, 0);

  // create icosahedron
  {
    ezMat3 mRotX, mRotZ, mRotZh;
    mRotX = ezMat3::MakeRotationX(ezAngle::MakeFromDegree(360.0f / 6.0f));
    mRotZ = ezMat3::MakeRotationZ(ezAngle::MakeFromDegree(-360.0f / 5.0f));
    mRotZh = ezMat3::MakeRotationZ(ezAngle::MakeFromDegree(-360.0f / 10.0f));

    ezUInt32 vert[12];
    ezVec3 vDir(0, 0, 1);

    vDir.Normalize();
    vert[0] = AddVertex(vDir * fRadius, vDir, ezVec2::MakeZero(), options.m_Color, boneIndices);

    vDir = mRotX * vDir;

    for (ezInt32 i = 0; i < 5; ++i)
    {
      vDir.Normalize();
      vert[1 + i] = AddVertex(vDir * fRadius, vDir, ezVec2::MakeZero(), options.m_Color, boneIndices);
      vDir = mRotZ * vDir;
    }

    vDir = mRotX * vDir;
    vDir = mRotZh * vDir;

    for (ezInt32 i = 0; i < 5; ++i)
    {
      vDir.Normalize();
      vert[6 + i] = AddVertex(vDir * fRadius, vDir, ezVec2::MakeZero(), options.m_Color, boneIndices);
      vDir = mRotZ * vDir;
    }

    vDir.Set(0, 0, -1);
    vDir.Normalize();
    vert[11] = AddVertex(vDir * fRadius, vDir, ezVec2::MakeZero(), options.m_Color, boneIndices);


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
          const ezVec3 vCenter = (m_Vertices[Edges[i].m_uiVertex[0]].m_vPosition + m_Vertices[Edges[i].m_uiVertex[1]].m_vPosition).GetNormalized();
          uiNewVert[i] = AddVertex(vCenter * fRadius, vCenter, ezVec2::MakeZero(), options.m_Color, boneIndices);

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
  TransformVertices(options.m_Transform, uiFirstVertex);
}

void ezGeometry::AddCylinder(float fRadiusTop, float fRadiusBottom, float fPositiveLength, float fNegativeLength, bool bCapTop, bool bCapBottom, ezUInt16 uiSegments, const GeoOptions& options, ezAngle fraction /*= ezAngle::MakeFromDegree(360.0f)*/)
{
  uiSegments = ezMath::Max<ezUInt16>(uiSegments, 3u);
  fraction = ezMath::Clamp(fraction, ezAngle(), ezAngle::MakeFromDegree(360.0f));

  const bool bFlipWinding = options.IsFlipWindingNecessary();
  const bool bIsFraction = fraction.GetDegree() < 360.0f;
  const ezAngle fDegStep = ezAngle::MakeFromDegree(fraction.GetDegree() / uiSegments);

  const ezVec3 vTopCenter(0, 0, fPositiveLength);
  const ezVec3 vBottomCenter(0, 0, -fNegativeLength);

  const ezQuat tilt = ezBasisAxis::GetBasisRotation(options.m_MainAxis, ezBasisAxis::PositiveZ);
  const ezMat4 trans = options.m_Transform * tilt.GetAsMat4();

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

      VertsTop.PushBack(AddVertex(trans, options, vTopCenter + vDir * fRadiusTop, vDir, ezVec2(fU, 0)));
      VertsBottom.PushBack(AddVertex(trans, options, vBottomCenter + vDir * fRadiusBottom, vDir, ezVec2(fU, 1)));
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
    quad[0] = AddVertex(trans, options, vTopCenter + vDir0 * fRadiusTop, vNrm0, ezVec2(0, 0));
    quad[1] = AddVertex(trans, options, vTopCenter, vNrm0, ezVec2(1, 0));
    quad[2] = AddVertex(trans, options, vBottomCenter, vNrm0, ezVec2(1, 1));
    quad[3] = AddVertex(trans, options, vBottomCenter + vDir0 * fRadiusBottom, vNrm0, ezVec2(0, 1));


    AddPolygon(quad, bFlipWinding);

    const ezVec3 vNrm1 = ezVec3(0, 0, 1).CrossRH(vDir1).GetNormalized();
    quad[0] = AddVertex(trans, options, vTopCenter, vNrm1, ezVec2(0, 0));
    quad[1] = AddVertex(trans, options, vTopCenter + vDir1 * fRadiusTop, vNrm1, ezVec2(1, 0));
    quad[2] = AddVertex(trans, options, vBottomCenter + vDir1 * fRadiusBottom, vNrm1, ezVec2(1, 1));
    quad[3] = AddVertex(trans, options, vBottomCenter, vNrm1, ezVec2(0, 1));

    AddPolygon(quad, bFlipWinding);
  }

  if (bCapBottom)
  {
    ezHybridArray<ezUInt32, 512> VertsBottom;

    if (bIsFraction)
    {
      const ezUInt32 uiCenterVtx = AddVertex(trans, options, vBottomCenter, ezVec3(0, 0, -1), ezVec2(0));

      for (ezInt32 i = uiSegments; i >= 0; --i)
      {
        const ezAngle deg = (float)i * fDegStep;

        const float fX = ezMath::Cos(deg);
        const float fY = ezMath::Sin(deg);

        const ezVec3 vDir(fX, fY, 0);

        AddVertex(trans, options, vBottomCenter + vDir * fRadiusBottom, ezVec3(0, 0, -1), ezVec2(fY, fX));
      }

      VertsBottom.SetCountUninitialized(3);
      VertsBottom[0] = uiCenterVtx;

      for (ezUInt32 i = 0; i < uiSegments; ++i)
      {
        VertsBottom[1] = uiCenterVtx + i + 1;
        VertsBottom[2] = uiCenterVtx + i + 2;

        AddPolygon(VertsBottom, bFlipWinding);
      }
    }
    else
    {
      for (ezInt32 i = uiSegments - 1; i >= 0; --i)
      {
        const ezAngle deg = (float)i * fDegStep;

        const float fX = ezMath::Cos(deg);
        const float fY = ezMath::Sin(deg);

        const ezVec3 vDir(fX, fY, 0);

        VertsBottom.PushBack(AddVertex(trans, options, vBottomCenter + vDir * fRadiusBottom, ezVec3(0, 0, -1), ezVec2(fY, fX)));
      }

      AddPolygon(VertsBottom, bFlipWinding);
    }
  }

  if (bCapTop)
  {
    ezHybridArray<ezUInt32, 512> VertsTop;

    if (bIsFraction)
    {
      const ezUInt32 uiCenterVtx = AddVertex(trans, options, vTopCenter, ezVec3(0, 0, 1), ezVec2(0));

      for (ezInt32 i = 0; i <= uiSegments; ++i)
      {
        const ezAngle deg = (float)i * fDegStep;

        const float fX = ezMath::Cos(deg);
        const float fY = ezMath::Sin(deg);

        const ezVec3 vDir(fX, fY, 0);

        AddVertex(trans, options, vTopCenter + vDir * fRadiusTop, ezVec3(0, 0, 1), ezVec2(fY, -fX));
      }

      VertsTop.SetCountUninitialized(3);
      VertsTop[0] = uiCenterVtx;

      for (ezUInt32 i = 0; i < uiSegments; ++i)
      {
        VertsTop[1] = uiCenterVtx + i + 1;
        VertsTop[2] = uiCenterVtx + i + 2;

        AddPolygon(VertsTop, bFlipWinding);
      }
    }
    else
    {
      for (ezInt32 i = 0; i < uiSegments; ++i)
      {
        const ezAngle deg = (float)i * fDegStep;

        const float fX = ezMath::Cos(deg);
        const float fY = ezMath::Sin(deg);

        const ezVec3 vDir(fX, fY, 0);

        VertsTop.PushBack(AddVertex(trans, options, vTopCenter + vDir * fRadiusTop, ezVec3(0, 0, 1), ezVec2(fY, -fX)));
      }

      AddPolygon(VertsTop, bFlipWinding);
    }
  }
}

void ezGeometry::AddCylinderOnePiece(float fRadiusTop, float fRadiusBottom, float fPositiveLength, float fNegativeLength, ezUInt16 uiSegments, const GeoOptions& options)
{
  uiSegments = ezMath::Max<ezUInt16>(uiSegments, 3u);

  const bool bFlipWinding = options.IsFlipWindingNecessary();
  const ezAngle fDegStep = ezAngle::MakeFromDegree(360.0f / uiSegments);

  const ezQuat tilt = ezBasisAxis::GetBasisRotation(options.m_MainAxis, ezBasisAxis::PositiveZ);
  const ezMat4 trans = options.m_Transform * tilt.GetAsMat4();

  const ezVec3 vTopCenter(0, 0, fPositiveLength);
  const ezVec3 vBottomCenter(0, 0, -fNegativeLength);

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

      VertsTop.PushBack(AddVertex(trans, options, vTopCenter + vDir * fRadiusTop, vDir, ezVec2(fU, 0)));
      VertsBottom.PushBack(AddVertex(trans, options, vBottomCenter + vDir * fRadiusBottom, vDir, ezVec2(fU, 1)));
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

void ezGeometry::AddCone(float fRadius, float fHeight, bool bCap, ezUInt16 uiSegments, const GeoOptions& options)
{
  uiSegments = ezMath::Max<ezUInt16>(uiSegments, 3);

  const ezQuat tilt = ezBasisAxis::GetBasisRotation(options.m_MainAxis, ezBasisAxis::PositiveZ);
  const ezMat4 trans = options.m_Transform * tilt.GetAsMat4();

  const bool bFlipWinding = options.IsFlipWindingNecessary();

  ezHybridArray<ezUInt32, 512> VertsBottom;

  const ezAngle fDegStep = ezAngle::MakeFromDegree(360.0f / uiSegments);

  const ezUInt32 uiTip = AddVertex(trans, options, ezVec3(0, 0, fHeight), ezVec3(0, 0, 1));

  for (ezInt32 i = uiSegments - 1; i >= 0; --i)
  {
    const ezAngle deg = (float)i * fDegStep;

    ezVec3 vDir(ezMath::Cos(deg), ezMath::Sin(deg), 0);

    VertsBottom.PushBack(AddVertex(trans, options, vDir * fRadius, vDir));
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

void ezGeometry::AddStackedSphere(float fRadius, ezUInt16 uiSegments, ezUInt16 uiStacks, const GeoOptions& options)
{
  uiSegments = ezMath::Max<ezUInt16>(uiSegments, 3u);
  uiStacks = ezMath::Max<ezUInt16>(uiStacks, 2u);

  const ezQuat tilt = ezBasisAxis::GetBasisRotation(options.m_MainAxis, ezBasisAxis::PositiveZ);
  const ezMat4 trans = options.m_Transform * tilt.GetAsMat4();

  const bool bFlipWinding = options.IsFlipWindingNecessary();
  const ezAngle fDegreeDiffSegments = ezAngle::MakeFromDegree(360.0f / (float)(uiSegments));
  const ezAngle fDegreeDiffStacks = ezAngle::MakeFromDegree(180.0f / (float)(uiStacks));

  const ezUInt32 uiFirstVertex = m_Vertices.GetCount();

  // first create all the vertex positions
  for (ezUInt32 st = 1; st < uiStacks; ++st)
  {
    const ezAngle fDegreeStack = ezAngle::MakeFromDegree(-90.0f + (st * fDegreeDiffStacks.GetDegree()));
    const float fCosDS = ezMath::Cos(fDegreeStack);
    const float fSinDS = ezMath::Sin(fDegreeStack);
    const float fY = -fSinDS * fRadius;

    const float fV = (float)st / (float)uiStacks;

    for (ezUInt32 sp = 0; sp < uiSegments + 1u; ++sp)
    {
      float fU = ((float)sp / (float)(uiSegments)) * 2.0f;

      const ezAngle fDegree = (float)sp * fDegreeDiffSegments;

      ezVec3 vPos;
      vPos.x = ezMath::Cos(fDegree) * fRadius * fCosDS;
      vPos.y = -ezMath::Sin(fDegree) * fRadius * fCosDS;
      vPos.z = fY;

      ezVec3 vNormal = vPos;
      vNormal.NormalizeIfNotZero(ezVec3(0, 0, 1)).IgnoreResult();
      AddVertex(trans, options, vPos, vNormal, ezVec2(fU, fV));
    }
  }

  ezUInt32 tri[3];
  ezUInt32 quad[4];

  // now create the top cone
  for (ezUInt32 p = 0; p < uiSegments; ++p)
  {
    float fU = ((p + 0.5f) / (float)(uiSegments)) * 2.0f;

    tri[0] = AddVertex(trans, options, ezVec3(0, 0, fRadius), ezVec3(0, 0, 1), ezVec2(fU, 0));
    tri[1] = uiFirstVertex + p + 1;
    tri[2] = uiFirstVertex + p;

    AddPolygon(tri, bFlipWinding);
  }

  // now create the stacks in the middle
  for (ezUInt16 st = 0; st < uiStacks - 2; ++st)
  {
    const ezUInt32 uiRowBottom = (uiSegments + 1) * st;
    const ezUInt32 uiRowTop = (uiSegments + 1) * (st + 1);

    for (ezInt32 i = 0; i < uiSegments; ++i)
    {
      quad[0] = uiFirstVertex + (uiRowTop + i + 1);
      quad[1] = uiFirstVertex + (uiRowTop + i);
      quad[2] = uiFirstVertex + (uiRowBottom + i);
      quad[3] = uiFirstVertex + (uiRowBottom + i + 1);

      AddPolygon(quad, bFlipWinding);
    }
  }

  const ezInt32 iTopStack = (uiSegments + 1) * (uiStacks - 2);

  // now create the bottom cone
  for (ezUInt32 p = 0; p < uiSegments; ++p)
  {
    float fU = ((p + 0.5f) / (float)(uiSegments)) * 2.0f;

    tri[0] = AddVertex(trans, options, ezVec3(0, 0, -fRadius), ezVec3(0, 0, -1), ezVec2(fU, 1));
    tri[1] = uiFirstVertex + (iTopStack + p);
    tri[2] = uiFirstVertex + (iTopStack + p + 1);

    AddPolygon(tri, bFlipWinding);
  }
}

void ezGeometry::AddHalfSphere(float fRadius, ezUInt16 uiSegments, ezUInt16 uiStacks, bool bCap, const GeoOptions& options)
{
  uiSegments = ezMath::Max<ezUInt16>(uiSegments, 3u);
  uiStacks = ezMath::Max<ezUInt16>(uiStacks, 1u);

  const ezQuat tilt = ezBasisAxis::GetBasisRotation(options.m_MainAxis, ezBasisAxis::PositiveZ);
  const ezMat4 trans = options.m_Transform * tilt.GetAsMat4();

  const bool bFlipWinding = options.IsFlipWindingNecessary();
  const ezAngle fDegreeDiffSegments = ezAngle::MakeFromDegree(360.0f / (float)(uiSegments));
  const ezAngle fDegreeDiffStacks = ezAngle::MakeFromDegree(90.0f / (float)(uiStacks));

  const ezUInt32 uiFirstVertex = m_Vertices.GetCount();

  // first create all the vertex positions
  for (ezUInt32 st = 0; st < uiStacks; ++st)
  {
    const ezAngle fDegreeStack = ezAngle::MakeFromDegree(-90.0f + ((st + 1) * fDegreeDiffStacks.GetDegree()));
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

      AddVertex(trans, options, vPos, vPos.GetNormalized(), ezVec2(fU, fV));
    }
  }

  ezUInt32 uiTopVertex = AddVertex(trans, options, ezVec3(0, 0, fRadius), ezVec3(0, 0, 1), ezVec2(0.0f));

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

void ezGeometry::AddCapsule(float fRadius, float fHeight, ezUInt16 uiSegments, ezUInt16 uiStacks, const GeoOptions& options)
{
  uiSegments = ezMath::Max<ezUInt16>(uiSegments, 3u);
  uiStacks = ezMath::Max<ezUInt16>(uiStacks, 1u);
  fHeight = ezMath::Max(fHeight, 0.0f);

  const ezQuat tilt = ezBasisAxis::GetBasisRotation(options.m_MainAxis, ezBasisAxis::PositiveZ);
  const ezMat4 trans = options.m_Transform * tilt.GetAsMat4();

  const bool bFlipWinding = options.IsFlipWindingNecessary();
  const ezAngle fDegreeDiffStacks = ezAngle::MakeFromDegree(90.0f / (float)(uiStacks));

  const ezUInt32 uiFirstVertex = m_Vertices.GetCount();

  // first create all the vertex positions
  const float fDegreeStepSlices = 360.0f / (float)(uiSegments);

  float fOffset = fHeight * 0.5f;

  // for (ezUInt32 h = 0; h < 2; ++h)
  {
    for (ezUInt32 st = 0; st < uiStacks; ++st)
    {
      const ezAngle fDegreeStack = ezAngle::MakeFromDegree(-90.0f + ((st + 1) * fDegreeDiffStacks.GetDegree()));
      const float fCosDS = ezMath::Cos(fDegreeStack);
      const float fSinDS = ezMath::Sin(fDegreeStack);
      const float fY = -fSinDS * fRadius;

      for (ezUInt32 sp = 0; sp < uiSegments; ++sp)
      {
        const ezAngle fDegree = ezAngle::MakeFromDegree(sp * fDegreeStepSlices);

        ezVec3 vPos;
        vPos.x = ezMath::Cos(fDegree) * fRadius * fCosDS;
        vPos.z = fY + fOffset;
        vPos.y = ezMath::Sin(fDegree) * fRadius * fCosDS;

        AddVertex(trans, options, vPos, vPos.GetNormalized(), ezVec2(0));
      }
    }

    fOffset -= fHeight;

    for (ezUInt32 st = 0; st < uiStacks; ++st)
    {
      const ezAngle fDegreeStack = ezAngle::MakeFromDegree(0.0f - (st * fDegreeDiffStacks.GetDegree()));
      const float fCosDS = ezMath::Cos(fDegreeStack);
      const float fSinDS = ezMath::Sin(fDegreeStack);
      const float fY = fSinDS * fRadius;

      for (ezUInt32 sp = 0; sp < uiSegments; ++sp)
      {
        const ezAngle fDegree = ezAngle::MakeFromDegree(sp * fDegreeStepSlices);

        ezVec3 vPos;
        vPos.x = ezMath::Cos(fDegree) * fRadius * fCosDS;
        vPos.z = fY + fOffset;
        vPos.y = ezMath::Sin(fDegree) * fRadius * fCosDS;

        AddVertex(trans, options, vPos, vPos.GetNormalized(), ezVec2(0));
      }
    }
  }

  ezUInt32 uiTopVertex = AddVertex(trans, options, ezVec3(0, 0, fRadius + fHeight * 0.5f), ezVec3(0, 0, 1), ezVec2(0));
  ezUInt32 uiBottomVertex = AddVertex(trans, options, ezVec3(0, 0, -fRadius - fHeight * 0.5f), ezVec3(0, 0, -1), ezVec2(0));

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
  ezUInt16 uiMaxStacks = static_cast<ezUInt16>(uiStacks * 2 - 1);
  for (ezUInt16 st = 0; st < uiMaxStacks; ++st)
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

void ezGeometry::AddTorus(float fInnerRadius, float fOuterRadius, ezUInt16 uiSegments, ezUInt16 uiSegmentDetail, bool bExtraVerticesForTexturing, const GeoOptions& options)
{
  uiSegments = ezMath::Max<ezUInt16>(uiSegments, 3u);
  uiSegmentDetail = ezMath::Max<ezUInt16>(uiSegmentDetail, 3u);
  fOuterRadius = ezMath::Max(fInnerRadius + 0.01f, fOuterRadius);

  const ezQuat tilt = ezBasisAxis::GetBasisRotation(options.m_MainAxis, ezBasisAxis::PositiveZ);
  const ezMat4 trans = options.m_Transform * tilt.GetAsMat4();

  const bool bFlipWinding = options.IsFlipWindingNecessary();
  const float fCylinderRadius = (fOuterRadius - fInnerRadius) * 0.5f;
  const float fLoopRadius = fInnerRadius + fCylinderRadius;

  const ezAngle fAngleStepSegment = ezAngle::MakeFromDegree(360.0f / uiSegments);
  const ezAngle fAngleStepCylinder = ezAngle::MakeFromDegree(360.0f / uiSegmentDetail);

  const ezUInt16 uiFirstVertex = static_cast<ezUInt16>(m_Vertices.GetCount());

  const ezUInt16 uiNumSegments = bExtraVerticesForTexturing ? uiSegments + 1 : uiSegments;
  const ezUInt16 uiNumSegmentDetail = bExtraVerticesForTexturing ? uiSegmentDetail + 1 : uiSegmentDetail;

  // this is the loop for the torus ring
  for (ezUInt16 seg = 0; seg < uiNumSegments; ++seg)
  {
    float fU = ((float)seg / (float)uiSegments) * 2.0f;

    const ezAngle fAngle = seg * fAngleStepSegment;

    const float fSinAngle = ezMath::Sin(fAngle);
    const float fCosAngle = ezMath::Cos(fAngle);

    const ezVec3 vLoopPos = ezVec3(fSinAngle, fCosAngle, 0) * fLoopRadius;

    // this is the loop to go round the cylinder
    for (ezUInt16 p = 0; p < uiNumSegmentDetail; ++p)
    {
      float fV = (float)p / (float)uiSegmentDetail;

      const ezAngle fCylinderAngle = p * fAngleStepCylinder;

      const ezVec3 vDir(ezMath::Cos(fCylinderAngle) * fSinAngle, ezMath::Cos(fCylinderAngle) * fCosAngle, ezMath::Sin(fCylinderAngle));

      const ezVec3 vPos = vLoopPos + fCylinderRadius * vDir;

      AddVertex(trans, options, vPos, vDir, ezVec2(fU, fV));
    }
  }

  if (bExtraVerticesForTexturing)
  {
    for (ezUInt16 seg = 0; seg < uiSegments; ++seg)
    {
      const ezUInt16 rs0 = uiFirstVertex + seg * (uiSegmentDetail + 1);
      const ezUInt16 rs1 = uiFirstVertex + (seg + 1) * (uiSegmentDetail + 1);

      for (ezUInt16 p = 0; p < uiSegmentDetail; ++p)
      {
        ezUInt32 quad[4];
        quad[0] = rs1 + p;
        quad[3] = rs1 + p + 1;
        quad[2] = rs0 + p + 1;
        quad[1] = rs0 + p;

        AddPolygon(quad, bFlipWinding);
      }
    }
  }
  else
  {
    ezUInt16 prevRing = (uiSegments - 1);

    for (ezUInt16 seg = 0; seg < uiSegments; ++seg)
    {
      const ezUInt16 thisRing = seg;

      const ezUInt16 prevRingFirstVtx = uiFirstVertex + (prevRing * uiSegmentDetail);
      ezUInt16 prevRingPrevVtx = prevRingFirstVtx + (uiSegmentDetail - 1);

      const ezUInt16 thisRingFirstVtx = uiFirstVertex + (thisRing * uiSegmentDetail);
      ezUInt16 thisRingPrevVtx = thisRingFirstVtx + (uiSegmentDetail - 1);

      for (ezUInt16 p = 0; p < uiSegmentDetail; ++p)
      {
        const ezUInt16 prevRingThisVtx = prevRingFirstVtx + p;
        const ezUInt16 thisRingThisVtx = thisRingFirstVtx + p;

        ezUInt32 quad[4];

        quad[0] = prevRingPrevVtx;
        quad[1] = prevRingThisVtx;
        quad[2] = thisRingThisVtx;
        quad[3] = thisRingPrevVtx;

        AddPolygon(quad, bFlipWinding);

        prevRingPrevVtx = prevRingThisVtx;
        thisRingPrevVtx = thisRingThisVtx;
      }

      prevRing = thisRing;
    }
  }
}

void ezGeometry::AddTexturedRamp(const ezVec3& vSize, const GeoOptions& options)
{
  const ezVec3 halfSize = vSize * 0.5f;
  const bool bFlipWinding = options.IsFlipWindingNecessary();
  ezUInt32 idx[4];
  ezUInt32 idx3[3];

  {
    ezVec3 vNormal = ezVec3(-halfSize.z, 0, halfSize.x).GetNormalized();
    idx[0] = AddVertex(options, ezVec3(-halfSize.x, -halfSize.y, -halfSize.z), vNormal, ezVec2(0, 1));
    idx[1] = AddVertex(options, ezVec3(+halfSize.x, -halfSize.y, +halfSize.z), vNormal, ezVec2(0, 0));
    idx[2] = AddVertex(options, ezVec3(+halfSize.x, +halfSize.y, +halfSize.z), vNormal, ezVec2(1, 0));
    idx[3] = AddVertex(options, ezVec3(-halfSize.x, +halfSize.y, -halfSize.z), vNormal, ezVec2(1, 1));
    AddPolygon(idx, bFlipWinding);
  }

  {
    idx[0] = AddVertex(options, ezVec3(-halfSize.x, +halfSize.y, -halfSize.z), ezVec3(0, 0, -1), ezVec2(1, 0));
    idx[1] = AddVertex(options, ezVec3(+halfSize.x, +halfSize.y, -halfSize.z), ezVec3(0, 0, -1), ezVec2(1, 1));
    idx[2] = AddVertex(options, ezVec3(+halfSize.x, -halfSize.y, -halfSize.z), ezVec3(0, 0, -1), ezVec2(0, 1));
    idx[3] = AddVertex(options, ezVec3(-halfSize.x, -halfSize.y, -halfSize.z), ezVec3(0, 0, -1), ezVec2(0, 0));
    AddPolygon(idx, bFlipWinding);
  }

  {
    idx[0] = AddVertex(options, ezVec3(+halfSize.x, +halfSize.y, -halfSize.z), ezVec3(1, 0, 0), ezVec2(0, 1));
    idx[1] = AddVertex(options, ezVec3(+halfSize.x, +halfSize.y, +halfSize.z), ezVec3(1, 0, 0), ezVec2(0, 0));
    idx[2] = AddVertex(options, ezVec3(+halfSize.x, -halfSize.y, +halfSize.z), ezVec3(1, 0, 0), ezVec2(1, 0));
    idx[3] = AddVertex(options, ezVec3(+halfSize.x, -halfSize.y, -halfSize.z), ezVec3(1, 0, 0), ezVec2(1, 1));
    AddPolygon(idx, bFlipWinding);
  }

  {
    idx3[0] = AddVertex(options, ezVec3(+halfSize.x, -halfSize.y, -halfSize.z), ezVec3(0, -1, 0), ezVec2(0, 1));
    idx3[1] = AddVertex(options, ezVec3(+halfSize.x, -halfSize.y, +halfSize.z), ezVec3(0, -1, 0), ezVec2(0, 0));
    idx3[2] = AddVertex(options, ezVec3(-halfSize.x, -halfSize.y, -halfSize.z), ezVec3(0, -1, 0), ezVec2(1, 1));
    AddPolygon(idx3, bFlipWinding);
  }

  {
    idx3[0] = AddVertex(options, ezVec3(-halfSize.x, +halfSize.y, -halfSize.z), ezVec3(0, +1, 0), ezVec2(0, 1));
    idx3[1] = AddVertex(options, ezVec3(+halfSize.x, +halfSize.y, +halfSize.z), ezVec3(0, +1, 0), ezVec2(1, 0));
    idx3[2] = AddVertex(options, ezVec3(+halfSize.x, +halfSize.y, -halfSize.z), ezVec3(0, +1, 0), ezVec2(1, 1));
    AddPolygon(idx3, bFlipWinding);
  }
}

void ezGeometry::AddStairs(const ezVec3& vSize, ezUInt32 uiNumSteps, ezAngle curvature, bool bSmoothSloped, const GeoOptions& options)
{
  const bool bFlipWinding = options.IsFlipWindingNecessary();

  curvature = ezMath::Clamp(curvature, -ezAngle::MakeFromDegree(360), ezAngle::MakeFromDegree(360));
  const ezAngle curveStep = curvature / (float)uiNumSteps;

  const float fStepDiv = 1.0f / uiNumSteps;
  const float fStepDepth = vSize.x / uiNumSteps;
  const float fStepHeight = vSize.z / uiNumSteps;

  ezVec3 vMoveFwd(fStepDepth, 0, 0);
  const ezVec3 vMoveUp(0, 0, fStepHeight);
  ezVec3 vMoveUpFwd(fStepDepth, 0, fStepHeight);

  ezVec3 vBaseL0(-vSize.x * 0.5f, -vSize.y * 0.5f, -vSize.z * 0.5f);
  ezVec3 vBaseL1(-vSize.x * 0.5f, +vSize.y * 0.5f, -vSize.z * 0.5f);
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

  ezQuat qRot = ezQuat::MakeFromAxisAndAngle(ezVec3(0, 0, 1), curveStep);

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
    poly[0] = AddVertex(options, vTopL0, ezVec3(0, 0, 1), ezVec2(fTexU0, 0));
    poly[3] = AddVertex(options, vTopL1, ezVec3(0, 0, 1), ezVec2(fTexU0, 1));
    poly[1] = AddVertex(options, vTopR0, ezVec3(0, 0, 1), ezVec2(fTexU1, 0));
    poly[2] = AddVertex(options, vTopR1, ezVec3(0, 0, 1), ezVec2(fTexU1, 1));
    AddPolygon(poly, bFlipWinding);

    // bottom
    poly[0] = AddVertex(options, vBaseL0, ezVec3(0, 0, -1), ezVec2(fTexU0, 0));
    poly[1] = AddVertex(options, vBaseL1, ezVec3(0, 0, -1), ezVec2(fTexU0, 1));
    poly[3] = AddVertex(options, vBaseR0, ezVec3(0, 0, -1), ezVec2(fTexU1, 0));
    poly[2] = AddVertex(options, vBaseR1, ezVec3(0, 0, -1), ezVec2(fTexU1, 1));
    AddPolygon(poly, bFlipWinding);

    // step front
    if (!bSmoothSloped)
    {
      poly[0] = AddVertex(options, vPrevTopR0, ezVec3(-1, 0, 0), ezVec2(0, fTexU0));
      poly[3] = AddVertex(options, vPrevTopR1, ezVec3(-1, 0, 0), ezVec2(1, fTexU0));
      poly[1] = AddVertex(options, vTopL0, ezVec3(-1, 0, 0), ezVec2(0, fTexU1));
      poly[2] = AddVertex(options, vTopL1, ezVec3(-1, 0, 0), ezVec2(1, fTexU1));
      AddPolygon(poly, bFlipWinding);
    }

    // side 1
    poly[0] = AddVertex(options, vBaseL0, -vSideNormal0, ezVec2(fTexU0, 0));
    poly[1] = AddVertex(options, vBaseR0, -vSideNormal1, ezVec2(fTexU1, 0));
    poly[3] = AddVertex(options, vTopL0, -vSideNormal0, ezVec2(fTexU0, fTexU1));
    poly[2] = AddVertex(options, vTopR0, -vSideNormal1, ezVec2(fTexU1, fTexU1));
    AddPolygon(poly, bFlipWinding);

    // side 2
    poly[0] = AddVertex(options, vBaseL1, vSideNormal0, ezVec2(fTexU0, 0));
    poly[3] = AddVertex(options, vBaseR1, vSideNormal1, ezVec2(fTexU1, 0));
    poly[1] = AddVertex(options, vTopL1, vSideNormal0, ezVec2(fTexU0, fTexU1));
    poly[2] = AddVertex(options, vTopR1, vSideNormal1, ezVec2(fTexU1, fTexU1));
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
    poly[0] = AddVertex(options, vBaseL0, -vStepFrontNormal, ezVec2(0, 0));
    poly[1] = AddVertex(options, vBaseL1, -vStepFrontNormal, ezVec2(1, 0));
    poly[3] = AddVertex(options, vPrevTopR0, -vStepFrontNormal, ezVec2(0, 1));
    poly[2] = AddVertex(options, vPrevTopR1, -vStepFrontNormal, ezVec2(1, 1));
    AddPolygon(poly, bFlipWinding);
  }
}

void ezGeometry::AddArch(const ezVec3& vSize0, ezUInt32 uiNumSegments, float fThickness, ezAngle angle, bool bMakeSteps, bool bSmoothBottom, bool bSmoothTop, bool bCapTopAndBottom, const GeoOptions& options)
{
  const ezQuat tilt = ezBasisAxis::GetBasisRotation(options.m_MainAxis, ezBasisAxis::PositiveZ);
  const ezMat4 trans = options.m_Transform * tilt.GetAsMat4();

  const ezVec3 vSize = tilt * vSize0;

  // sanitize input values
  {
    if (angle.GetRadian() == 0.0f)
      angle = ezAngle::MakeFromDegree(360);

    angle = ezMath::Clamp(angle, ezAngle::MakeFromDegree(-360.0f), ezAngle::MakeFromDegree(360.0f));

    fThickness = ezMath::Clamp(fThickness, 0.01f, ezMath::Min(vSize.x, vSize.y) * 0.45f);

    bSmoothBottom = bMakeSteps && bSmoothBottom;
    bSmoothTop = bMakeSteps && bSmoothTop;
  }

  bool bFlipWinding = options.IsFlipWindingNecessary();

  if (angle.GetRadian() < 0)
    bFlipWinding = !bFlipWinding;

  const ezAngle angleStep = angle / (float)uiNumSegments;
  const float fScaleX = vSize.x * 0.5f;
  const float fScaleY = vSize.y * 0.5f;
  const float fHalfHeight = vSize.z * 0.5f;
  const float fStepHeight = vSize.z / (float)uiNumSegments;

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

  const bool isFullCircle = ezMath::Abs(angle.GetRadian()) >= ezAngle::MakeFromDegree(360).GetRadian();

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
      poly[0] = AddVertex(trans, options, vCurBottomOuter, vCurDirOutwards, ezVec2(fCurOuterU, 0));
      poly[1] = AddVertex(trans, options, vNextBottomOuter, vNextDirOutwards, ezVec2(fNextOuterU, 0));
      poly[3] = AddVertex(trans, options, vCurTopOuter, vCurDirOutwards, ezVec2(fCurOuterU, 1));
      poly[2] = AddVertex(trans, options, vNextTopOuter, vNextDirOutwards, ezVec2(fNextOuterU, 1));
      AddPolygon(poly, bFlipWinding);
    }

    // Inside
    {
      poly[0] = AddVertex(trans, options, vCurBottomInner, -vCurDirOutwards, ezVec2(fCurOuterU, 0));
      poly[3] = AddVertex(trans, options, vNextBottomInner, -vNextDirOutwards, ezVec2(fNextOuterU, 0));
      poly[1] = AddVertex(trans, options, vCurTopInner, -vCurDirOutwards, ezVec2(fCurOuterU, 1));
      poly[2] = AddVertex(trans, options, vNextTopInner, -vNextDirOutwards, ezVec2(fNextOuterU, 1));
      AddPolygon(poly, bFlipWinding);
    }

    // Bottom
    if (bCapTopAndBottom)
    {
      poly[0] = AddVertex(trans, options, vCurBottomInner, ezVec3(0, 0, -1), vCurBottomInner.GetAsVec2());
      poly[1] = AddVertex(trans, options, vNextBottomInner, ezVec3(0, 0, -1), vNextBottomInner.GetAsVec2());
      poly[3] = AddVertex(trans, options, vCurBottomOuter, ezVec3(0, 0, -1), vCurBottomOuter.GetAsVec2());
      poly[2] = AddVertex(trans, options, vNextBottomOuter, ezVec3(0, 0, -1), vNextBottomOuter.GetAsVec2());
      AddPolygon(poly, bFlipWinding);
    }

    // Top
    if (bCapTopAndBottom)
    {
      poly[0] = AddVertex(trans, options, vCurTopInner, ezVec3(0, 0, 1), vCurTopInner.GetAsVec2());
      poly[3] = AddVertex(trans, options, vNextTopInner, ezVec3(0, 0, 1), vNextTopInner.GetAsVec2());
      poly[1] = AddVertex(trans, options, vCurTopOuter, ezVec3(0, 0, 1), vCurTopOuter.GetAsVec2());
      poly[2] = AddVertex(trans, options, vNextTopOuter, ezVec3(0, 0, 1), vNextTopOuter.GetAsVec2());
      AddPolygon(poly, bFlipWinding);
    }

    // Front
    if (bMakeSteps || (!isFullCircle && segment == 0))
    {
      const ezVec3 vNormal = (bFlipWinding ? -1.0f : 1.0f) * vCurDirOutwards.CrossRH(ezVec3(0, 0, 1));
      poly[0] = AddVertex(trans, options, vCurBottomInner, vNormal, ezVec2(0, 0));
      poly[1] = AddVertex(trans, options, vCurBottomOuter, vNormal, ezVec2(1, 0));
      poly[3] = AddVertex(trans, options, vCurTopInner, vNormal, ezVec2(0, 1));
      poly[2] = AddVertex(trans, options, vCurTopOuter, vNormal, ezVec2(1, 1));
      AddPolygon(poly, bFlipWinding);
    }

    // Back
    if (bMakeSteps || (!isFullCircle && segment == uiNumSegments - 1))
    {
      const ezVec3 vNormal = (bFlipWinding ? -1.0f : 1.0f) * -vNextDirOutwards.CrossRH(ezVec3(0, 0, 1));
      poly[0] = AddVertex(trans, options, vNextBottomInner, vNormal, ezVec2(0, 0));
      poly[3] = AddVertex(trans, options, vNextBottomOuter, vNormal, ezVec2(1, 0));
      poly[1] = AddVertex(trans, options, vNextTopInner, vNormal, ezVec2(0, 1));
      poly[2] = AddVertex(trans, options, vNextTopOuter, vNormal, ezVec2(1, 1));
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
