#pragma once

#include <Foundation/Math/Vec3.h>
#include <Foundation/Math/Mat4.h>

template<typename Type>
EZ_FORCE_INLINE ezQuatTemplate<Type>::ezQuatTemplate()
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  const Type TypeNaN = ezMath::BasicType<Type>::GetNaN();
  w = TypeNaN;
#endif
}

template<typename Type>
EZ_FORCE_INLINE ezQuatTemplate<Type>::ezQuatTemplate(Type X, Type Y, Type Z, Type W) : v(X, Y, Z), w(W)
{
}

template<typename Type>
EZ_FORCE_INLINE const ezQuatTemplate<Type> ezQuatTemplate<Type>::IdentityQuaternion()
{
  return ezQuatTemplate(0, 0, 0, 1);
}

template<typename Type>
EZ_FORCE_INLINE void ezQuatTemplate<Type>::SetElements(Type X, Type Y, Type Z, Type W)
{
  v.Set(X, Y, Z);
  w = W;
}

template<typename Type>
EZ_FORCE_INLINE void ezQuatTemplate<Type>::SetIdentity()
{
  v.SetZero();
  w = (Type)1;
}

template<typename Type>
void ezQuatTemplate<Type>::SetFromAxisAndAngle(const ezVec3Template<Type>& vRotationAxis, ezAngle angle)
{
  const ezAngle halfAngle = angle * 0.5f;

  v = ezMath::Sin(halfAngle) * vRotationAxis;
  w = ezMath::Cos(halfAngle);
}

template<typename Type>
void ezQuatTemplate<Type>::Normalize()
{
  EZ_NAN_ASSERT(this);

  Type n = v.x * v.x + v.y * v.y + v.z * v.z + w * w;

  n = ezMath::Invert(ezMath::Sqrt(n));

  v *= n;
  w *= n;
}

template<typename Type>
ezResult ezQuatTemplate<Type>::GetRotationAxisAndAngle(ezVec3Template<Type>& vAxis, ezAngle& angle) const
{
  EZ_NAN_ASSERT(this);

  const ezAngle acos = ezMath::ACos(w);
  const float d = ezMath::Sin(acos);

  if (d == 0)
    return EZ_FAILURE;

  vAxis = (v / d);
  angle = acos * 2;

  return EZ_SUCCESS;
}

template<typename Type>
EZ_FORCE_INLINE const ezQuatTemplate<Type> ezQuatTemplate<Type>::operator-() const
{
  EZ_NAN_ASSERT(this);

  return (ezQuatTemplate(-v.x, -v.y, -v.z, w));
}

template<typename Type>
const ezVec3Template<Type> operator* (const ezQuatTemplate<Type>& q, const ezVec3Template<Type>& v)
{
  ezVec3Template<Type> t = q.v.Cross(v) * (Type)2;
  return v + q.w * t + q.v.Cross(t);
}

template<typename Type>
EZ_FORCE_INLINE const ezQuatTemplate<Type> operator* (const ezQuatTemplate<Type>& q1, const ezQuatTemplate<Type>& q2)
{
  ezQuatTemplate<Type> q;

  q.w = q1.w * q2.w - q1.v.Dot(q2.v);
  q.v = q1.w * q2.v + q2.w * q1.v + q1.v.Cross(q2.v);

  return (q);
}

template<typename Type>
bool ezQuatTemplate<Type>::IsValid(Type fEpsilon) const
{
  if (!v.IsValid())
    return false;
  if (!ezMath::IsFinite(w))
    return false;

  Type n = v.x * v.x + v.y * v.y + v.z * v.z + w * w;

  return (ezMath::IsEqual(n, (Type)1, fEpsilon));
}

template<typename Type>
bool ezQuatTemplate<Type>::IsNaN() const
{
  return v.IsNaN() || ezMath::IsNaN(w);
}

template<typename Type>
bool ezQuatTemplate<Type>::IsEqualRotation(const ezQuatTemplate<Type>& qOther, float fEpsilon) const
{
  ezVec3Template<Type> vA1, vA2;
  ezAngle A1, A2;

  if (GetRotationAxisAndAngle(vA1, A1) == EZ_FAILURE)
    return false;
  if (qOther.GetRotationAxisAndAngle(vA2, A2) == EZ_FAILURE)
    return false;

  if ((A1.IsEqualSimple(A2, ezAngle::Degree(fEpsilon))) &&
      (vA1.IsEqual(vA2, fEpsilon)))
    return true;

  if ((A1.IsEqualSimple(-A2, ezAngle::Degree(fEpsilon))) &&
      (vA1.IsEqual(-vA2, fEpsilon)))
    return true;

  return false;
}

template<typename Type>
const ezMat3Template<Type> ezQuatTemplate<Type>::GetAsMat3() const
{
  EZ_NAN_ASSERT(this);

  ezMat3Template<Type> m;

  const Type fTx = v.x + v.x;
  const Type fTy = v.y + v.y;
  const Type fTz = v.z + v.z;
  const Type fTwx = fTx * w;
  const Type fTwy = fTy * w;
  const Type fTwz = fTz * w;
  const Type fTxx = fTx * v.x;
  const Type fTxy = fTy * v.x;
  const Type fTxz = fTz * v.x;
  const Type fTyy = fTy * v.y;
  const Type fTyz = fTz * v.y;
  const Type fTzz = fTz * v.z;

  m.Element(0, 0) = (Type)1 - (fTyy + fTzz);
  m.Element(1, 0) = fTxy - fTwz;
  m.Element(2, 0) = fTxz + fTwy;
  m.Element(0, 1) = fTxy + fTwz;
  m.Element(1, 1) = (Type)1 - (fTxx + fTzz);
  m.Element(2, 1) = fTyz - fTwx;
  m.Element(0, 2) = fTxz - fTwy;
  m.Element(1, 2) = fTyz + fTwx;
  m.Element(2, 2) = (Type)1 - (fTxx + fTyy);
  return m;
}

template<typename Type>
const ezMat4Template<Type> ezQuatTemplate<Type>::GetAsMat4() const
{
  EZ_NAN_ASSERT(this);

  ezMat4Template<Type> m;

  const Type fTx = v.x + v.x;
  const Type fTy = v.y + v.y;
  const Type fTz = v.z + v.z;
  const Type fTwx = fTx * w;
  const Type fTwy = fTy * w;
  const Type fTwz = fTz * w;
  const Type fTxx = fTx * v.x;
  const Type fTxy = fTy * v.x;
  const Type fTxz = fTz * v.x;
  const Type fTyy = fTy * v.y;
  const Type fTyz = fTz * v.y;
  const Type fTzz = fTz * v.z;

  m.Element(0, 0) = (Type)1 - (fTyy + fTzz);
  m.Element(1, 0) = fTxy - fTwz;
  m.Element(2, 0) = fTxz + fTwy;
  m.Element(3, 0) = (Type)0;
  m.Element(0, 1) = fTxy + fTwz;
  m.Element(1, 1) = (Type)1 - (fTxx + fTzz);
  m.Element(2, 1) = fTyz - fTwx;
  m.Element(3, 1) = (Type)0;
  m.Element(0, 2) = fTxz - fTwy;
  m.Element(1, 2) = fTyz + fTwx;
  m.Element(2, 2) = (Type)1 - (fTxx + fTyy);
  m.Element(3, 2) = (Type)0;
  m.Element(0, 3) = (Type)0;
  m.Element(1, 3) = (Type)0;
  m.Element(2, 3) = (Type)0;
  m.Element(3, 3) = (Type)1;
  return m;
}

template<typename Type>
void ezQuatTemplate<Type>::SetFromMat3(const ezMat3Template<Type>& m)
{
  EZ_NAN_ASSERT(&m);

  const Type trace = m.Element(0, 0) + m.Element(1, 1) + m.Element(2, 2);
  const Type half = (Type) 0.5;

  Type val[4];

  if (trace > (Type)0)
  {
    Type s = ezMath::Sqrt(trace + (Type)1);
    Type t = half / s;

    val[0] = (m.Element(1, 2) - m.Element(2, 1)) * t;
    val[1] = (m.Element(2, 0) - m.Element(0, 2)) * t;
    val[2] = (m.Element(0, 1) - m.Element(1, 0)) * t;

    val[3] = half * s;
  }
  else
  {
    const ezInt32 next[] = { 1,2,0 };
    ezInt32 i = 0;

    if (m.Element(1, 1) > m.Element(0, 0))
      i = 1;

    if (m.Element(2, 2) > m.Element(i, i))
      i = 2;

    ezInt32 j = next[i];
    ezInt32 k = next[j];

    Type s = ezMath::Sqrt(m.Element(i, i) - (m.Element(j, j) + m.Element(k, k)) + (Type)1);
    Type t = half / s;

    val[i] = half * s;
    val[3] = (m.Element(j, k) - m.Element(k, j)) * t;
    val[j] = (m.Element(i, j) + m.Element(j, i)) * t;
    val[k] = (m.Element(i, k) + m.Element(k, i)) * t;
  }

  v.x = val[0];
  v.y = val[1];
  v.z = val[2];
  w = val[3];
}

/*! \note This function will ALWAYS return a quaternion that rotates from one direction to another.
  If both directions are identical, it is the unit rotation (none). If they are exactly opposing, this will be
  ANY 180.0 degree rotation. That means the vectors will align perfectly, but there is no determine rotation for other points
  that might be rotated with this quaternion. If a main / fallback axis is needed to rotate points, you need to calculate
  such a rotation with other means.
*/
template<typename Type>
void ezQuatTemplate<Type>::SetShortestRotation(const ezVec3Template<Type>& vDirFrom, const ezVec3Template<Type>& vDirTo)
{
  const ezVec3Template<Type> v0 = vDirFrom.GetNormalized();
  const ezVec3Template<Type> v1 = vDirTo.GetNormalized();

  const Type fDot = v0.Dot(v1);

  // if both vectors are identical -> no rotation needed
  if (ezMath::IsEqual(fDot, (Type)1, (Type) 0.0001))
  {
    SetIdentity();
    return;
  }
  else
    // if both vectors are opposing
    if (ezMath::IsEqual(fDot, (Type)-1, (Type) 0.0001))
    {
      // find an axis, that is not identical and not opposing, ezVec3Template::Cross-product to find perpendicular vector, rotate around that
      if (ezMath::Abs(v0.Dot(ezVec3Template<Type>(1, 0, 0))) < (Type) 0.8)
        SetFromAxisAndAngle(v0.Cross(ezVec3Template<Type>(1, 0, 0)).GetNormalized(), ezAngle::Radian(ezMath::BasicType<float>::Pi()));
      else
        SetFromAxisAndAngle(v0.Cross(ezVec3Template<Type>(0, 1, 0)).GetNormalized(), ezAngle::Radian(ezMath::BasicType<float>::Pi()));

      return;
    }

  const ezVec3Template<Type> c = v0.Cross(v1);
  const Type d = v0.Dot(v1);
  const Type s = ezMath::Sqrt(((Type)1 + d) * (Type)2);

  EZ_ASSERT_DEBUG(c.IsValid(), "SetShortestRotation failed.");

  v = c / s;
  w = s / (Type)2;

  Normalize();
}

template<typename Type>
void ezQuatTemplate<Type>::SetSlerp(const ezQuatTemplate<Type>& qFrom, const ezQuatTemplate<Type>& qTo, Type t)
{
  EZ_ASSERT_DEBUG((t >= (Type)0) && (t <= (Type)1), "Invalid lerp factor.");

  const Type one = 1;
  const Type qdelta = (Type)1 - (Type) 0.001;

  const Type fDot = (qFrom.v.x * qTo.v.x + qFrom.v.y * qTo.v.y + qFrom.v.z * qTo.v.z + qFrom.w * qTo.w);

  Type cosTheta = fDot;

  bool bFlipSign = false;
  if (cosTheta < (Type)0)
  {
    bFlipSign = true;
    cosTheta = -cosTheta;
  }

  Type t0, t1;

  if (cosTheta < qdelta)
  {
    ezAngle theta = ezMath::ACos(cosTheta);

    // use sqrtInv(1+c^2) instead of 1.0/sin(theta) 
    const Type iSinTheta = (Type)1 / ezMath::Sqrt(one - (cosTheta*cosTheta));
    const ezAngle tTheta = t * theta;

    Type s0 = ezMath::Sin(theta - tTheta);
    Type s1 = ezMath::Sin(tTheta);

    t0 = s0 * iSinTheta;
    t1 = s1 * iSinTheta;
  }
  else
  {
    // If q0 is nearly the same as q1 we just linearly interpolate
    t0 = one - t;
    t1 = t;
  }

  if (bFlipSign)
    t1 = -t1;

  v.x = t0 * qFrom.v.x;
  v.y = t0 * qFrom.v.y;
  v.z = t0 * qFrom.v.z;
  w = t0 * qFrom.w;

  v.x += t1 * qTo.v.x;
  v.y += t1 * qTo.v.y;
  v.z += t1 * qTo.v.z;
  w += t1 * qTo.w;

  Normalize();

}

template<typename Type>
EZ_FORCE_INLINE bool operator== (const ezQuatTemplate<Type>& q1, const ezQuatTemplate<Type>& q2)
{
  return q1.v.IsIdentical(q2.v) && q1.w == q2.w;
}

template<typename Type>
EZ_FORCE_INLINE bool operator!= (const ezQuatTemplate<Type>& q1, const ezQuatTemplate<Type>& q2)
{
  return !(q1 == q2);
}

template<typename Type>
void ezQuatTemplate<Type>::GetAsEulerAngles(ezAngle& out_Yaw, ezAngle& out_Pitch, ezAngle& out_Roll) const
{
  /// \test This is new

  // Taken from here:
  // http://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToEuler/index.htm

  const double test = v.x * v.y + v.z * w;

  if (test > 0.499)
  {
    // singularity at north pole
    out_Yaw = 2.0f * ezMath::ATan2(v.x, w);
    out_Pitch = ezAngle::Radian(ezMath::BasicType<float>::Pi() / 2.0f);
    out_Roll = ezAngle::Radian(0.0f);
    return;
  }

  if (test < -0.499)
  {
    // singularity at south pole
    out_Yaw = -2.0f * ezMath::ATan2(v.x, w);
    out_Pitch = ezAngle::Radian(-ezMath::BasicType<float>::Pi() / 2.0f);
    out_Roll = ezAngle::Radian(0.0f);
    return;
  }

  const double sqx = v.x * v.x;
  const double sqy = v.y * v.y;
  const double sqz = v.z * v.z;

  out_Yaw = ezMath::ATan2((float)(2.0 * v.y * w - 2.0 * v.x * v.z), (float)(1.0 - 2.0 * sqy - 2.0 * sqz));
  out_Pitch = ezMath::ASin((float)(2.0 * test));
  out_Roll = ezMath::ATan2((float)(2.0 * v.x * w - 2.0 * v.y * v.z), (float)(1.0 - 2.0 * sqx - 2.0 * sqz));
}

template<typename Type>
void ezQuatTemplate<Type>::SetFromEulerAngles(const ezAngle& Yaw, const ezAngle& Pitch, const ezAngle& Roll)
{
  /// \test This is new

  // Taken from here:
  // http://www.euclideanspace.com/maths/geometry/rotations/conversions/eulerToQuaternion/index.htm

  const double c1 = ezMath::Cos(Yaw * 0.5f);
  const double s1 = ezMath::Sin(Yaw * 0.5f);
  const double c2 = ezMath::Cos(Pitch * 0.5f);
  const double s2 = ezMath::Sin(Pitch * 0.5f);
  const double c3 = ezMath::Cos(Roll * 0.5f);
  const double s3 = ezMath::Sin(Roll * 0.5f);
  const double c1c2 = c1 * c2;
  const double s1s2 = s1 * s2;

  w = (float)(c1c2 * c3 - s1s2 * s3);
  v.x = (float)(c1c2 * s3 + s1s2 * c3);
  v.y = (float)(s1 * c2 * c3 + c1*s2 * s3);
  v.z = (float)(c1 * s2 * c3 - s1*c2 * s3);
}