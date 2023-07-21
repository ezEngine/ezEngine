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
EZ_ALWAYS_INLINE ezQuatTemplate<Type>::ezQuatTemplate(Type inX, Type inY, Type inZ, Type inW)
  : x(inX)
  , y(inY)
  , z(inZ)
  , w(inW)
{
}

template <typename Type>
EZ_ALWAYS_INLINE const ezQuatTemplate<Type> ezQuatTemplate<Type>::MakeIdentity()
{
  return ezQuatTemplate(0, 0, 0, 1);
}

template <typename Type>
EZ_ALWAYS_INLINE ezQuatTemplate<Type> ezQuatTemplate<Type>::MakeFromElements(Type inX, Type inY, Type inZ, Type inW)
{
  return ezQuatTemplate<Type>(inX, inY, inZ, inW);
}

template <typename Type>
EZ_ALWAYS_INLINE void ezQuatTemplate<Type>::SetIdentity()
{
  x = (Type)0;
  y = (Type)0;
  z = (Type)0;
  w = (Type)1;
}

template <typename Type>
ezQuatTemplate<Type> ezQuatTemplate<Type>::MakeFromAxisAndAngle(const ezVec3Template<Type>& vRotationAxis, ezAngle angle)
{
  const ezAngle halfAngle = angle * 0.5f;

  ezVec3 v = static_cast<Type>(ezMath::Sin(halfAngle)) * vRotationAxis;
  float w = ezMath::Cos(halfAngle);

  return ezQuatTemplate<Type>(v.x, v.y, v.z, w);
}

template <typename Type>
void ezQuatTemplate<Type>::Normalize()
{
  EZ_NAN_ASSERT(this);

  Type n = x * x + y * y + z * z + w * w;

  n = ezMath::Invert(ezMath::Sqrt(n));

  x *= n;
  y *= n;
  z *= n;
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
    out_vAxis.x = x * ds;
    out_vAxis.y = y * ds;
    out_vAxis.z = z * ds;
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

  return (ezQuatTemplate(-x, -y, -z, w));
}

template <typename Type>
EZ_FORCE_INLINE Type ezQuatTemplate<Type>::Dot(const ezQuatTemplate& rhs) const
{
  EZ_NAN_ASSERT(this);
  EZ_NAN_ASSERT(&rhs);

  return GetVectorPart().Dot(rhs.GetVectorPart()) + w * rhs.w;
}

template <typename Type>
EZ_ALWAYS_INLINE const ezVec3Template<Type> operator*(const ezQuatTemplate<Type>& q, const ezVec3Template<Type>& v)
{
  ezVec3Template<Type> t = q.GetVectorPart().CrossRH(v) * (Type)2;
  return v + q.w * t + q.GetVectorPart().CrossRH(t);
}

template <typename Type>
EZ_ALWAYS_INLINE const ezQuatTemplate<Type> operator*(const ezQuatTemplate<Type>& q1, const ezQuatTemplate<Type>& q2)
{
  ezQuatTemplate<Type> q;

  q.w = q1.w * q2.w - (q1.x * q2.x + q1.y * q2.y + q1.z * q2.z);

  const ezVec3 v1 = q1.GetVectorPart();
  const ezVec3 v2 = q2.GetVectorPart();

  const ezVec3 vr = q1.w * v2 + q2.w * v1 + v1.CrossRH(v2);
  q.x = vr.x;
  q.y = vr.y;
  q.z = vr.z;

  return q;
}

template <typename Type>
bool ezQuatTemplate<Type>::IsValid(Type fEpsilon) const
{
  if (!GetVectorPart().IsValid())
    return false;
  if (!ezMath::IsFinite(w))
    return false;

  Type n = x * x + y * y + z * z + w * w;

  return ezMath::IsEqual(n, (Type)1, fEpsilon);
}

template <typename Type>
bool ezQuatTemplate<Type>::IsNaN() const
{
  return ezMath::IsNaN(x) || ezMath::IsNaN(y) || ezMath::IsNaN(z) || ezMath::IsNaN(w);
}

template <typename Type>
bool ezQuatTemplate<Type>::IsEqualRotation(const ezQuatTemplate<Type>& qOther, Type fEpsilon) const
{
  if (GetVectorPart().IsEqual(qOther.GetVectorPart(), (Type)0.00001) && ezMath::IsEqual(w, qOther.w, (Type)0.00001))
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

  const Type fTx = x + x;
  const Type fTy = y + y;
  const Type fTz = z + z;
  const Type fTwx = fTx * w;
  const Type fTwy = fTy * w;
  const Type fTwz = fTz * w;
  const Type fTxx = fTx * x;
  const Type fTxy = fTy * x;
  const Type fTxz = fTz * x;
  const Type fTyy = fTy * y;
  const Type fTyz = fTz * y;
  const Type fTzz = fTz * z;

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

  const Type fTx = x + x;
  const Type fTy = y + y;
  const Type fTz = z + z;
  const Type fTwx = fTx * w;
  const Type fTwy = fTy * w;
  const Type fTwz = fTz * w;
  const Type fTxx = fTx * x;
  const Type fTxy = fTy * x;
  const Type fTxz = fTz * x;
  const Type fTyy = fTy * y;
  const Type fTyz = fTz * y;
  const Type fTzz = fTz * z;

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
ezQuatTemplate<Type> ezQuatTemplate<Type>::MakeFromMat3(const ezMat3Template<Type>& m)
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

  ezQuatTemplate<Type> q;
  q.x = val[0];
  q.y = val[1];
  q.z = val[2];
  q.w = val[3];
  return q;
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
ezQuatTemplate<Type> ezQuatTemplate<Type>::MakeShortestRotation(const ezVec3Template<Type>& vDirFrom, const ezVec3Template<Type>& vDirTo)
{
  const ezVec3Template<Type> v0 = vDirFrom.GetNormalized();
  const ezVec3Template<Type> v1 = vDirTo.GetNormalized();

  const Type fDot = v0.Dot(v1);

  // if both vectors are identical -> no rotation needed
  if (ezMath::IsEqual(fDot, (Type)1, (Type)0.0000001))
  {
    return MakeIdentity();
  }
  else if (ezMath::IsEqual(fDot, (Type)-1, (Type)0.0000001)) // if both vectors are opposing
  {
    // find an axis, that is not identical and not opposing, ezVec3Template::Cross-product to find perpendicular vector, rotate around that
    if (ezMath::Abs(v0.Dot(ezVec3Template<Type>(1, 0, 0))) < (Type)0.8)
      return MakeFromAxisAndAngle(v0.CrossRH(ezVec3Template<Type>(1, 0, 0)).GetNormalized(), ezAngle::Radian(ezMath::Pi<float>()));
    else
      return MakeFromAxisAndAngle(v0.CrossRH(ezVec3Template<Type>(0, 1, 0)).GetNormalized(), ezAngle::Radian(ezMath::Pi<float>()));
  }

  const ezVec3Template<Type> c = v0.CrossRH(v1);
  const Type d = v0.Dot(v1);
  const Type s = ezMath::Sqrt(((Type)1 + d) * (Type)2);

  EZ_ASSERT_DEBUG(c.IsValid(), "SetShortestRotation failed.");

  const Type fOneDivS = 1.0f / s;

  ezQuatTemplate<Type> q;
  q.x = c.x * fOneDivS;
  q.y = c.y * fOneDivS;
  q.z = c.z * fOneDivS;
  q.w = s / (Type)2;
  q.Normalize();

  return q;
}

template <typename Type>
ezQuatTemplate<Type> ezQuatTemplate<Type>::MakeSlerp(const ezQuatTemplate<Type>& qFrom, const ezQuatTemplate<Type>& qTo, Type t)
{
  EZ_ASSERT_DEBUG((t >= (Type)0) && (t <= (Type)1), "Invalid lerp factor.");

  const Type one = 1;
  const Type qdelta = (Type)1 - (Type)0.001;

  const Type fDot = (qFrom.x * qTo.x + qFrom.y * qTo.y + qFrom.z * qTo.z + qFrom.w * qTo.w);

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

  ezQuatTemplate<Type> q;

  q.x = t0 * qFrom.x;
  q.y = t0 * qFrom.y;
  q.z = t0 * qFrom.z;
  q.w = t0 * qFrom.w;

  q.x += t1 * qTo.x;
  q.y += t1 * qTo.y;
  q.z += t1 * qTo.z;
  q.w += t1 * qTo.w;

  q.Normalize();

  return q;
}

template <typename Type>
EZ_ALWAYS_INLINE bool operator==(const ezQuatTemplate<Type>& q1, const ezQuatTemplate<Type>& q2)
{
  return q1.x == q2.x && q1.y == q2.y && q1.z == q2.z && q1.w == q2.w;
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

  const double fSingularityTest = w * y - z * x;
  const double fSingularityThreshold = 0.4999995;

  if (fSingularityTest > fSingularityThreshold) // singularity at north pole
  {
    yaw = -2.0f * ezMath::ATan2(x, w);
    pitch = ezAngle::Degree(90.0f);
    roll = ezAngle::Degree(0.0f);
  }
  else if (fSingularityTest < -fSingularityThreshold) // singularity at south pole
  {
    yaw = 2.0f * ezMath::ATan2(x, w);
    pitch = ezAngle::Degree(-90.0f);
    roll = ezAngle::Degree(0.0f);
  }
  else
  {
    // yaw (z-axis rotation)
    const double siny = 2.0 * (w * z + x * y);
    const double cosy = 1.0 - 2.0 * (y * y + z * z);
    yaw = ezMath::ATan2((float)siny, (float)cosy);

    // pitch (y-axis rotation)
    pitch = ezMath::ASin(2.0f * (float)fSingularityTest);

    // roll (x-axis rotation)
    const double sinr = 2.0 * (w * x + y * z);
    const double cosr = 1.0 - 2.0 * (x * x + y * y);
    roll = ezMath::ATan2((float)sinr, (float)cosr);
  }
}

template <typename Type>
ezQuatTemplate<Type> ezQuatTemplate<Type>::MakeFromEulerAngles(const ezAngle& x, const ezAngle& y, const ezAngle& z)
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

  ezQuatTemplate<Type> q;
  q.w = (float)(cy * cp * cr + sy * sp * sr);
  q.x = (float)(cy * cp * sr - sy * sp * cr);
  q.y = (float)(cy * sp * cr + sy * cp * sr);
  q.z = (float)(sy * cp * cr - cy * sp * sr);
  return q;
}
