#pragma once

#include <Foundation/Math/Vec3.h>

/// \brief Describes on which side of a plane a point or an object is located.
struct ezPositionOnPlane
{
  enum Enum
  {
    Back,       ///< Something is completely on the back side of a plane
    Front,      ///< Something is completely in front of a plane
    OnPlane,    ///< Something is lying completely on a plane (all points)
    Spanning,   ///< Something is spanning a plane, i.e. some points are on the front and some on the back
  };
};

/// \brief A class that represents a mathematical plane.
template<typename Type>
class ezPlaneTemplate
{
public:
  // Means this object can be copied using memcpy instead of copy construction.
  EZ_DECLARE_POD_TYPE();

  typedef Type ComponentType;

// *** Data ***
public:

  ezVec3Template<Type> m_vNormal;
  Type m_fNegDistance;


// *** Constructors ***
public:

  /// \brief Default constructor. Does not initialize the plane.
  ezPlaneTemplate(); // [tested]

  /// \brief Creates the plane-equation from a normal and a point on the plane.
  ezPlaneTemplate(const ezVec3Template<Type> &vNormal, const ezVec3Template<Type>& vPointOnPlane); // [tested]

  /// \brief Creates the plane-equation from three points on the plane.
  ezPlaneTemplate(const ezVec3Template<Type>& v1, const ezVec3Template<Type>& v2, const ezVec3Template<Type>& v3); // [tested]

  /// \brief Creates the plane-equation from three points on the plane, given as an array.
  ezPlaneTemplate(const ezVec3Template<Type>* const pVertices); // [tested]

  /// \brief Creates the plane-equation from a set of unreliable points lying on the same plane. Some points might be equal or too close to each other for the typical algorithm.
  ezPlaneTemplate(const ezVec3Template<Type>* const pVertices, ezUInt32 iMaxVertices); // [tested]

#if EZ_ENABLED(EZ_MATH_CHECK_FOR_NAN)
  void AssertNotNaN() const
  {
    EZ_ASSERT_ALWAYS(!IsNaN(), "This object contains NaN values. This can happen when you forgot to initialize it before using it. Please check that all code-paths properly initialize this object.");
  }
#endif

  /// \brief Creates the plane-equation from a normal and a point on the plane.
  void SetFromNormalAndPoint(const ezVec3Template<Type> &vNormal, const ezVec3Template<Type>& vPointOnPlane); // [tested]

  /// \brief Creates the plane-equation from three points on the plane.
  ezResult SetFromPoints(const ezVec3Template<Type>& v1, const ezVec3Template<Type>& v2, const ezVec3Template<Type>& v3); // [tested]

  /// \brief Creates the plane-equation from three points on the plane, given as an array.
  ezResult SetFromPoints(const ezVec3Template<Type>* const pVertices); // [tested]

  /// \brief Creates the plane-equation from a set of unreliable points lying on the same plane. Some points might be equal or too close to each other for the typical algorithm. Returns false, if no reliable set of points could be found. Does try to create a plane anyway.
  ezResult SetFromPoints(const ezVec3Template<Type>* const pVertices, ezUInt32 iMaxVertices); // [tested]

  /// \brief Creates a plane from two direction vectors that span the plane, and one point on it.
  ezResult SetFromDirections(const ezVec3Template<Type>& vTangent1, const ezVec3Template<Type>& vTangent2, const ezVec3Template<Type>& vPointOnPlane); // [tested]

  /// \brief Sets the plane to an invalid state (all zero).
  void SetInvalid(); // [tested]

// *** Distance and Position ***
public:

  /// \brief Returns the distance of the point to the plane.
  Type GetDistanceTo(const ezVec3Template<Type>& vPoint) const; // [tested]

  /// \brief Returns the minimum distance that any of the given points had to the plane.
  ///
  /// 'Minimum' means the (non-absolute) distance of a point to the plane. So a point behind the plane will always have a 'lower distance'
  /// than a point in front of the plane, even if that is closer to the plane's surface.
  Type GetMinimumDistanceTo(const ezVec3Template<Type>* pPoints, ezUInt32 uiNumPoints, ezUInt32 uiStride = sizeof (ezVec3Template<Type>)) const; // [tested]

  /// \brief Returns the minimum and maximum distance that any of the given points had to the plane.
  ///
  /// 'Minimum' (and 'maximum') means the (non-absolute) distance of a point to the plane. So a point behind the plane will always have a 'lower distance'
  /// than a point in front of the plane, even if that is closer to the plane's surface.
  void GetMinMaxDistanceTo(Type &out_fMin, Type &out_fMax, const ezVec3Template<Type>* pPoints, ezUInt32 uiNumPoints, ezUInt32 uiStride = sizeof (ezVec3Template<Type>)) const; // [tested]

  /// \brief Returns on which side of the plane the point lies.
  ezPositionOnPlane::Enum GetPointPosition(const ezVec3Template<Type>& vPoint) const; // [tested]

  /// \brief Returns on which side of the plane the point lies.
  ezPositionOnPlane::Enum GetPointPosition(const ezVec3Template<Type>& vPoint, Type fPlaneHalfWidth) const; // [tested]

  /// \brief Returns on which side of the plane the set of points lies. Might be on both sides.
  ezPositionOnPlane::Enum GetObjectPosition(const ezVec3Template<Type>* const vPoints, ezUInt32 iVertices) const; // [tested]

  /// \brief Returns on which side of the plane the set of points lies. Might be on both sides.
  ezPositionOnPlane::Enum GetObjectPosition(const ezVec3Template<Type>* const vPoints, ezUInt32 iVertices, Type fPlaneHalfWidth) const; // [tested]

  /// \brief Returns on which side of the plane the sphere is located.
  ezPositionOnPlane::Enum GetObjectPosition(const ezBoundingSphereTemplate<Type>& Sphere) const; // [tested]

  /// \brief Returns on which side of the plane the box is located.
  ezPositionOnPlane::Enum GetObjectPosition(const ezBoundingBoxTemplate<Type>& Box) const; // [tested]

  /// \brief Projects a point onto a plane (along the planes normal).
  const ezVec3Template<Type> ProjectOntoPlane(const ezVec3Template<Type>& vPoint) const; // [tested]

  /// \brief Returns the mirrored point. E.g. on the other side of the plane, at the same distance.
  const ezVec3Template<Type> Mirror(const ezVec3Template<Type>& vPoint) const; // [tested]

  /// \brief Take the given direction vector and returns a modified one that is coplanar to the plane.
  const ezVec3Template<Type> GetCoplanarDirection(const ezVec3Template<Type>& vDirection) const; // [tested]

// *** Comparisons ***
public:

  /// \brief Checks whether this plane and the other are identical.
  bool IsIdentical(const ezPlaneTemplate<Type>& rhs) const; // [tested]

  /// \brief Checks whether this plane and the other are equal within some threshold.
  bool IsEqual(const ezPlaneTemplate<Type>& rhs, Type fEpsilon = ezMath::BasicType<Type>::DefaultEpsilon()) const; // [tested]

  /// \brief Checks whether the plane has valid values (not NaN, or infinite, normalized normal).
  bool IsValid() const; // [tested]

  /// \brief Checks whether any component is NaN.
  bool IsNaN() const; // [tested]

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
  bool GetRayIntersection(const ezVec3Template<Type>& vRayStartPos, const ezVec3Template<Type>& vRayDir, Type* out_fIntersection = nullptr, ezVec3Template<Type>* out_vIntersection = nullptr) const; // [tested]

  /// \brief Returns true, if the ray intersects the plane. Intersection time and point are stored in the out-parameters. Allows for intersections at negative times (shooting into the opposite direction).
  bool GetRayIntersectionBiDirectional(const ezVec3Template<Type>& vRayStartPos, const ezVec3Template<Type>& vRayDir, Type* out_fIntersection = nullptr, ezVec3Template<Type>* out_vIntersection = nullptr) const; // [tested]

  /// \brief Returns true, if there is any intersection with the plane between the line's start and end position. Returns the fraction along the line and the actual intersection point.
  bool GetLineSegmentIntersection(const ezVec3Template<Type>& vLineStartPos, const ezVec3Template<Type>& vLineEndPos, Type* out_fHitFraction = nullptr, ezVec3Template<Type>* out_vIntersection = nullptr) const; // [tested]

  /// \brief Computes the one point where all three planes intersect. Returns EZ_FAILURE if no such point exists.
  static ezResult GetPlanesIntersectionPoint(const ezPlaneTemplate<Type>& p0, const ezPlaneTemplate<Type>& p1, const ezPlaneTemplate<Type>& p2, ezVec3Template<Type>& out_Result); // [tested]

// *** Helper Functions ***
public:

  /// \brief Returns three points from an unreliable set of points, that reliably form a plane. Returns false, if there are none.
  static ezResult FindSupportPoints(const ezVec3Template<Type>* const pVertices, ezInt32 iMaxVertices, ezInt32& out_v1, ezInt32& out_v2, ezInt32& out_v3); // [tested]

};

/// \brief Checks whether this plane and the other are identical.
template<typename Type>
bool operator== (const ezPlaneTemplate<Type>& lhs, const ezPlaneTemplate<Type>& rhs); // [tested]

/// \brief Checks whether this plane and the other are not identical.
template<typename Type>
bool operator!= (const ezPlaneTemplate<Type>& lhs, const ezPlaneTemplate<Type>& rhs); // [tested]

#include <Foundation/Math/Implementation/Plane_inl.h>



