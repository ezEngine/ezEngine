#ifndef AE_FOUNDATION_MATH_QUATERNION_H
#define AE_FOUNDATION_MATH_QUATERNION_H

#include "Vec3.h"

namespace AE_NS_FOUNDATION
{
  class AE_FOUNDATION_DLL aeQuaternion
  {
  // *** Data ***
  public:
    aeVec3 v;
    float w;

    enum Initialization
    {
      Identity,
    };

  // *** Constructors ***
  public:

    aeQuaternion () {}
    //! Constructor that initializes the Quaternion with the identity.
    aeQuaternion (Initialization init);
    //! Initializes the Quaternion with these values.
    aeQuaternion (const aeVec3& V, float W);
    //! Initializes the Quaternion with these values.
    aeQuaternion (float X, float Y, float Z, float W);


  // *** Functions to create a quaternion ***
  public:

    //! Sets the Quaternion to the identity.
    void SetIdentity (void);
    //! Sets the Quaternion to these values.
    void SetQuaternion (const aeVec3& V, float W);
    //! Sets the Quaternion to these values.
    void SetQuaternion (float X, float Y, float Z, float W);

    //! Creates a quaternion from a rotation-axis and an angle.
    void CreateQuaternion (const aeVec3& vRotationAxis, float fAngle);
    //! Creates a quaternion, that rotates through the shortest arc from "vDirFrom" to "vDirTo".
    void CreateQuaternion (const aeVec3& vDirFrom, const aeVec3& vDirTo);


  // *** Common Functions ***
  public:

    //! Normalizes the quaternion to unit length. ALL rotation-quaternions should be normalized at all times.
    void Normalize (void);
    //! Returns this quaternion normalized.
    const aeQuaternion GetNormalized (void) const;

    //! Returns the rotation-axis, that this quaternion rotates around.
    const aeVec3 GetRotationAxis (void) const;
    //! Returns the angle on the rotation-axis, that this quaternion rotates.
    float GetRotationAngle (void) const;

    //! Returns the Quaternion as a matrix.
    const aeMatrix GetAsMatrix (void) const;

  // *** Operators ***
  public:
    const aeQuaternion operator- (void) const;
  };

  //! Rotates v by q
  const aeVec3 operator* (const aeQuaternion& q, const aeVec3& v);
  //! Concatenates the rotations of q1 and q2
  const aeQuaternion operator* (const aeQuaternion& q1, const aeQuaternion& q2);

}

#include "Inline/Quaternion.inl"

#endif



