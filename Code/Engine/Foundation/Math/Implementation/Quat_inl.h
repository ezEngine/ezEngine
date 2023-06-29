#pragma once

#include <Foundation/Math/Mat4.h>
#include <Foundation/Math/Vec3.h>

template <typename Type>
EZ_ALWAYS_INLINE ezQuatTemplate<Type>::ezQuatTemplate()
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  const Type TypeNaN = ezMath::NaN<Type>();
  w = TypeNaN;
#endif
}

template <typename Type>
EZ_ALWAYS_INLINE ezQuatTemplate<Type>::ezQuatTemplate(Type x, Type y, Type z, Type w)
  : v(x, y, z)
  , w(w)
{
}

template <typename Type>
EZ_ALWAYS_INLINE const ezQuatTemplate<Type> ezQuatTemplate<Type>::IdentityQuaternion()
{
  return ezQuatTemplate(0, 0, 0, 1);
}

template <typename Type>
EZ_ALWAYS_INLINE void ezQuatTemplate<Type>::SetElements(Type inX, Type inY, Type inZ, Type inW)
{
  v.Set(inX, inY, inZ);
  w = inW;
}

template <typename Type>
EZ_ALWAYS_INLINE void ezQuatTemplate<Type>::SetIdentity()
{
  v.SetZero();
  w = (Type)1;
}

template <typename Type>
void ezQuatTemplate<Type>::SetFromAxisAndAngle(const ezVec3Template<Type>& vRotationAxis, ezAngle angle)
{
  const ezAngle halfAngle = angle * 0.5f;

  v = static_cast<Type>(ezMath::Sin(halfAngle)) * vRotationAxis;
  w = ezMath::Cos(halfAngle);
}

template <typename Type>
void ezQuatTemplate<Type>::Normalize()
{
  EZ_NAN_ASSERT(this);

  Type n = v.x * v.x + v.y * v.y + v.z * v.z + w * w;

  n = ezMath::Invert(ezMath::Sqrt(n));

  v *= n;
  w *= n;
}

template <typename Type>
void ezQuatTemplate<Type>::GetRotationAxisAndAngle(ezVec3Template<Type>& out_vAxis, ezAngle& out_angle, Type fEpsilon) const
{
  EZ_NAN_ASSERT(this);

  out_angle = 2 * ezMath::ACos(static_cast<float>(w));

  const float s = ezMath::Sqrt(1 - w * w);

  if (s < fEpsilon)
  {
    out_vAxis.Set(1, 0, 0);
  }
  else
  {
    const float ds = 1.0f / s;
    out_vAxis.x = v.x * ds;
    out_vAxis.y = v.y * ds;
    out_vAxis.z = v.z * ds;
  }
}

template <typename Type>
EZ_FORCE_INLINE void ezQuatTemplate<Type>::Invert()
{
  EZ_NAN_ASSERT(this);

  *this = -(*this);
}

template <typename Type>
EZ_FORCE_INLINE const ezQuatTemplate<Type> ezQuatTemplate<Type>::operator-() const
{
  EZ_NAN_ASSERT(this);

  return (ezQuatTemplate(-v.x, -v.y, -v.z, w));
}

template <typename Type>
EZ_FORCE_INLINE Type ezQuatTemplate<Type>::Dot(const ezQuatTemplate& rhs) const
{
  EZ_NAN_ASSERT(this);
  EZ_NAN_ASSERT(&rhs);

  return v.Dot(rhs.v) + w * rhs.w;
}

template <typename Type>
EZ_ALWAYS_INLINE const ezVec3Template<Type> operator*(const ezQuatTemplate<Type>& q, const ezVec3Template<Type>& v)
{
  ezVec3Template<Type> t = q.v.CrossRH(v) * (Type)2;
  return v + q.w * t + q.v.CrossRH(t);
}

template <typename Type>
EZ_ALWAYS_INLINE const ezQuatTemplate<Type> operator*(const ezQuatTemplate<Type>& q1, const ezQuatTemplate<Type>& q2)
{
  ezQuatTemplate<Type> q;

  q.w = q1.w * q2.w - q1.v.Dot(q2.v);
  q.v = q1.w * q2.v + q2.w * q1.v + q1.v.CrossRH(q2.v);

  return (q);
}

template <typename Type>
bool ezQuatTemplate<Type>::IsValid(Type fEpsilon) const
{
  if (!v.IsValid())
    return false;
  if (!ezMath::IsFinite(w))
    return false;

  Type n = v.x * v.x + v.y * v.y + v.z * v.z + w * w;

  return (ezMath::IsEqual(n, (Type)1, fEpsilon));
}

template <typename Type>
bool ezQuatTemplate<Type>::IsNaN() const
{
  return v.IsNaN() || ezMath::IsNaN(w);
}

template <typename Type>
bool ezQuatTemplate<Type>::IsEqualRotation(const ezQuatTemplate<Type>& qOther, Type fEpsilon) const
{
  if (v.IsEqual(qOther.v, (Type)0.00001) && ezMath::IsEqual(w, qOther.w, (Type)0.00001))
  {
    return true;
  }

  ezVec3Template<Type> vA1, vA2;
  ezAngle A1, A2;

  GetRotationAxisAndAngle(vA1, A1);
  qOther.GetRotationAxisAndAngle(vA2, A2);

  if ((A1.IsEqualSimple(A2, ezAngle::Degree(static_cast<float>(fEpsilon)))) && (vA1.IsEqual(vA2, fEpsilon)))
    return true;

  if ((A1.IsEqualSimple(-A2, ezAngle::Degree(static_cast<float>(fEpsilon)))) && (vA1.IsEqual(-vA2, fEpsilon)))
    return true;

  return false;
}

template <typename Type>
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

template <typename Type>
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

template <typename Type>
void ezQuatTemplate<Type>::SetFromMat3(const ezMat3Template<Type>& m)
{
  EZ_NAN_ASSERT(&m);

  const Type trace = m.Element(0, 0) + m.Element(1, 1) + m.Element(2, 2);
  const Type half = (Type)0.5;

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
    const ezInt32 next[] = {1, 2, 0};
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

template <typename Type>
void ezQuatTemplate<Type>::ReconstructFromMat3(const ezMat3Template<Type>& mMat)
{
  const ezVec3 x = (mMat * ezVec3(1, 0, 0)).GetNormalized();
  const ezVec3 y = (mMat * ezVec3(0, 1, 0)).GetNormalized();
  const ezVec3 z = x.CrossRH(y);

  ezMat3 m;
  m.SetColumn(0, x);
  m.SetColumn(1, y);
  m.SetColumn(2, z);

  SetFromMat3(m);
}

template <typename Type>
void ezQuatTemplate<Type>::ReconstructFromMat4(const ezMat4Template<Type>& mMat)
{
  const ezVec3 x = mMat.TransformDirection(ezVec3(1, 0, 0)).GetNormalized();
  const ezVec3 y = mMat.TransformDirection(ezVec3(0, 1, 0)).GetNormalized();
  const ezVec3 z = x.CrossRH(y);

  ezMat3 m;
  m.SetColumn(0, x);
  m.SetColumn(1, y);
  m.SetColumn(2, z);

  SetFromMat3(m);
}

/*! \note This function will ALWAYS return a quaternion that rotates from one direction to another.
  If both directions are identical, it is the unit rotation (none). If they are exactly opposing, this will be
  ANY 180.0 degree rotation. That means the vectors will align perfectly, but there is no determine rotation for other points
  that might be rotated with this quaternion. If a main / fallback axis is needed to rotate points, you need to calculate
  such a rotation with other means.
*/
template <typename Type>
void ezQuatTemplate<Type>::SetShortestRotation(const ezVec3Template<Type>& vDirFrom, const ezVec3Template<Type>& vDirTo)
{
  const ezVec3Template<Type> v0 = vDirFrom.GetNormalized();
  const ezVec3Template<Type> v1 = vDirTo.GetNormalized();

  const Type fDot = v0.Dot(v1);

  // if both vectors are identical -> no rotation needed
  if (ezMath::IsEqual(fDot, (Type)1, (Type)0.0000001))
  {
    SetIdentity();
    return;
  }
  else if (ezMath::IsEqual(fDot, (Type)-1, (Type)0.0000001)) // if both vectors are opposing
  {
    // find an axis, that is not identical and not opposing, ezVec3Template::Cross-product to find perpendicular vector, rotate around that
    if (ezMath::Abs(v0.Dot(ezVec3Template<Type>(1, 0, 0))) < (Type)0.8)
      SetFromAxisAndAngle(v0.CrossRH(ezVec3Template<Type>(1, 0, 0)).GetNormalized(), ezAngle::Radian(ezMath::Pi<float>()));
    else
      SetFromAxisAndAngle(v0.CrossRH(ezVec3Template<Type>(0, 1, 0)).GetNormalized(), ezAngle::Radian(ezMath::Pi<float>()));

    return;
  }

  const ezVec3Template<Type> c = v0.CrossRH(v1);
  const Type d = v0.Dot(v1);
  const Type s = ezMath::Sqrt(((Type)1 + d) * (Type)2);

  EZ_ASSERT_DEBUG(c.IsValid(), "SetShortestRotation failed.");

  v = c / s;
  w = s / (Type)2;

  Normalize();
}

template <typename Type>
void ezQuatTemplate<Type>::SetSlerp(const ezQuatTemplate<Type>& qFrom, const ezQuatTemplate<Type>& qTo, Type t)
{
  EZ_ASSERT_DEBUG((t >= (Type)0) && (t <= (Type)1), "Invalid lerp factor.");

  const Type one = 1;
  const Type qdelta = (Type)1 - (Type)0.001;

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
    ezAngle theta = ezMath::ACos((float)cosTheta);

    // use sqrtInv(1+c^2) instead of 1.0/sin(theta)
    const Type iSinTheta = (Type)1 / ezMath::Sqrt(one - (cosTheta * cosTheta));
    const ezAngle tTheta = static_cast<float>(t) * theta;

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

template <typename Type>
EZ_ALWAYS_INLINE bool operator==(const ezQuatTemplate<Type>& q1, const ezQuatTemplate<Type>& q2)
{
  return q1.v.IsIdentical(q2.v) && q1.w == q2.w;
}

template <typename Type>
EZ_ALWAYS_INLINE bool operator!=(const ezQuatTemplate<Type>& q1, const ezQuatTemplate<Type>& q2)
{
  return !(q1 == q2);
}

template <typename Type>
void ezQuatTemplate<Type>::GetAsEulerAngles(ezAngle& out_x, ezAngle& out_y, ezAngle& out_z) const
{
  // Taken from https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
  // and http://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToEuler/
  // adapted to our convention (yaw->pitch->roll, ZYX order or 3-2-1 order)

  auto& yaw = out_z;
  auto& pitch = out_y;
  auto& roll = out_x;

  const double fSingularityTest = w * v.y - v.z * v.x;
  const double fSingularityThreshold = 0.499;

  if (fSingularityTest > fSingularityThreshold) // singularity at north pole
  {
    yaw = -2.0f * ezMath::ATan2(v.x, w);
    pitch = ezAngle::Degree(90.0f);
    roll = ezAngle::Degree(0.0f);
  }
  else if (fSingularityTest < -fSingularityThreshold) // singularity at south pole
  {
    yaw = 2.0f * ezMath::ATan2(v.x, w);
    pitch = ezAngle::Degree(-90.0f);
    roll = ezAngle::Degree(0.0f);
  }
  else
  {
    // yaw (z-axis rotation)
    const double siny = 2.0 * (w * v.z + v.x * v.y);
    const double cosy = 1.0 - 2.0 * (v.y * v.y + v.z * v.z);
    yaw = ezMath::ATan2((float)siny, (float)cosy);

    // pitch (y-axis rotation)
    pitch = ezMath::ASin(2.0f * (float)fSingularityTest);

    // roll (x-axis rotation)
    const double sinr = 2.0 * (w * v.x + v.y * v.z);
    const double cosr = 1.0 - 2.0 * (v.x * v.x + v.y * v.y);
    roll = ezMath::ATan2((float)sinr, (float)cosr);
  }
}

template <typename Type>
void ezQuatTemplate<Type>::SetFromEulerAngles(const ezAngle& x, const ezAngle& y, const ezAngle& z)
{
  /// Taken from here (yaw->pitch->roll, ZYX order or 3-2-1 order):
  /// https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
  const auto& yaw = z;
  const auto& pitch = y;
  const auto& roll = x;
  const double cy = ezMath::Cos(yaw * 0.5);
  const double sy = ezMath::Sin(yaw * 0.5);
  const double cp = ezMath::Cos(pitch * 0.5);
  const double sp = ezMath::Sin(pitch * 0.5);
  const double cr = ezMath::Cos(roll * 0.5);
  const double sr = ezMath::Sin(roll * 0.5);

  w = (float)(cy * cp * cr + sy * sp * sr);
  v.x = (float)(cy * cp * sr - sy * sp * cr);
  v.y = (float)(cy * sp * cr + sy * cp * sr);
  v.z = (float)(sy * cp * cr - cy * sp * sr);
}
