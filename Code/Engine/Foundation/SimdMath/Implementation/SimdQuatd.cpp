#include <Foundation/FoundationPCH.h>

#include <Foundation/SimdMath/SimdQuatd.h>

ezSimdQuatd ezSimdQuatd::MakeShortestRotation(const ezSimdVec4d& vDirFrom, const ezSimdVec4d& vDirTo)
{
  const ezSimdVec4d v0 = vDirFrom.GetNormalized<3>();
  const ezSimdVec4d v1 = vDirTo.GetNormalized<3>();

  const ezSimdDouble fDot = v0.Dot<3>(v1);

  // if both vectors are identical -> no rotation needed
  if (fDot.IsEqual(1.0f, 0.0001f))
  {
    return ezSimdQuatd::MakeIdentity();
  }
  else if (fDot.IsEqual(-1.0f, 0.0001f)) // if both vectors are opposing
  {
    return ezSimdQuatd::MakeFromAxisAndAngle(v0.GetOrthogonalVector().GetNormalized<3>(), ezAngle::MakeFromRadian(ezMath::Pi<float>()));
  }

  const ezSimdVec4d c = v0.CrossRH(v1);
  const ezSimdDouble s = ((fDot + ezSimdDouble(1.0f)) * ezSimdDouble(2.0f)).GetSqrt();

  ezSimdQuatd res;
  res.m_v = c / s;
  res.m_v.SetW(s * ezSimdDouble(0.5f));
  res.Normalize();
  return res;
}

ezSimdQuatd ezSimdQuatd::MakeSlerp(const ezSimdQuatd& qFrom, const ezSimdQuatd& qTo, const ezSimdDouble& t)
{
  EZ_ASSERT_DEBUG((t >= 0.0f) && (t <= 1.0f), "Invalid lerp factor.");

  const ezSimdDouble one = 1.0f;
  const ezSimdDouble qdelta = 1.0f - 0.001f;

  const ezSimdDouble fDot = qFrom.m_v.Dot<4>(qTo.m_v);

  ezSimdDouble cosTheta = fDot;

  bool bFlipSign = false;
  if (cosTheta < 0.0f)
  {
    bFlipSign = true;
    cosTheta = -cosTheta;
  }

  ezSimdDouble t0, t1;

  if (cosTheta < qdelta)
  {
    ezAngle theta = ezMath::ACos(float(cosTheta));

    // use sqrtInv(1+c^2) instead of 1.0/sin(theta)
    const ezSimdDouble iSinTheta = (one - (cosTheta * cosTheta)).GetInvSqrt();
    const ezAngle tTheta = (float)t * theta;

    ezSimdDouble s0 = ezMath::Sin(theta - tTheta);
    ezSimdDouble s1 = ezMath::Sin(tTheta);

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

  ezSimdQuatd res;
  res.m_v = qFrom.m_v * t0 + qTo.m_v * t1;
  res.Normalize();
  return res;
}

bool ezSimdQuatd::IsEqualRotation(const ezSimdQuatd& qOther, const ezSimdDouble& fEpsilon) const
{
  ezSimdVec4d vA1, vA2;
  ezSimdDouble fA1, fA2;

  if (GetRotationAxisAndAngle(vA1, fA1) == EZ_FAILURE)
    return false;
  if (qOther.GetRotationAxisAndAngle(vA2, fA2) == EZ_FAILURE)
    return false;

  ezAngle A1 = ezAngle::MakeFromRadian(float(fA1));
  ezAngle A2 = ezAngle::MakeFromRadian(float(fA2));

  if ((A1.IsEqualSimple(A2, ezAngle::MakeFromDegree(float(fEpsilon)))) && (vA1.IsEqual(vA2, fEpsilon).AllSet<3>()))
    return true;

  if ((A1.IsEqualSimple(-A2, ezAngle::MakeFromDegree(float(fEpsilon)))) && (vA1.IsEqual(-vA2, fEpsilon).AllSet<3>()))
    return true;

  return false;
}
