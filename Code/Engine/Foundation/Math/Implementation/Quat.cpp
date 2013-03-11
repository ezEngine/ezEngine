#include <Foundation/PCH.h>
#include <Foundation/Math/Mat4.h>
#include <Foundation/Math/Quat.h>

const ezMat3 ezQuat::GetAsMat3() const
{
  ezMat3 m;

  const float fTx  = 2.0f * v.x;
  const float fTy  = 2.0f * v.y;
  const float fTz  = 2.0f * v.z;
  const float fTwx = fTx * w;
  const float fTwy = fTy * w;
  const float fTwz = fTz * w;
  const float fTxx = fTx * v.x;
  const float fTxy = fTy * v.x;
  const float fTxz = fTz * v.x;
  const float fTyy = fTy * v.y;
  const float fTyz = fTz * v.y;
  const float fTzz = fTz * v.z;

  m.m_fColumn[0][0] = 1.0f - (fTyy + fTzz);
  m.m_fColumn[1][0] = fTxy - fTwz;
  m.m_fColumn[2][0] = fTxz + fTwy;
  m.m_fColumn[0][1] = fTxy + fTwz;
  m.m_fColumn[1][1] = 1.0f - (fTxx + fTzz);
  m.m_fColumn[2][1] = fTyz - fTwx;
  m.m_fColumn[0][2] = fTxz - fTwy;
  m.m_fColumn[1][2] = fTyz + fTwx;
  m.m_fColumn[2][2] = 1.0f - (fTxx + fTyy);
  return m;
}

const ezMat4 ezQuat::GetAsMat4() const
{
  ezMat4 m;

  const float fTx  = 2.0f * v.x;
  const float fTy  = 2.0f * v.y;
  const float fTz  = 2.0f * v.z;
  const float fTwx = fTx * w;
  const float fTwy = fTy * w;
  const float fTwz = fTz * w;
  const float fTxx = fTx * v.x;
  const float fTxy = fTy * v.x;
  const float fTxz = fTz * v.x;
  const float fTyy = fTy * v.y;
  const float fTyz = fTz * v.y;
  const float fTzz = fTz * v.z;

  m.m_fColumn[0][0] = 1.0f - (fTyy + fTzz);
  m.m_fColumn[1][0] = fTxy - fTwz;
  m.m_fColumn[2][0] = fTxz + fTwy;
  m.m_fColumn[3][0] = 0.0f;
  m.m_fColumn[0][1] = fTxy + fTwz;
  m.m_fColumn[1][1] = 1.0f - (fTxx + fTzz);
  m.m_fColumn[2][1] = fTyz - fTwx;
  m.m_fColumn[3][1] = 0.0f;
  m.m_fColumn[0][2] = fTxz - fTwy;
  m.m_fColumn[1][2] = fTyz + fTwx;
  m.m_fColumn[2][2] = 1.0f - (fTxx + fTyy);
  m.m_fColumn[3][2] = 0.0f;
  m.m_fColumn[0][3] = 0.0f;
  m.m_fColumn[1][3] = 0.0f;
  m.m_fColumn[2][3] = 0.0f;
  m.m_fColumn[3][3] = 1.0f;
  return m;
}

void ezQuat::SetFromMat3(const ezMat3& m)
{
  const float trace = m.m_fColumn[0][0] + m.m_fColumn[1][1] + m.m_fColumn[2][2];
  const float half = 0.5f;

  float val[4];

  if (trace > 0.0f)
  {
    float s = ezMath::Sqrt (trace + 1.0f);
    float t = half / s;

    val[0] = (m.m_fColumn[1][2] - m.m_fColumn[2][1]) * t;
    val[1] = (m.m_fColumn[2][0] - m.m_fColumn[0][2]) * t;
    val[2] = (m.m_fColumn[0][1] - m.m_fColumn[1][0]) * t;

    val[3] = half * s;
  }
  else
  {
    const ezInt32 next[] = {1,2,0};
    ezInt32 i = 0;

    if (m.m_fColumn[1][1] > m.m_fColumn[0][0]) 
      i = 1;

    if (m.m_fColumn[2][2] > m.m_fColumn[i][i]) 
      i = 2;

    ezInt32 j = next[i];
    ezInt32 k = next[j];

    float s = ezMath::Sqrt (m.m_fColumn[i][i] - (m.m_fColumn[j][j] + m.m_fColumn [k][k]) + 1.0f);
    float t = half / s;

    val[i] = half * s;
    val[3] = (m.m_fColumn[j][k] - m.m_fColumn[k][j]) * t;
    val[j] = (m.m_fColumn[i][j] + m.m_fColumn[j][i]) * t;
    val[k] = (m.m_fColumn[i][k] + m.m_fColumn[k][i]) * t;
  }

  v.x = val[0];
  v.y = val[1];
  v.z = val[2];
  w = val[3];
}

/*! \note This function will ALWAYS return a quaternion that rotates from one direction to another.
  If both directions are identical, it is the unit rotation (none). If they are exactly opposing, this will be
  ANY 180.0° rotation. That means the vectors will align perfectly, but there is no determine rotation for other points
  that might be rotated with this quaternion. If a main / fallback axis is needed to rotate points, you need to calculate
  such a rotation with other means.
*/
void ezQuat::SetShortestRotation(const ezVec3& vDirFrom, const ezVec3& vDirTo)
{
  const ezVec3 v0 = vDirFrom.GetNormalized ();
  const ezVec3 v1 = vDirTo.GetNormalized ();

  const float fDot = v0.Dot (v1);

  // if both vectors are identical -> no rotation needed
  if (ezMath::IsFloatEqual (fDot, 1.0f, 0.0001f))
  {
    SetIdentity ();
    return;
  }
  else
  // if both vectors are opposing
  if (ezMath::IsFloatEqual (fDot, -1.0f, 0.0001f))
  {
    // find an axis, that is not identical and not opposing, ezVec3::Cross-product to find perpendicular vector, rotate around that
    if (ezMath::Abs (v0.Dot (ezVec3(1, 0, 0))) < 0.8f)
      SetFromAxisAndAngle(v0.Cross (ezVec3(1, 0, 0)).GetNormalized (), 180.0f);
    else
      SetFromAxisAndAngle(v0.Cross (ezVec3(0, 1, 0)).GetNormalized (), 180.0f);

    return;
  }

  const ezVec3 c = v0.Cross (v1);
  const float d = v0.Dot (v1);
  const float s = ezMath::Sqrt ((1.0f + d) * 2.0f);

  EZ_ASSERT(c.IsValid (), "SetShortestRotation failed.");

  v = c / s;
  w = s * 0.5f;

  Normalize ();
}

void ezQuat::SetSlerp(const ezQuat& qFrom, const ezQuat& qTo, float t)
{
  EZ_ASSERT ((t >= 0.0f) && (t <= 1.0f), "Invalid lerp factor.");

  const float one = 1.0f;
  const float qdelta = 1.0f - 1e-3f;

  const float fDot = (qFrom.v.x * qTo.v.x + qFrom.v.y * qTo.v.y + qFrom.v.z * qTo.v.z + qFrom.w * qTo.w);

  float cosTheta = fDot;

  bool bFlipSign = false;
  if (cosTheta < 0.0f)
  {
    bFlipSign = true;
    cosTheta = -cosTheta;
  }

  float t0,t1;

  if (cosTheta < qdelta)
  {
    float theta = ezMath::ACosRad (cosTheta);

    // use sqrtInv(1+c^2) instead of 1.0/sin(theta) 
    const float iSinTheta = 1.0f / ezMath::Sqrt(one - (cosTheta*cosTheta));
    const float tTheta = t * theta;

    float s0 = ezMath::SinRad (theta-tTheta);
    float s1 = ezMath::SinRad (tTheta);

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

  Normalize ();

}

