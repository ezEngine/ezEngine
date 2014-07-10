#pragma once

#include <CoreUtils/Basics.h>

class EZ_COREUTILS_DLL ezGeometry
{
public:

  struct Vertex
  {
    EZ_DECLARE_POD_TYPE();

    ezVec3 m_vPosition;
    ezVec3 m_vNormal;
    ezColor8UNorm m_Color;
    ezInt32 m_iCustomIndex;
  };

  struct Polygon
  {
    EZ_DECLARE_POD_TYPE();

    ezVec3 m_vNormal;
    ezHybridArray<ezUInt32, 4> m_Vertices;
  };

  struct Line
  {
    EZ_DECLARE_POD_TYPE();

    ezUInt32 m_uiStartVertex;
    ezUInt32 m_uiEndVertex;
  };

  const ezDeque<Vertex>& GetVertices() const { return m_Vertices; }
  const ezDeque<Polygon>& GetPolygons() const { return m_Polygons; }
  const ezDeque<Line>& GetLines() const { return m_Lines; }

  void Clear();

  ezUInt32 AddVertex(const ezVec3& vPos, const ezVec3& vNormal, const ezColor8UNorm& color, ezInt32 iCustomIndex = 0);
  ezUInt32 AddVertex(const ezVec3& vPos, const ezVec3& vNormal, const ezColor8UNorm& color, ezInt32 iCustomIndex, const ezMat4& mTransform);

  void AddPolygon(const ezArrayPtr<ezUInt32>& Vertices);

  void AddLine(ezUInt32 uiStartVertex, ezUInt32 uiEndVertex);

  void ComputeFaceNormals();
  void ComputeSmoothVertexNormals();

  void SetAllVertexCustomIndex(ezInt32 iCustomIndex);
  void SetAllVertexColor(const ezColor8UNorm& color);
  void Transform(const ezMat4& mTransform, bool bTransformPolyNormals);

  void Merge(const ezGeometry& other);

  void AddRectXY(const ezColor8UNorm& color, const ezMat4& mTransform = ezMat4::IdentityMatrix(), ezInt32 iCustomIndex = 0);

  // Sphere
  // GeodesicSphere
  // Box
  // Pyramid
  // Cone
  // Cylinder
  // Capsule
  // ThickLine
  // Torus
  // Arc
  // Circle
  // Curved cone (spotlight)
  // flat arc / circle (ie. UE4 gizmo)
  // ....

  // Compounds:
  // Arrow
  // Cross ?
  //


private:

  ezDeque<Vertex> m_Vertices;
  ezDeque<Polygon> m_Polygons;
  ezDeque<Line> m_Lines;
};

