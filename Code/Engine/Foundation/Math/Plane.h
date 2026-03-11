#pragma once

#include <Foundation/Math/Vec3.h>
#include <Foundation/Math/Vec4.h>

/// \brief Describes on which side of a plane a point or an object is located.
struct ezPositionOnPlane
{
  enum Enum
  {
    Back,     ///< Something is completely on the back side of a plane
    Front,    ///< Something is completely in front of a plane
    OnPlane,  ///< Something is lying completely on a plane (all points)
    Spanning, ///< Something is spanning a plane, i.e. some points are on the front and some on the back
  };
};

/// \brief A class that represents a mathematical plane.
///
/// A plane in 3D space is defined by a normal vector and a distance from the origin.
/// This implementation uses the equation: normal · point + distance = 0, where the distance
/// is stored as negative for mathematical convenience in many operations.
template <typename Type>
struct ezPlaneTemplate
{
public:
  // Means this object can be copied using memcpy instead of copy construction.
  EZ_DECLARE_POD_TYPE();

  using ComponentType = Type;

  // *** Data ***
public:
  ezVec3Template<Type> m_vNormal;
  Type m_fNegDistance;


  // *** Constructors ***
public:
  /// \brief Default constructor. Does not initialize the plane.
  ezPlaneTemplate(); // [tested]

  /// \brief Returns an invalid plane with a zero normal.
  [[nodiscard]] static ezPlaneTemplate<Type> MakeInvalid();

  /// \brief Creates a plane from a normal and a point on the plane.
  ///
  /// \note This function asserts that the normal is normalized.
  [[nodiscard]] static ezPlaneTemplate<Type> MakeFromNormalAndPoint(const ezVec3Template<Type>& vNormal, const ezVec3Template<Type>& vPointOnPlane);

  /// \brief Creates a plane from three points.
  ///
  /// \note Asserts that the 3 points properly form a plane.
  /// Only use this function when you are certain that the input data isn't degenerate.
  /// If the data cannot be trusted, use SetFromPoints() and check the result.
  [[nodiscard]] static ezPlaneTemplate<Type> MakeFromPoints(const ezVec3Template<Type>& v1, const ezVec3Template<Type>& v2, const ezVec3Template<Type>& v3);

#if EZ_ENABLED(EZ_MATH_CHECK_FOR_NAN)
  void AssertNotNaN() const
  {
    EZ_ASSERT_ALWAYS(!IsNaN(), "This object contains NaN values. This can happen when you forgot to initialize it before using it. Please check that "
                               "all code-paths properly initialize this object.");
  }
#endif

  /// \brief Returns an ezVec4 with the plane normal in x,y,z and the negative distance in w.
  ezVec4Template<Type> GetAsVec4() const;

  /// \brief Creates the plane-equation from three points on the plane.
  ezResult SetFromPoints(const ezVec3Template<Type>& v1, const ezVec3Template<Type>& v2, const ezVec3Template<Type>& v3); // [tested]

  /// \brief Creates the plane-equation from three points on the plane, given as an array.
  ezResult SetFromPoints(const ezVec3Template<Type>* const pVertices); // [tested]

  /// \brief Creates the plane-equation from a set of unreliable points lying on the same plane. Some points might be equal or too close to each other
  /// for the typical algorithm. Returns false, if no reliable set of points could be found. Does try to create a plane anyway.
  ezResult SetFromPoints(const ezVec3Template<Type>* const pVertices, ezUInt32 uiMaxVertices); // [tested]

  /// \brief Creates a plane from two direction vectors that span the plane, and one point on it.
  ezResult SetFromDirections(const ezVec3Template<Type>& vTangent1, const ezVec3Template<Type>& vTangent2, const ezVec3Template<Type>& vPointOnPlane); // [tested]

  // *** Distance and Position ***
public:
  /// \brief Returns the distance of the point to the plane.
  Type GetDistanceTo(const ezVec3Template<Type>& vPoint) const; // [tested]

  /// \brief Returns the minimum distance that any of the given points had to the plane.
  ///
  /// 'Minimum' means the (non-absolute) distance of a point to the plane. So a point behind the plane will always have a 'lower distance'
  /// than a point in front of the plane, even if that is closer to the plane's surface.
  Type GetMinimumDistanceTo(const ezVec3Template<Type>* pPoints, ezUInt32 uiNumPoints, ezUInt32 uiStride = sizeof(ezVec3Template<Type>)) const; // [tested]

  /// \brief Returns the minimum distance between given box and a plane
  Type GetMinimumDistanceTo(const ezBoundingBoxTemplate<Type>& box) const; // [tested]

  /// \brief Returns the maximum distance between given box and a plane
  Type GetMaximumDistanceTo(const ezBoundingBoxTemplate<Type>& box) const; // [tested]

  /// \brief Returns the minimum and maximum distance that any of the given points had to the plane.
  ///
  /// 'Minimum' (and 'maximum') means the (non-absolute) distance of a point to the plane. So a point behind the plane will always have a 'lower
  /// distance' than a point in front of the plane, even if that is closer to the plane's surface.
  void GetMinMaxDistanceTo(Type& out_fMin, Type& out_fMax, const ezVec3Template<Type>* pPoints, ezUInt32 uiNumPoints, ezUInt32 uiStride = sizeof(ezVec3Template<Type>)) const; // [tested]

  /// \brief Returns on which side of the plane the point lies.
  ezPositionOnPlane::Enum GetPointPosition(const ezVec3Template<Type>& vPoint) const; // [tested]

  /// \brief Returns on which side of the plane the point lies.
  ezPositionOnPlane::Enum GetPointPosition(const ezVec3Template<Type>& vPoint, Type fPlaneHalfWidth) const; // [tested]

  /// \brief Returns on which side of the plane the set of points lies. Might be on both sides.
  ezPositionOnPlane::Enum GetObjectPosition(const ezVec3Template<Type>* const pPoints, ezUInt32 uiVertices) const; // [tested]

  /// \brief Returns on which side of the plane the set of points lies. Might be on both sides.
  ezPositionOnPlane::Enum GetObjectPosition(const ezVec3Template<Type>* const pPoints, ezUInt32 uiVertices, Type fPlaneHalfWidth) const; // [tested]

  /// \brief Returns on which side of the plane the sphere is located.
  ezPositionOnPlane::Enum GetObjectPosition(const ezBoundingSphereTemplate<Type>& sphere) const; // [tested]

  /// \brief Returns on which side of the plane the box is located.
  ezPositionOnPlane::Enum GetObjectPosition(const ezBoundingBoxTemplate<Type>& box) const; // [tested]

  /// \brief Projects a point onto a plane (along the planes normal).
  [[nodiscard]] const ezVec3Template<Type> ProjectOntoPlane(const ezVec3Template<Type>& vPoint) const; // [tested]

  /// \brief Returns the mirrored point. E.g. on the other side of the plane, at the same distance.
  [[nodiscard]] const ezVec3Template<Type> Mirror(const ezVec3Template<Type>& vPoint) const; // [tested]

  /// \brief Take the given direction vector and returns a modified one that is coplanar to the plane.
  const ezVec3Template<Type> GetCoplanarDirection(const ezVec3Template<Type>& vDirection) const; // [tested]

  // *** Comparisons ***
public:
  /// \brief Checks whether this plane and the other are identical.
  bool IsIdentical(const ezPlaneTemplate<Type>& rhs) const; // [tested]

  /// \brief Checks whether this plane and the other are equal within some threshold.
  bool IsEqual(const ezPlaneTemplate<Type>& rhs, Type fEpsilon = ezMath::DefaultEpsilon<Type>()) const; // [tested]

  /// \brief Checks whether the plane has valid values (not NaN, normalized normal).
  bool IsValid() const; // [tested]

  /// \brief Checks whether any component is NaN.
  bool IsNaN() const; // [tested]

  /// \brief Checks whether any component is Infinity.
  bool IsFinite() const; // [tested]

  // *** Modifications ***
public:
  /// \brief Transforms the plane with the given matrix.
  void Transform(const ezMat3Template<Type>& m); // [tested]

  /// \brief Transforms the plane with the given matrix.
  void Transform(const ezMat4Template<Type>& m); // [tested]

  /// \brief Negates Normal/Distance to switch which side of the plane is front and back.
  void Flip(); // [tested]

  /// \brief Negates Normal/Distance to switch which side of the plane is front and back. Returns true, if the plane had to be flipped.
  bool FlipIfNecessary(const ezVec3Template<Type>& vPoint, bool bPlaneShouldFacePoint = true); // [tested]

  // *** Intersection Tests ***
public:
  /// \brief Returns true, if the ray hit the plane. The intersection time describes at which multiple of the ray direction the ray hit the plane.
  ///
  /// An intersection will be reported regardless of whether the ray starts 'behind' or 'in front of' the plane, as long as it points at it.
  /// \a vRayDir does not need to be normalized.\n
  /// out_vIntersection = vRayStartPos + out_fIntersection * vRayDir
  ///
  /// Intersections with \a out_fIntersection less than zero will be discarded and not reported as intersections.
  /// If such intersections are desired, use GetRayIntersectionBiDirectional instead.
  [[nodiscard]] bool GetRayIntersection(const ezVec3Template<Type>& vRayStartPos, const ezVec3Template<Type>& vRayDir, Type* out_pIntersectionDinstance = nullptr, ezVec3Template<Type>* out_pIntersection = nullptr) const; // [tested]

  /// \brief Returns true, if the ray intersects the plane. Intersection time and point are stored in the out-parameters. Allows for intersections at
  /// negative times (shooting into the opposite direction).
  [[nodiscard]] bool GetRayIntersectionBiDirectional(const ezVec3Template<Type>& vRayStartPos, const ezVec3Template<Type>& vRayDir, Type* out_pIntersectionDistance = nullptr, ezVec3Template<Type>* out_pIntersection = nullptr) const; // [tested]

  /// \brief Returns true, if there is any intersection with the plane between the line's start and end position. Returns the fraction along the line
  /// and the actual intersection point.
  [[nodiscard]] bool GetLineSegmentIntersection(const ezVec3Template<Type>& vLineStartPos, const ezVec3Template<Type>& vLineEndPos, Type* out_pHitFraction = nullptr, ezVec3Template<Type>* out_pIntersection = nullptr) const; // [tested]

  /// \brief Computes the one point where all three planes intersect. Returns EZ_FAILURE if no such point exists.
  static ezResult GetPlanesIntersectionPoint(const ezPlaneTemplate<Type>& p0, const ezPlaneTemplate<Type>& p1, const ezPlaneTemplate<Type>& p2, ezVec3Template<Type>& out_vResult); // [tested]

  // *** Helper Functions ***
public:
  /// \brief Returns three points from an unreliable set of points, that reliably form a plane. Returns false, if there are none.
  static ezResult FindSupportPoints(const ezVec3Template<Type>* const pVertices, ezInt32 iMaxVertices, ezInt32& out_i1, ezInt32& out_i2, ezInt32& out_i3); // [tested]
};

/// \brief Checks whether this plane and the other are identical.
template <typename Type>
bool operator==(const ezPlaneTemplate<Type>& lhs, const ezPlaneTemplate<Type>& rhs); // [tested]

/// \brief Checks whether this plane and the other are not identical.
template <typename Type>
bool operator!=(const ezPlaneTemplate<Type>& lhs, const ezPlaneTemplate<Type>& rhs); // [tested]

#include <Foundation/Math/Implementation/Plane_inl.h>
