#pragma once

#include <CoreUtils/Basics.h>

/// \brief Provides functions to generate standard geometric shapes, such as boxes, spheres, cylinders, etc.
///
/// This class provides simple functions to create frequently used basic shapes. It allows to transform the shapes, merge them
/// into a single mesh, compute normals, etc.
/// It is meant for debug and editor geometry (gizmos etc.). Vertices can have position, normal, color and 'shape index'.
/// UV coordinates are currently not supported.
class EZ_COREUTILS_DLL ezGeometry
{
public:

  /// \brief The data that is stored per vertex.
  struct Vertex
  {
    EZ_DECLARE_POD_TYPE();

    ezVec3 m_vPosition;
    ezVec3 m_vNormal;
    ezColor8UNorm m_Color;
    ezInt32 m_iCustomIndex;
  };

  /// \brief Each polygon has a face normal and a set of indices, which vertices it references.
  struct Polygon
  {
    EZ_DECLARE_POD_TYPE();

    ezVec3 m_vNormal;
    ezHybridArray<ezUInt32, 4> m_Vertices;
  };

  /// \brief A line only references two vertices.
  struct Line
  {
    EZ_DECLARE_POD_TYPE();

    ezUInt32 m_uiStartVertex;
    ezUInt32 m_uiEndVertex;
  };

  /// \brief Returns the entire vertex data.
  const ezDeque<Vertex>& GetVertices() const { return m_Vertices; }

  /// \brief Returns the entire polygon data.
  const ezDeque<Polygon>& GetPolygons() const { return m_Polygons; }

  /// \brief Returns the entire line data.
  const ezDeque<Line>& GetLines() const { return m_Lines; }

  /// \brief Clears all data.
  void Clear();

  /// \brief Adds a vertex, returns the index to the added vertex.
  ezUInt32 AddVertex(const ezVec3& vPos, const ezVec3& vNormal, const ezColor8UNorm& color, ezInt32 iCustomIndex = 0);

  /// \brief Adds a vertex, returns the index to the added vertex. Position and normal are transformed with the given matrix.
  ezUInt32 AddVertex(const ezVec3& vPos, const ezVec3& vNormal, const ezColor8UNorm& color, ezInt32 iCustomIndex, const ezMat4& mTransform);

  /// \brief Adds a polygon that consists of all the referenced vertices. No face normal is computed at this point.
  void AddPolygon(const ezArrayPtr<ezUInt32>& Vertices);

  /// \brief Adds a line.
  void AddLine(ezUInt32 uiStartVertex, ezUInt32 uiEndVertex);

  /// \brief Computes normals for all polygons from the current vertex positions. Call this when you do not intend to make further modifications.
  void ComputeFaceNormals();

  /// \brief Computes smooth (averaged) normals for each vertex. Requires that face normals are computed.
  ///
  /// This only yields smooth normals for vertices that are shared among multiple polygons, otherwise a vertex will have the same normal
  /// as the one face that it is used in.
  void ComputeSmoothVertexNormals();

  /// \brief Changes the custom index for all vertices (starting at vertex \a uiFirstVertex).
  void SetAllVertexCustomIndex(ezInt32 iCustomIndex, ezUInt32 uiFirstVertex = 0);

  /// \brief Changes the color for all vertices (starting at vertex \a uiFirstVertex).
  void SetAllVertexColor(const ezColor8UNorm& color, ezUInt32 uiFirstVertex = 0);

  /// \brief Transforms all vertices by the given transform.
  ///
  /// When \a bTransformPolyNormals is true, the polygon normals are transformed, as well.
  /// Set this to false when face normals are going to be computed later anyway.
  void Transform(const ezMat4& mTransform, bool bTransformPolyNormals);

  /// \brief Merges the given mesh into this one. Use this to composite multiple shapes into one.
  void Merge(const ezGeometry& other);

  /// \brief Adds a rectangle shape in the XY plane, with the front in positive Z direction.
  /// It is centered at the origin, extending half fSizeX and half fSizeY into direction +X, -X, +Y and -Y.
  void AddRectXY(float fSizeX, float fSizeY, const ezColor8UNorm& color, const ezMat4& mTransform = ezMat4::IdentityMatrix(), ezInt32 iCustomIndex = 0);

  /// \brief Adds a geodesic sphere with radius 1 at the origin.
  ///
  /// When uiSubDivisions is zero, the sphere will have 20 triangles.\n
  /// For each subdivision step the number of triangles quadruples.\n
  /// 0 =   20 triangles,   12 vertices\n
  /// 1 =   80 triangles,   42 vertices\n
  /// 2 =  320 triangles,  162 vertices\n
  /// 3 = 1280 triangles,  642 vertices\n
  /// 4 = 5120 triangles, 2562 vertices\n
  /// ...\n
  void AddGeodesicSphere(ezUInt8 uiSubDivisions, const ezColor8UNorm& color, const ezMat4& mTransform = ezMat4::IdentityMatrix(), ezInt32 iCustomIndex = 0);

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
  void TransformVertices(const ezMat4& mTransform, ezUInt32 uiFirstVertex);

  ezDeque<Vertex> m_Vertices;
  ezDeque<Polygon> m_Polygons;
  ezDeque<Line> m_Lines;
};

