#ifndef AE_FOUNDATION_MATH_PLANE_H
#define AE_FOUNDATION_MATH_PLANE_H

#include "Vec3.h"

namespace AE_NS_FOUNDATION
{
  struct aePositionOnPlane
  {
    enum Enum
    {
      Back,       //!< Something is completely on the back side of a plane
      Front,      //!< Something is completely in front of a plane
      OnPlane,    //!< Something is lying completely on a plane (all points)
      Spanning,   //!< Something is spanning a plane, i.e. some points are on the front and some on the back
    };
  };

  //! A class that represents a mathematical plane.
  class AE_FOUNDATION_DLL aePlane
  {
  // *** Data ***
  public:

    aeVec3 m_vNormal;
    float m_fDistance;

  // *** Constructors ***
  public:

    aePlane () {}
    //! Creates the plane-equation from a normal and a distance.
    aePlane (const aeVec3 &vNormal, float fDistToPlane);
    //! Creates the plane-equation from a normal and a point on the plane.
    aePlane (const aeVec3 &vNormal, const aeVec3& vPointOnPlane);
    //! Creates the plane-equation from three points on the plane.
    aePlane (const aeVec3& v1, const aeVec3& v2, const aeVec3& v3);
    //! Creates the plane-equation from three points on the plane, given as an array.
    aePlane (const aeVec3* const pVertices);
    //! Creates the plane-equation from a set of unreliable points lying on the same plane. Some points might be equal or too close to each other for the typical algorithm.
    aePlane (const aeVec3* const pVertices, aeUInt32 iMaxVertices);

    //! Creates the plane-equation from a normal and a distance.
    void CreatePlane (const aeVec3 &vNormal, float fDistToPlane);
    //! Creates the plane-equation from a normal and a point on the plane.
    void CreatePlane (const aeVec3 &vNormal, const aeVec3& vPointOnPlane);
    //! Creates the plane-equation from three points on the plane.
    void CreatePlane (const aeVec3& v1, const aeVec3& v2, const aeVec3& v3);
    //! Creates the plane-equation from three points on the plane, given as an array.
    void CreatePlane (const aeVec3* const pVertices);
    //! Creates the plane-equation from a set of unreliable points lying on the same plane. Some points might be equal or too close to each other for the typical algorithm.
    void CreatePlane (const aeVec3* const pVertices, aeUInt32 iMaxVertices);

    //! Returns three points from an unreliable set of points, that reliably form a plane. Returns false, if there are none.
    static bool FindSupportPoints (const aeVec3* const pVertices, int iMaxVertices, int& out_v1, int& out_v2, int& out_v3);

    //! Transforms the plane with the given matrix.
    void TransformPlane (const aeMatrix& m);

    //! Negates Normal/Distance to switch which side of the plane is front and back.
    void FlipPlane (void);

  // *** Distance and Position ***
  public:

    //! Returns the distance of the point to the plane.
    float GetDistanceToPoint (const aeVec3& vPoint) const;

    //! Returns on which side of the plane the point lies.
    aePositionOnPlane::Enum GetPointPosition (const aeVec3& vPoint) const;
    //! Returns on which side of the plane the point lies.
    aePositionOnPlane::Enum GetPointPosition (const aeVec3& vPoint, float fPlaneHalfWidth) const;
    //! Returns on which side of the plane the set of points lies. Might be on boths sides.
    aePositionOnPlane::Enum GetObjectPosition (const aeVec3* const vPoints, int iVertices) const;
    //! Returns on which side of the plane the set of points lies. Might be on boths sides.
    aePositionOnPlane::Enum GetObjectPosition (const aeVec3* const vPoints, int iVertices, float fPlaneHalfWidth) const;

    //! Projects a point onto a plane (along the planes normal).
    const aeVec3 ProjectOntoPlane (const aeVec3& vPoint) const;

  // *** Ray/Line Intersection Tests ***
  public:

    //! Returns true, if the ray intersects the plane. Intersection time and point are stored in the out-parameters.
    bool GetRayIntersection (const aeVec3& vPointOnRay, const aeVec3& vRayDirectionNormalized, float& out_fIntersectionTime, aeVec3& out_vIntersection) const;
    //! Returns true, if the ray intersects the plane. Intersection time and point are stored in the out-parameters. Allows for intersections at negative times (shooting into the opposite direction) AND intersections from rays that start off behind the plane.
    bool GetInfiniteRayIntersection (const aeVec3& vPointOnRay, const aeVec3& vRayDirectionNormalized, float& out_fIntersectionTime, aeVec3& out_vIntersection) const;
  
  };
}

#include "Inline/Plane.inl"

#endif


