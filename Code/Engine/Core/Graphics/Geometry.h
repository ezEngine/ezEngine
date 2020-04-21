#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Math/Color.h>
#include <Foundation/Math/Mat4.h>
#include <Foundation/Math/Vec3.h>

/// \brief Provides functions to generate standard geometric shapes, such as boxes, spheres, cylinders, etc.
///
/// This class provides simple functions to create frequently used basic shapes. It allows to transform the shapes, merge them
/// into a single mesh, compute normals, etc.
/// It is meant for debug and editor geometry (gizmos, etc.). Vertices can have position, normal, color and 'shape index'.
class EZ_CORE_DLL ezGeometry
{
public:
  /// \brief The data that is stored per vertex.
  struct Vertex
  {
    EZ_DECLARE_POD_TYPE();

    ezVec3 m_vPosition;
    ezVec3 m_vNormal;
    ezVec3 m_vTangent;
    float m_fBiTangentSign;
    ezVec2 m_vTexCoord;
    ezColor m_Color;
    ezInt32 m_iCustomIndex;

    bool operator<(const Vertex& rhs) const;
    bool operator==(const Vertex& rhs) const;
  };

  /// \brief Each polygon has a face normal and a set of indices, which vertices it references.
  struct Polygon
  {
    // Reverses the order of vertices.
    void FlipWinding();

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
  ezUInt32 AddVertex(const ezVec3& vPos, const ezVec3& vNormal, const ezVec2& vTexCoord, const ezColor& color, ezInt32 iCustomIndex = 0);

  /// \brief Adds a vertex, returns the index to the added vertex. Position and normal are transformed with the given matrix.
  ezUInt32 AddVertex(const ezVec3& vPos, const ezVec3& vNormal, const ezVec2& vTexCoord, const ezColor& color, ezInt32 iCustomIndex,
                     const ezMat4& mTransform);

  /// \brief Adds a polygon that consists of all the referenced vertices. No face normal is computed at this point.
  void AddPolygon(const ezArrayPtr<ezUInt32>& Vertices, bool bFlipWinding);

  /// \brief Adds a line.
  void AddLine(ezUInt32 uiStartVertex, ezUInt32 uiEndVertex);

  /// \brief Triangulates all polygons that have more than \a uiMaxVerticesInPolygon vertices.
  ///
  /// Set \a uiMaxVerticesInPolygon to 4, if you want to keep quads unchanged.
  void TriangulatePolygons(ezUInt32 uiMaxVerticesInPolygon = 3);

  /// \brief Computes normals for all polygons from the current vertex positions. Call this when you do not intend to make further
  /// modifications.
  void ComputeFaceNormals();

  /// \brief Computes smooth (averaged) normals for each vertex. Requires that face normals are computed.
  ///
  /// This only yields smooth normals for vertices that are shared among multiple polygons, otherwise a vertex will have the same normal
  /// as the one face that it is used in.
  void ComputeSmoothVertexNormals();

  /// \brief Computes tangents. This function can increase or reduce vertex count.
  ///
  /// The tangent generation is done by Morten S. Mikkelsen's tangent space generation code.
  void ComputeTangents();

  /// \brief Checks if present tangents are meaningful and resetting them if necessary
  ///
  /// Checks if the tangents are approximately orthogonal to the vertex normal and
  /// of unit length. If this is not the case the respective tangent will be zeroed.
  /// The caller can provide a custom floating point comparison epsilon
  void ValidateTangents(float epsilon = 0.01f);

  /// \brief Returns the number of triangles that the polygons are made up of
  ezUInt32 CalculateTriangleCount() const;

  /// \brief Changes the custom index for all vertices (starting at vertex \a uiFirstVertex).
  void SetAllVertexCustomIndex(ezInt32 iCustomIndex, ezUInt32 uiFirstVertex = 0);

  /// \brief Changes the color for all vertices (starting at vertex \a uiFirstVertex).
  void SetAllVertexColor(const ezColor& color, ezUInt32 uiFirstVertex = 0);

  /// \brief Changes the texture coordinates for all vertices (starting at vertex \a uiFirstVertex).
  void SetAllVertexTexCoord(const ezVec2& texCoord, ezUInt32 uiFirstVertex = 0);

  /// \brief Transforms all vertices by the given transform.
  ///
  /// When \a bTransformPolyNormals is true, the polygon normals are transformed, as well.
  /// Set this to false when face normals are going to be computed later anyway.
  void Transform(const ezMat4& mTransform, bool bTransformPolyNormals);

  /// \brief Merges the given mesh into this one. Use this to composite multiple shapes into one.
  void Merge(const ezGeometry& other);

  /// \brief Adds a rectangle shape in the XY plane, with the front in positive Z direction.
  /// It is centered at the origin, extending half size.x and half size.y into direction +X, -X, +Y and -Y.
  void AddRectXY(const ezVec2& size, const ezColor& color, const ezMat4& mTransform = ezMat4::IdentityMatrix(), ezInt32 iCustomIndex = 0);

  /// \brief Same as AddRectXY but additionally tessellates the plane. Tessellation factors must be larger than zero.
  void AddTesselatedRectXY(const ezVec2& size, const ezColor& color, ezUInt32 uiTesselationX, ezUInt32 uiTesselationY, const ezMat4& mTransform = ezMat4::IdentityMatrix(), ezInt32 iCustomIndex = 0);

  /// \brief Adds an untextured box (8 vertices).
  void AddBox(const ezVec3& size, const ezColor& color, const ezMat4& mTransform = ezMat4::IdentityMatrix(), ezInt32 iCustomIndex = 0);

  /// \brief Adds box out of lines (8 vertices).
  void AddLineBox(const ezVec3& size, const ezColor& color, const ezMat4& mTransform = ezMat4::IdentityMatrix(), ezInt32 iCustomIndex = 0);

  /// \brief Adds the 8 corners of a box as lines.
  ///
  /// fCornerFraction must be between 1.0 and 0.0, with 1 making it a completely closed box and 0 no lines at all.
  void AddLineBoxCorners(const ezVec3& size, float fCornerFraction, const ezColor& color,
                         const ezMat4& mTransform = ezMat4::IdentityMatrix(), ezInt32 iCustomIndex = 0);

  /// \brief Adds a box that has UV coordinates set (24 vertices).
  void AddTexturedBox(const ezVec3& size, const ezColor& color, const ezMat4& mTransform = ezMat4::IdentityMatrix(),
                      ezInt32 iCustomIndex = 0);

  /// \brief Adds a pyramid. This is different to a low-res cone in that the corners are placed differently (like on a box).
  ///
  /// The origin is at the center of the base quad.size.z is the height of the pyramid.
  void AddPyramid(const ezVec3& size, bool bCap, const ezColor& color, const ezMat4& mTransform = ezMat4::IdentityMatrix(),
                  ezInt32 iCustomIndex = 0);

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
  void AddGeodesicSphere(float fRadius, ezUInt8 uiSubDivisions, const ezColor& color, const ezMat4& mTransform = ezMat4::IdentityMatrix(),
                         ezInt32 iCustomIndex = 0);

  /// \brief Adds a cylinder.
  ///
  /// If fPositiveLength == fNegativeLength, the origin is at the center.
  /// If fNegativeLength is zero, the origin is at the bottom and so on.
  ///
  /// uiSegments is the detail around the up axis, must be at least 3.
  /// The top or bottom caps can be removed using \a bCapTop and \a bCapBottom.
  /// When \a fraction is set to any value below 360 degree, a pie / pacman shaped cylinder is created.
  void AddCylinder(float fRadiusTop, float fRadiusBottom, float fPositiveLength, float fNegativeLength, bool bCapTop, bool bCapBottom,
                   ezUInt16 uiSegments, const ezColor& color, const ezMat4& mTransform = ezMat4::IdentityMatrix(), ezInt32 iCustomIndex = 0,
                   ezAngle fraction = ezAngle::Degree(360.0f));

  /// \brief Same as AddCylinder(), but always adds caps and does not generate separate vertices for the caps.
  ///
  /// This is a more compact representation, but does not allow as good texturing.
  void AddCylinderOnePiece(float fRadiusTop, float fRadiusBottom, float fPositiveLength, float fNegativeLength, ezUInt16 uiSegments,
                           const ezColor& color, const ezMat4& mTransform = ezMat4::IdentityMatrix(), ezInt32 iCustomIndex = 0);

  /// \brief Adds a cone. The origin is at the center of the bottom.
  ///
  /// uiSegments is the detail around the up axis, must be at least 3.
  void AddCone(float fRadius, float fHeight, bool bCap, ezUInt16 uiSegments, const ezColor& color,
               const ezMat4& mTransform = ezMat4::IdentityMatrix(), ezInt32 iCustomIndex = 0);

  /// \brief Adds a sphere.
  ///
  /// uiSegments is the detail around the up axis, must be at least 3.
  /// uiStacks is the detail of the rounded top and bottom, must be at least 2.
  void AddSphere(float fRadius, ezUInt16 uiSegments, ezUInt16 uiStacks, const ezColor& color,
                 const ezMat4& mTransform = ezMat4::IdentityMatrix(), ezInt32 iCustomIndex = 0);

  /// \brief Adds half a sphere.
  ///
  /// The origin is at the 'full sphere center', ie. at the center of the cap.
  /// uiSegments is the detail around the up axis, must be at least 3.
  /// uiStacks is the detail of the rounded top and bottom, must be at least 1.
  void AddHalfSphere(float fRadius, ezUInt16 uiSegments, ezUInt16 uiStacks, bool bCap, const ezColor& color,
                     const ezMat4& mTransform = ezMat4::IdentityMatrix(), ezInt32 iCustomIndex = 0);

  /// \brief Adds a capsule.
  ///
  /// The origin is at the center of the capsule.
  /// Radius and height are added to get the total height of the capsule.
  /// uiSegments is the detail around the up axis, must be at least 3.
  /// uiStacks is the detail of the rounded top and bottom, must be at least 1.
  void AddCapsule(float fRadius, float fHeight, ezUInt16 uiSegments, ezUInt16 uiStacks, const ezColor& color,
                  const ezMat4& mTransform = ezMat4::IdentityMatrix(), ezInt32 iCustomIndex = 0);

  /// \brief Adds a full torus.
  ///
  /// The origin is at the center of the torus.
  /// fInnerRadius is the radius from the center to where the torus ring starts.
  /// fOuterRadius is the radius to where the torus ring stops.
  /// uiSegments is the detail around the up axis.
  /// uiSegmentDetail is the number of segments around the torus ring (ie. the cylinder detail)
  void AddTorus(float fInnerRadius, float fOuterRadius, ezUInt16 uiSegments, ezUInt16 uiSegmentDetail, const ezColor& color,
                const ezMat4& mTransform = ezMat4::IdentityMatrix(), ezInt32 iCustomIndex = 0);

  /// \brief Adds a ramp that has UV coordinates set.
  void AddTexturedRamp(const ezVec3& size, const ezColor& color, const ezMat4& mTransform = ezMat4::IdentityMatrix(),
                       ezInt32 iCustomIndex = 0);

  /// \brief Generates a straight stair mesh along the X axis. The number of steps determines the step height and depth.
  void AddStairs(const ezVec3& size, ezUInt32 uiNumSteps, ezAngle curvature, bool bSmoothSloped, const ezColor& color,
                 const ezMat4& mTransform = ezMat4::IdentityMatrix(), ezInt32 iCustomIndex = 0);

  void AddArch(const ezVec3& size, ezUInt32 uiNumSegments, float fThickness, ezAngle angle, bool bMakeSteps, bool bSmoothBottom,
               bool bSmoothTop, const ezColor& color, const ezMat4& mTransform = ezMat4::IdentityMatrix(), ezInt32 iCustomIndex = 0);

  /// \todo GeomUtils improvements:
  // ThickLine
  // Part of a Torus
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

