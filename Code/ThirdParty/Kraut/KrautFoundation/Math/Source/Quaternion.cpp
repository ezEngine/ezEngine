#include "../Matrix.h"
#include "../Quaternion.h"
#include "../../Basics/Checks.h"



namespace AE_NS_FOUNDATION
{
  const aeMatrix aeQuaternion::GetAsMatrix (void) const
  {
    aeMatrix m;

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
    return (m);
  }

  /*! \note This function will ALWAYS return a quaternion that rotates from one direction to another.
    If both directions are identical, it is the unit rotation (none). If they are exactly opposing, this will be
    ANY 180.0 rotation. That means the vectors will align perfectly, but there is no determine rotation for other points
    that might be rotated with this quaternion. If a main / fallback axis is needed to rotate points, you need to calculate
    such a rotation with other means.
  */
  void aeQuaternion::CreateQuaternion (const aeVec3& vDirFrom, const aeVec3& vDirTo)
  {
    const aeVec3 v0 = vDirFrom.GetNormalized ();
    const aeVec3 v1 = vDirTo.GetNormalized ();

    const float fDot = v0.Dot (v1);

    // if both vectors are identical -> no rotation needed
    if (aeMath::IsFloatEqual (fDot, 1.0f, 0.0001f))
    {
      SetIdentity ();
      return;
    }
    else
    // if both vectors are opposing
    if (aeMath::IsFloatEqual (fDot, -1.0f, 0.0001f))
    {
      // find an axis, that is not identical and not opposing, aeVec3::Cross-product to find perpendicular vector, rotate around that
      if (aeMath::Abs (v0.Dot (aeVec3::GetAxisX ())) < 0.8f)
        CreateQuaternion (v0.Cross (aeVec3::GetAxisX ()).GetNormalized (), 180.0f);
      else
        CreateQuaternion (v0.Cross (aeVec3::GetAxisY ()).GetNormalized (), 180.0f);

      return;
    }

    const aeVec3 c = v0.Cross (v1);
    const float d = v0.Dot (v1);
    const float s = aeMath::Sqrt ((1.0f + d) * 2.0f);

    AE_CHECK_DEV (c.IsValid (), "CreateQuaternion failed.");

    v = c / s;
    w = s * 0.5f;

    Normalize ();
  }
}


