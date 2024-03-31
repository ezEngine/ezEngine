#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Math/Color.h>
#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Math/Declarations.h>
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
    ezVec4U16 m_BoneIndices;
    ezColorLinearUB m_BoneWeights;

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

  /// \brief Options shared among all geometry creation functions
  struct GeoOptions
  {
    EZ_DECLARE_POD_TYPE();

    GeoOptions() {}                                        // NOLINT: This struct is used before the surrounding class ends, so it needs a default constructor. = default doesn't work here.

    ezBasisAxis::Enum m_MainAxis = ezBasisAxis::PositiveZ; ///< Used by some geometry as a reference direction.
    ezColor m_Color = ezColor(1, 1, 1, 1);                 ///< The color of the entire geometric object
    ezMat4 m_Transform = ezMat4::MakeIdentity();           ///< An additional transform to apply to the geometry while adding it
    ezUInt16 m_uiBoneIndex = 0;                            ///< Which bone should influence this geometry, for single-bone skinning.

    bool IsFlipWindingNecessary() const;
  };

  /// \brief Returns the entire vertex data.
  ezDeque<Vertex>& GetVertices() { return m_Vertices; }

  /// \brief Returns the entire polygon data.
  ezDeque<Polygon>& GetPolygons() { return m_Polygons; }

  /// \brief Returns the entire line data.
  ezDeque<Line>& GetLines() { return m_Lines; }

  /// \brief Returns the entire vertex data.
  const ezDeque<Vertex>& GetVertices() const { return m_Vertices; }

  /// \brief Returns the entire polygon data.
  const ezDeque<Polygon>& GetPolygons() const { return m_Polygons; }

  /// \brief Returns the entire line data.
  const ezDeque<Line>& GetLines() const { return m_Lines; }

  /// \brief Clears all data.
  void Clear();

  /// \brief Adds a vertex, returns the index to the added vertex.
  ezUInt32 AddVertex(const ezVec3& vPos, const ezVec3& vNormal, const ezVec2& vTexCoord = ezVec2(0.0f), const ezColor& color = ezColor::White, const ezVec4U16& vBoneIndices = ezVec4U16::MakeZero(), const ezColorLinearUB& boneWeights = ezColorLinearUB(255, 0, 0, 0));

  /// \brief Overload that transforms position and normal with the given matrix.
  ezUInt32 AddVertex(const ezMat4& mTransform, const ezVec3& vPos, const ezVec3& vNormal, const ezVec2& vTexCoord = ezVec2(0.0f), const ezColor& color = ezColor::White, const ezVec4U16& vBoneIndices = ezVec4U16::MakeZero(), const ezColorLinearUB& boneWeights = ezColorLinearUB(255, 0, 0, 0))
  {
    return AddVertex(mTransform.TransformPosition(vPos), mTransform.TransformDirection(vNormal).GetNormalized(), vTexCoord, color, vBoneIndices, boneWeights);
  }

  /// \brief Overload that uses the options for color and to transform position and normal and uses a single bone.
  ezUInt32 AddVertex(const GeoOptions& options, const ezVec3& vPos, const ezVec3& vNormal, const ezVec2& vTexCoord = ezVec2(0.0f))
  {
    return AddVertex(options.m_Transform, vPos, vNormal, vTexCoord, options.m_Color, ezVec4U16(options.m_uiBoneIndex, 0, 0, 0), ezColorLinearUB(255, 0, 0, 0));
  }

  /// \brief Overload that uses the options for color and a single bone and transforms position and normal by a separately provided matrix.
  ezUInt32 AddVertex(const ezMat4& mTransform, const GeoOptions& options, const ezVec3& vPos, const ezVec3& vNormal, const ezVec2& vTexCoord = ezVec2(0.0f))
  {
    return AddVertex(mTransform, vPos, vNormal, vTexCoord, options.m_Color, ezVec4U16(options.m_uiBoneIndex, 0, 0, 0), ezColorLinearUB(255, 0, 0, 0));
  }

  /// \brief Adds a polygon that consists of all the referenced vertices. No face normal is computed at this point.
  void AddPolygon(const ezArrayPtr<ezUInt32>& vertices, bool bFlipWinding);

  /// \brief Adds a line with the given start and end vertex index.
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

  /// \brief Checks whether present tangents are meaningful and resets them, if necessary
  ///
  /// Checks if the tangents are approximately orthogonal to the vertex normal and
  /// of unit length. If this is not the case the respective tangent will be zeroed.
  /// The caller can provide a comparison epsilon.
  void ValidateTangents(float fEpsilon = 0.01f);

  /// \brief Returns the number of triangles that the polygons are made up of.
  ezUInt32 CalculateTriangleCount() const;

  /// \brief Changes the bone indices for all vertices (starting at vertex \a uiFirstVertex).
  void SetAllVertexBoneIndices(const ezVec4U16& vBoneIndices, ezUInt32 uiFirstVertex = 0);

  /// \brief Changes the color for all vertices (starting at vertex \a uiFirstVertex).
  void SetAllVertexColor(const ezColor& color, ezUInt32 uiFirstVertex = 0);

  /// \brief Changes the texture coordinates for all vertices (starting at vertex \a uiFirstVertex).
  void SetAllVertexTexCoord(const ezVec2& vTexCoord, ezUInt32 uiFirstVertex = 0);

  /// \brief Transforms all vertices by the given transform.
  ///
  /// When \a bTransformPolyNormals is true, the polygon normals are transformed, as well.
  /// Set this to false when face normals are going to be computed later anyway.
  void Transform(const ezMat4& mTransform, bool bTransformPolyNormals);

  /// \brief Merges the given mesh into this one. Use this to composite multiple shapes into one.
  void Merge(const ezGeometry& other);

  /// \brief Adds a rectangle shape, with the front pointing into the main axis direction.
  ///
  /// It is centered at the origin, extending half size.x and half size.y into direction +X, -X, +Y and -Y.
  /// Optionally tessellates the rectangle for more detail.
  void AddRect(const ezVec2& vSize, ezUInt32 uiTesselationX = 1, ezUInt32 uiTesselationY = 1, const GeoOptions& options = GeoOptions());

  /// \brief Adds a box.
  /// If bExtraVerticesForTexturing is false, 8 shared vertices are added.
  /// If bExtraVerticesForTexturing is true, 24 separate vertices with UV coordinates are added.
  void AddBox(const ezVec3& vFullExtents, bool bExtraVerticesForTexturing, const GeoOptions& options = GeoOptions());

  /// \brief Adds box out of lines (8 vertices).
  void AddLineBox(const ezVec3& vSize, const GeoOptions& options = GeoOptions());

  /// \brief Adds the 8 corners of a box as lines.
  ///
  /// fCornerFraction must be between 1.0 and 0.0, with 1 making it a completely closed box and 0 no lines at all.
  void AddLineBoxCorners(const ezVec3& vSize, float fCornerFraction, const GeoOptions& options = GeoOptions());

  /// \brief Adds a pyramid. This is different to a low-res cone in that the corners are placed differently (like on a box).
  ///
  /// The origin is at the center of the base quad and the tip is in the direction of the main axis (see GeoOptions).
  void AddPyramid(float fBaseSize, float fHeight, bool bCap, const GeoOptions& options = GeoOptions());

  /// \brief Adds a geodesic sphere at the origin.
  ///
  /// When uiSubDivisions is zero, the sphere will have 20 triangles.\n
  /// For each subdivision step the number of triangles quadruples.\n
  /// 0 =   20 triangles,   12 vertices\n
  /// 1 =   80 triangles,   42 vertices\n
  /// 2 =  320 triangles,  162 vertices\n
  /// 3 = 1280 triangles,  642 vertices\n
  /// 4 = 5120 triangles, 2562 vertices\n
  /// ...\n
  void AddGeodesicSphere(float fRadius, ezUInt8 uiSubDivisions, const GeoOptions& options = GeoOptions());

  /// \brief Adds a cylinder revolving around the main axis (see GeoOptions).
  ///
  /// If fPositiveLength == fNegativeLength, the origin is at the center.
  /// If fNegativeLength is zero, the origin is at the bottom and so on.
  ///
  /// uiSegments is the detail around the up axis, must be at least 3.
  /// The top or bottom caps can be removed using \a bCapTop and \a bCapBottom.
  /// When \a fraction is set to any value below 360 degree, a pie / PacMan-shaped cylinder is created.
  void AddCylinder(float fRadiusTop, float fRadiusBottom, float fPositiveLength, float fNegativeLength, bool bCapTop, bool bCapBottom, ezUInt16 uiSegments, const GeoOptions& options = GeoOptions(), ezAngle fraction = ezAngle::MakeFromDegree(360.0f));

  /// \brief Same as AddCylinder(), but always adds caps and does not generate separate vertices for the caps.
  ///
  /// This is a more compact representation, but does not allow as good texturing.
  void AddCylinderOnePiece(float fRadiusTop, float fRadiusBottom, float fPositiveLength, float fNegativeLength, ezUInt16 uiSegments, const GeoOptions& options = GeoOptions());

  /// \brief Adds a cone with the origin at the center of the bottom and the tip pointing into the direction of the main axis (see GeoOptions).
  ///
  /// uiSegments is the detail around the up axis, must be at least 3.
  void AddCone(float fRadius, float fHeight, bool bCap, ezUInt16 uiSegments, const GeoOptions& options = GeoOptions());

  /// \brief Adds a sphere consisting of a number of stacks along the main axis (see GeoOptions) with a fixed tessellation.
  ///
  /// uiSegments is the detail around the up axis, must be at least 3.
  /// uiStacks is the detail along the up axis, must be at least 2.
  void AddStackedSphere(float fRadius, ezUInt16 uiSegments, ezUInt16 uiStacks, const GeoOptions& options = GeoOptions());

  /// \brief Adds half a stacked sphere with the half being in the direction of the main axis (see GeoOptions).
  ///
  /// The origin is at the 'full sphere center', ie. at the center of the cap.
  /// uiSegments is the detail around the up axis, must be at least 3.
  /// uiStacks is the detail of the rounded top and bottom, must be at least 1.
  void AddHalfSphere(float fRadius, ezUInt16 uiSegments, ezUInt16 uiStacks, bool bCap, const GeoOptions& options = GeoOptions());

  /// \brief Adds a capsule, revolving around the main axis (see GeoOptions).
  ///
  /// The origin is at the center of the capsule.
  /// Radius and height are added to get the total height of the capsule.
  /// uiSegments is the detail around the up axis, must be at least 3.
  /// uiStacks is the detail of the rounded top and bottom, must be at least 1.
  void AddCapsule(float fRadius, float fHeight, ezUInt16 uiSegments, ezUInt16 uiStacks, const GeoOptions& options = GeoOptions());

  /// \brief Adds a full torus with the ring revolving around the main axis (see GeoOptions).
  ///
  /// The origin is at the center of the torus.
  /// \param fInnerRadius is the radius from the center to where the torus ring starts.
  /// \param fOuterRadius is the radius to where the torus ring stops.
  /// \param uiSegments is the detail around the main axis.
  /// \param uiSegmentDetail is the number of segments around the torus ring (ie. the cylinder detail)
  /// \param bExtraVerticesForTexturing specifies whether the torus should be one closed piece or have additional vertices at the seams, such that texturing works better.
  void AddTorus(float fInnerRadius, float fOuterRadius, ezUInt16 uiSegments, ezUInt16 uiSegmentDetail, bool bExtraVerticesForTexturing, const GeoOptions& options = GeoOptions());

  /// \brief Adds a ramp that has UV coordinates set.
  void AddTexturedRamp(const ezVec3& vSize, const GeoOptions& options = GeoOptions());

  /// \brief Generates a straight stair mesh along the X axis. The number of steps determines the step height and depth.
  void AddStairs(const ezVec3& vSize, ezUInt32 uiNumSteps, ezAngle curvature, bool bSmoothSloped, const GeoOptions& options = GeoOptions());

  /// \brief Creates and arch, pipe or spiral stairs within the defined volume (size) curving around the main axis.
  ///
  /// \param uiNumSegments The number of segments of the arch or pipe, or the number of steps in spiral stairs.
  /// \param fThickness The wall thickness of the pipe, or the width of steps.
  /// \param angle How far to curl around the main axis. Values between -360 degree and +360 degree are possible. 0 degree is treated like 360 degree.
  /// \param bMakeSteps If true, segments are only thin and are offset in position from the previous one.
  /// \param bSmoothBottom, bSmoothTop If true and bMakeSteps as well, the segments will have a step, but connect smoothly, like in a spiral.
  /// \param bCapTopAndBottom If false, the top and bottom geometry is skipped. Can be used to reduce detail in pipe geometry that connects with something else anyway.
  void AddArch(const ezVec3& vSize, ezUInt32 uiNumSegments, float fThickness, ezAngle angle, bool bMakeSteps, bool bSmoothBottom, bool bSmoothTop, bool bCapTopAndBottom, const GeoOptions& options = GeoOptions());

private:
  void TransformVertices(const ezMat4& mTransform, ezUInt32 uiFirstVertex);

  ezDeque<Vertex> m_Vertices;
  ezDeque<Polygon> m_Polygons;
  ezDeque<Line> m_Lines;
};
