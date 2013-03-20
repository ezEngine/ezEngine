#include <Foundation/PCH.h>
#include <Foundation/Math/Mat3.h>

void ezMat3::SetRotationMatrix (const ezVec3& vAxis, float fAngle)
{
  EZ_ASSERT(vAxis.IsNormalized(), "vAxis must be normalized.");

  const float rad = ezMath::DegToRad(fAngle);
  const float cos = ezMath::CosRad (rad);
  const float sin = ezMath::SinRad (rad);
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
             
  //Column 2
  m_fColumn[1][0] = onecos_xy - zsin;
  m_fColumn[1][1] = cos + (oneminuscos * (vAxis.y * vAxis.y));
  m_fColumn[1][2] = onecos_yz + xsin;
             
  //Column 3
  m_fColumn[2][0] = onecos_xz + ysin;
  m_fColumn[2][1] = onecos_yz - xsin;
  m_fColumn[2][2] = cos + (oneminuscos * (vAxis.z * vAxis.z));
}

ezResult ezMat3::Invert(float fEpsilon)
{
  const float fDet = m_fColumn[0][0] * (m_fColumn[2][2] * m_fColumn[1][1] - m_fColumn[1][2] * m_fColumn[2][1]) -
                     m_fColumn[0][1] * (m_fColumn[2][2] * m_fColumn[1][0] - m_fColumn[1][2] * m_fColumn[2][0]) +
                     m_fColumn[0][2] * (m_fColumn[2][1] * m_fColumn[1][0] - m_fColumn[1][1] * m_fColumn[2][0]);

  if (ezMath::IsZero (fDet, fEpsilon))
    return EZ_FAILURE;

  const float fOneDivDet = 1.0f / fDet;

  ezMat3 Inverse;

  Inverse.m_fColumn[0][0] = (m_fColumn[2][2] * m_fColumn[1][1] - m_fColumn[1][2] * m_fColumn[2][1]);
  Inverse.m_fColumn[0][1] =-(m_fColumn[2][2] * m_fColumn[0][1] - m_fColumn[0][2] * m_fColumn[2][1]);
  Inverse.m_fColumn[0][2] = (m_fColumn[1][2] * m_fColumn[0][1] - m_fColumn[0][2] * m_fColumn[1][1]);
                                                                                              
  Inverse.m_fColumn[1][0] =-(m_fColumn[2][2] * m_fColumn[1][0] - m_fColumn[1][2] * m_fColumn[2][0]);
  Inverse.m_fColumn[1][1] = (m_fColumn[2][2] * m_fColumn[0][0] - m_fColumn[0][2] * m_fColumn[2][0]);
  Inverse.m_fColumn[1][2] =-(m_fColumn[1][2] * m_fColumn[0][0] - m_fColumn[0][2] * m_fColumn[1][0]);
                                                                                              
  Inverse.m_fColumn[2][0] = (m_fColumn[2][1] * m_fColumn[1][0] - m_fColumn[1][1] * m_fColumn[2][0]);
  Inverse.m_fColumn[2][1] =-(m_fColumn[2][1] * m_fColumn[0][0] - m_fColumn[0][1] * m_fColumn[2][0]);
  Inverse.m_fColumn[2][2] = (m_fColumn[1][1] * m_fColumn[0][0] - m_fColumn[0][1] * m_fColumn[1][0]);

  *this = Inverse * fOneDivDet;
  return EZ_SUCCESS;
}

void ezMat3::SetLookInDirectionMatrix (ezVec3 vLookDir, ezVec3 vUpDir)
{
  EZ_ASSERT(!vLookDir.IsZero(),"The look direction must not be zero.");
  EZ_ASSERT(vUpDir.IsNormalized(), "The up-direction must be normalized.");

  vLookDir.NormalizeIfNotZero();

  if (ezMath::Abs(vLookDir.Dot(vUpDir)) > 0.9999f) // less than 1 degree difference -> problem
    vUpDir = vLookDir.GetOrthogonalVector();

  // ensure vUpDir is orthogonal to the looking direction
  const ezVec3 vRightDir = vLookDir.Cross(vUpDir).GetNormalized ();

  // orthogonalize vUpDir
  vUpDir = vRightDir.Cross(vLookDir); // vLookDir and vRightDir are normalized and orthogonal, vUpDir will be normalized too

  SetColumn(0, vRightDir);
  SetColumn(1, vUpDir);
  SetColumn(2, vLookDir);

  /// \todo At some point we need to define a default coordinate system to use.
  /// I don't really know whether this code works (or better: how).
  /// It should probably use an OpenGL or DirectX coordinate system, not sure what exactly.
  /// The first guy to use it, will find it out.
}
