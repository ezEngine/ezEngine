#include <Foundation/FoundationPCH.h>

#include <Foundation/SimdMath/SimdQuat.h>

ezSimdQuat ezSimdQuat::MakeShortestRotation(const ezSimdVec4f& vDirFrom, const ezSimdVec4f& vDirTo)
{
  const ezSimdVec4f v0 = vDirFrom.GetNormalized<3>();
  const ezSimdVec4f v1 = vDirTo.GetNormalized<3>();

  const ezSimdFloat fDot = v0.Dot<3>(v1);

  // if both vectors are identical -> no rotation needed
  if (fDot.IsEqual(1.0f, 0.0001f))
  {
    return ezSimdQuat::MakeIdentity();
  }
  else if (fDot.IsEqual(-1.0f, 0.0001f)) // if both vectors are opposing
  {
    return ezSimdQuat::MakeFromAxisAndAngle(v0.GetOrthogonalVector().GetNormalized<3>(), ezAngle::MakeFromRadian(ezMath::Pi<float>()));
  }

  const ezSimdVec4f c = v0.CrossRH(v1);
  const ezSimdFloat s = ((fDot + ezSimdFloat(1.0f)) * ezSimdFloat(2.0f)).GetSqrt();

  ezSimdQuat res;
  res.m_v = c / s;
  res.m_v.SetW(s * ezSimdFloat(0.5f));
  res.Normalize();
  return res;
}

ezSimdQuat ezSimdQuat::MakeSlerp(const ezSimdQuat& qFrom, const ezSimdQuat& qTo, const ezSimdFloat& t)
{
  EZ_ASSERT_DEBUG((t >= 0.0f) && (t <= 1.0f), "Invalid lerp factor.");

  const ezSimdFloat one = 1.0f;
  const ezSimdFloat qdelta = 1.0f - 0.001f;

  const ezSimdFloat fDot = qFrom.m_v.Dot<4>(qTo.m_v);

  ezSimdFloat cosTheta = fDot;

  bool bFlipSign = false;
  if (cosTheta < 0.0f)
  {
    bFlipSign = true;
    cosTheta = -cosTheta;
  }

  ezSimdFloat t0, t1;

  if (cosTheta < qdelta)
  {
    ezAngle theta = ezMath::ACos(cosTheta);

    // use sqrtInv(1+c^2) instead of 1.0/sin(theta)
    const ezSimdFloat iSinTheta = (one - (cosTheta * cosTheta)).GetInvSqrt();
    const ezAngle tTheta = (float)t * theta;

    ezSimdFloat s0 = ezMath::Sin(theta - tTheta);
    ezSimdFloat s1 = ezMath::Sin(tTheta);

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

  ezSimdQuat res;
  res.m_v = qFrom.m_v * t0 + qTo.m_v * t1;
  res.Normalize();
  return res;
}

bool ezSimdQuat::IsEqualRotation(const ezSimdQuat& qOther, const ezSimdFloat& fEpsilon) const
{
  ezSimdVec4f vA1, vA2;
  ezSimdFloat fA1, fA2;

  if (GetRotationAxisAndAngle(vA1, fA1) == EZ_FAILURE)
    return false;
  if (qOther.GetRotationAxisAndAngle(vA2, fA2) == EZ_FAILURE)
    return false;

  ezAngle A1 = ezAngle::MakeFromRadian(fA1);
  ezAngle A2 = ezAngle::MakeFromRadian(fA2);

  if ((A1.IsEqualSimple(A2, ezAngle::MakeFromDegree(fEpsilon))) && (vA1.IsEqual(vA2, fEpsilon).AllSet<3>()))
    return true;

  if ((A1.IsEqualSimple(-A2, ezAngle::MakeFromDegree(fEpsilon))) && (vA1.IsEqual(-vA2, fEpsilon).AllSet<3>()))
    return true;

  return false;
}


