#include "../Matrix.h"
#include "../Quaternion.h"
#include "../../Basics/Checks.h"

namespace AE_NS_FOUNDATION
{
  void aeMatrix::SetRotationMatrix (const aeVec3& vAxis, float fAngle)
  {
    const float rad = fAngle * aeMath_DegToRad;
    const float cos = aeMath::CosRad (rad);
    const float sin = aeMath::SinRad (rad);
    const float oneminuscos = 1 - cos;

    const float xy = vAxis.x * vAxis.y;
    const float xz = vAxis.x * vAxis.z;
    const float yz = vAxis.y * vAxis.z;

    const float xsin = vAxis.x * sin;
    const float ysin = vAxis.y * sin;
    const float zsin = vAxis.z * sin;

    const float onecos_xy = oneminuscos * xy;
    const float onecos_xz = oneminuscos * xz;
    const float onecos_yz = oneminuscos * yz;

    //Column 1
    m_fColumn[0][0] = cos + (oneminuscos * (vAxis.x * vAxis.x));
    m_fColumn[0][1] = onecos_xy + zsin;
    m_fColumn[0][2] = onecos_xz - ysin;
    m_fColumn[0][3] = 0;
             
    //Column 2
    m_fColumn[1][0] = onecos_xy - zsin;
    m_fColumn[1][1] = cos + (oneminuscos * (vAxis.y * vAxis.y));
    m_fColumn[1][2] = onecos_yz + xsin;
    m_fColumn[1][3] = 0;
             
    //Column 3
    m_fColumn[2][0] = onecos_xz + ysin;
    m_fColumn[2][1] = onecos_yz - xsin;
    m_fColumn[2][2] = cos + (oneminuscos * (vAxis.z * vAxis.z));
    m_fColumn[2][3] = 0;
             
    //Column 4
    m_fColumn[3][0] = 0;
    m_fColumn[3][1] = 0;
    m_fColumn[3][2] = 0;
    m_fColumn[3][3] = 1;
  }

  /*! The vLookDirY vector defines the Y-vector of rotated objects. The vOrthoDirZ vector is only a gross direction, it will be recalculated properly.
    Use this function, if you want to align objects along a specific Y-axis (with their Y-axis). 
    Make sure, that vLookDirY and vOrthoDirZ are not too parallel to each other.
  */
  void aeMatrix::SetLookAtMatrixY (const aeVec3& vPos, const aeVec3& vLookDirY, const aeVec3& vOrthoDirZ)
  {
    const aeVec3 vDirUp = vLookDirY.GetNormalized ();
    const aeVec3 vRight = vOrthoDirZ.Cross (vLookDirY).GetNormalized ();
    const aeVec3 vForwards = vRight.Cross (vLookDirY).GetNormalized ();

    m_fColumn[0][0] = vRight.x;
    m_fColumn[1][0] = vDirUp.x;
    m_fColumn[2][0] = vForwards.x;
    m_fColumn[3][0] = 0;
             
    m_fColumn[0][1] = vRight.y;
    m_fColumn[1][1] = vDirUp.y;
    m_fColumn[2][1] = vForwards.y;
    m_fColumn[3][1] = 0;
             
    m_fColumn[0][2] = vRight.z;
    m_fColumn[1][2] = vDirUp.z;
    m_fColumn[2][2] = vForwards.z;
    m_fColumn[3][2] = 0;
             
    m_fColumn[0][3] = -vPos.x;
    m_fColumn[1][3] = -vPos.y;
    m_fColumn[2][3] = -vPos.z;
    m_fColumn[3][3] = 1.0f;
  }

  /*! The vLookDirZ vector defines the Z-vector of rotated objects. The vOrhtoDirY vector is only a gross direction, it will be recalculated properly.
    Use this function, if you want to align objects along a specific Z-axis (with their Z-axis). For example, to let an object "look" along its own
    Z-axis at another object.
    Make sure, that vLookDirZ and vOrhtoDirY are not too parallel to each other.
  */
  void aeMatrix::SetLookAtMatrixZ (const aeVec3& vPos, const aeVec3& vLookDirZ, const aeVec3& vOrhtoDirY)
  {
    const aeVec3 vDir = vLookDirZ.GetNormalized ();
    const aeVec3 vRight = vLookDirZ.Cross (vOrhtoDirY).GetNormalized ();
    const aeVec3 vUp = vRight.Cross (vDir).GetNormalized ();

    m_fColumn[0][0] = vRight.x;
    m_fColumn[1][0] = vUp.x;
    m_fColumn[2][0] = -vDir.x;
    m_fColumn[3][0] = 0;
             
    m_fColumn[0][1] = vRight.y;
    m_fColumn[1][1] = vUp.y;
    m_fColumn[2][1] = -vDir.y;
    m_fColumn[3][1] = 0;
             
    m_fColumn[0][2] = vRight.z;
    m_fColumn[1][2] = vUp.z;
    m_fColumn[2][2] = -vDir.z;
    m_fColumn[3][2] = 0;
             
    m_fColumn[0][3] = -vPos.x;
    m_fColumn[1][3] = -vPos.y;
    m_fColumn[2][3] = -vPos.z;
    m_fColumn[3][3] = 1.0f;
  }

  void aeMatrix::SetObjectOrientationMatrixX (const aeVec3& vPos, const aeVec3& vAlignDirX, const aeVec3& vOrthoDirY)
  {
    const aeVec3 vDir = vAlignDirX.GetNormalized ();
    const aeVec3 vForward = vAlignDirX.Cross (vOrthoDirY).GetNormalized ();
    const aeVec3 vUp = vForward.Cross (vDir).GetNormalized ();

    m_fColumn[0][0] = vDir.x;
    m_fColumn[1][0] = vUp.x;
    m_fColumn[2][0] = vForward.x;
    m_fColumn[3][0] = vPos.x;
             
    m_fColumn[0][1] = vDir.y;
    m_fColumn[1][1] = vUp.y;
    m_fColumn[2][1] = vForward.y;
    m_fColumn[3][1] = vPos.y;
             
    m_fColumn[0][2] = vDir.z;
    m_fColumn[1][2] = vUp.z;
    m_fColumn[2][2] = vForward.z;
    m_fColumn[3][2] = vPos.z;
             
    m_fColumn[0][3] = 0;
    m_fColumn[1][3] = 0;
    m_fColumn[2][3] = 0;
    m_fColumn[3][3] = 1.0f;
  }

  void aeMatrix::SetObjectOrientationMatrixY (const aeVec3& vPos, const aeVec3& vAlignDirY, const aeVec3& vOrthoDirZ)
  {
    const aeVec3 vDirUp = vAlignDirY.GetNormalized ();
    const aeVec3 vRight = vAlignDirY.Cross (vOrthoDirZ).GetNormalized ();
    const aeVec3 vForwards = vRight.Cross (vAlignDirY).GetNormalized ();

    m_fColumn[0][0] = vRight.x;
    m_fColumn[1][0] = vDirUp.x;
    m_fColumn[2][0] = vForwards.x;
    m_fColumn[3][0] = vPos.x;
             
    m_fColumn[0][1] = vRight.y;
    m_fColumn[1][1] = vDirUp.y;
    m_fColumn[2][1] = vForwards.y;
    m_fColumn[3][1] = vPos.y;
             
    m_fColumn[0][2] = vRight.z;
    m_fColumn[1][2] = vDirUp.z;
    m_fColumn[2][2] = vForwards.z;
    m_fColumn[3][2] = vPos.z;
             
    m_fColumn[0][3] = 0;
    m_fColumn[1][3] = 0;
    m_fColumn[2][3] = 0;
    m_fColumn[3][3] = 1.0f;
  }

  void aeMatrix::SetObjectOrientationMatrixZ (const aeVec3& vPos, const aeVec3& vAlignDirZ, const aeVec3& vOrhtoDirY)
  {
    const aeVec3 vDir = vAlignDirZ.GetNormalized ();
    const aeVec3 vRight = vOrhtoDirY.Cross (vAlignDirZ).GetNormalized ();
    const aeVec3 vUp = vDir.Cross (vRight).GetNormalized ();

    m_fColumn[0][0] = vRight.x;
    m_fColumn[1][0] = vUp.x;
    m_fColumn[2][0] = vDir.x;
    m_fColumn[3][0] = vPos.x;
             
    m_fColumn[0][1] = vRight.y;
    m_fColumn[1][1] = vUp.y;
    m_fColumn[2][1] = vDir.y;
    m_fColumn[3][1] = vPos.y;
             
    m_fColumn[0][2] = vRight.z;
    m_fColumn[1][2] = vUp.z;
    m_fColumn[2][2] = vDir.z;
    m_fColumn[3][2] = vPos.z;
             
    m_fColumn[0][3] = 0;
    m_fColumn[1][3] = 0;
    m_fColumn[2][3] = 0;
    m_fColumn[3][3] = 1.0f;
  }

  const aeQuaternion aeMatrix::GetAsQuaternion (void) const
  {
    aeQuaternion q;

    float fTrace = m_fColumn[0][0] + m_fColumn[1][1] + m_fColumn[2][2];

    if (fTrace > 0.00001f) 
    {
      fTrace += 1.0f;

      const float s = aeMath::Sqrt (fTrace) * 2;
      q.v.x = (m_fColumn[1][2]  -  m_fColumn[2][1]) / s;
      q.v.y = (m_fColumn[2][0]  -  m_fColumn[0][2]) / s,
      q.v.z = (m_fColumn[0][1]  -  m_fColumn[1][0]) / s,
      q.w   = s / 4;
    } 
    else 
    {
      if (m_fColumn[0][0] > m_fColumn[1][1] && m_fColumn[0][0] > m_fColumn[2][2]) 
      {
        const float s = 2.0f * aeMath::Sqrt (1.0f + m_fColumn[0][0] - m_fColumn[1][1] - m_fColumn[2][2]);

        q.v.x = 0.25f * s;
        q.v.y = (m_fColumn[0][1] + m_fColumn[1][0]) / s;
        q.v.z = (m_fColumn[0][2] + m_fColumn[2][0]) / s;
        q.w   = (m_fColumn[1][2] - m_fColumn[2][1]) / s;    
      } 
      else 
      if (m_fColumn[1][1] > m_fColumn[2][2]) 
      {
        const float s = 2.0f * aeMath::Sqrt (1.0f + m_fColumn[1][1] - m_fColumn[0][0] - m_fColumn[2][2]);

        q.v.y = 0.25f * s;
        q.v.x = (m_fColumn[1][0] + m_fColumn[0][1]) / s;
        q.v.z = (m_fColumn[2][1] + m_fColumn[1][2]) / s;
        q.w   = (m_fColumn[2][0] - m_fColumn[0][2]) / s;
      }
      else 
      {
        const float s = 2.0f * aeMath::Sqrt (1.0f + m_fColumn[2][2] - m_fColumn[0][0] - m_fColumn[1][1]);

        q.v.z = 0.25f * s;
        q.v.x = (m_fColumn[0][2] + m_fColumn[2][0]) / s;
        q.v.y = (m_fColumn[1][2] + m_fColumn[2][1]) / s;
        q.w   = (m_fColumn[0][1] - m_fColumn[1][0]) / s;
      }
    }

    q.Normalize ();
    return (q);
  }

  const aeMatrix aeMatrix::GetInverse (void) const
  {
    aeMatrix Inverse;

    const float fDet = GetDeterminantOf4x4Matrix (*this);

    if (fDet == 0.0f)
    {
      AE_CHECK_DEV (false, "Determinant of Matrix is zero, cannot invert it.");
      return aeMatrix::ZeroMatrix ();
    }

    float fOneDivDet = aeMath::Invert (fDet);

    for (aeInt32 i = 0; i < 4; ++i)
    {
      
      Inverse.m_fColumn[i][0] = GetDeterminantOf3x3SubMatrix (*this, i, 0) * fOneDivDet;
      fOneDivDet = -fOneDivDet;
      Inverse.m_fColumn[i][1] = GetDeterminantOf3x3SubMatrix (*this, i, 1) * fOneDivDet;
      fOneDivDet = -fOneDivDet;
      Inverse.m_fColumn[i][2] = GetDeterminantOf3x3SubMatrix (*this, i, 2) * fOneDivDet;
      fOneDivDet = -fOneDivDet;
      Inverse.m_fColumn[i][3] = GetDeterminantOf3x3SubMatrix (*this, i, 3) * fOneDivDet;
    }

    return (Inverse);
  }
}


