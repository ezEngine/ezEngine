#pragma once

#include <Core/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Containers/Deque.h>

/// \brief Computes convex hulls for 3D meshes.
///
/// By default it will also simplify the result to a reasonable degree,
/// to reduce complexity and vertex/triangle count.
///
/// Currently there is an upper limit of 16384 vertices to accept meshes.
/// Everything larger than that will not be processed.
class EZ_CORE_DLL ezConvexHullGenerator
{
public:
  struct Face
  {
    EZ_DECLARE_POD_TYPE();

    ezUInt16 m_uiVertexIdx[3];
  };

  ezConvexHullGenerator();

  /// \brief Used to remove degenerate and unnecessary triangles that have corners with very little angle change.
  /// Ie. specifying 10 degree, means that all triangle corners must have at least a 10 degree change (and inner angle of 170 degree).
  void SetSimplificationMinTriangleAngle(ezAngle angle) { m_MinTriangleAngle = angle; }

  /// \brief Used to remove vertices that do not contribute much to the silhouette.
  /// Vertices whose adjacent triangle normals do not differ by more than angle, will be discarded.
  void SetSimplificationFlatVertexNormalThreshold(ezAngle angle) { m_FlatVertexNormalThreshold = angle; }

  /// \brief The minimum triangle edge length. Every edge shorter than this will be discarded and replaced by a single vertex at the
  /// average position.
  /// \note The length is not in 'mesh space' coordinates, but instead in 'unit cube space'.
  /// That means, every mesh is scaled to fit into a cube of size [-1; +1] for each axis. Thus the exact scale of the mesh does not matter
  /// when setting this value. Default is 0.05.
  void SetSimplificationMinTriangleEdgeLength(double len) { m_fMinTriangleEdgeLength = len; }

  /// \brief Generates the convex hull. Simplifies the mesh according to the previously specified parameters.
  ezResult Build(const ezArrayPtr<const ezVec3> vertices);

  /// \brief When Build() was successful this can be called to retrieve the resulting vertices and triangles.
  void Retrieve(ezDynamicArray<ezVec3>& out_Vertices, ezDynamicArray<Face>& out_Faces);

  /// \brief Same as Retrieve() but only returns the vertices.
  void RetrieveVertices(ezDynamicArray<ezVec3>& out_Vertices);

private:
  ezResult ComputeCenterAndScale(const ezArrayPtr<const ezVec3> vertices);
  ezResult StoreNormalizedVertices(const ezArrayPtr<const ezVec3> vertices);
  void StoreTriangle(int i, int j, int k);
  ezResult InitializeHull();
  ezResult ComputeHull();
  bool IsInside(ezUInt32 vtxId) const;
  void RemoveVisibleFaces(ezUInt32 vtxId);
  void PatchHole(ezUInt32 vtxId);
  bool PruneFlatVertices(double fNormalThreshold);
  bool PruneDegenerateTriangles(double fMaxAngle);
  bool PruneSmallTriangles(double fMaxEdgeLen);
  ezResult ProcessVertices(const ezArrayPtr<const ezVec3> vertices);

  struct TwoSet
  {
    EZ_ALWAYS_INLINE TwoSet() { a = 0xFFFF; b = 0xFFFF; }
    EZ_ALWAYS_INLINE void Add(ezUInt16 x) { (a == 0xFFFF ? a : b) = x; }
    EZ_ALWAYS_INLINE bool Contains(ezUInt16 x) { return a == x || b == x; }
    EZ_ALWAYS_INLINE void Remove(ezUInt16 x) { (a == x ? a : b) = 0xFFFF; }
    EZ_ALWAYS_INLINE int GetSize() { return (a != 0xFFFF) + (b != 0xFFFF); }

    ezUInt16 a, b;
  };

  struct Triangle
  {
    ezVec3d m_vNormal;
    double m_fPlaneDistance;
    ezUInt16 m_uiVertexIdx[3];
    bool m_bFlip;
    bool m_bIsDegenerate;
  };

  // used for mesh simplification
  ezAngle m_MinTriangleAngle;
  ezAngle m_FlatVertexNormalThreshold;
  double m_fMinTriangleEdgeLength;

  ezVec3d m_vCenter;
  double m_fScale;

  ezVec3d m_vInside;

  // all the 'good' vertices (no duplicates)
  // normalized to be within a unit-cube
  ezDynamicArray<ezVec3d> m_Vertices;

  // Will be resized to Square(m_Vertices.GetCount())
  // Index [i * m_Vertices.GetCount() + j] indicates which (up to two) other points
  // combine with the edge i and j to make a triangle in the hull.  Only defined when i < j.
  ezDynamicArray<TwoSet> m_Edges;

  ezDeque<Triangle> m_Triangles;
};

