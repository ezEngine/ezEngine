#include <Foundation/PCH.h>
#include <Foundation/Math/Vec3.h>

const ezVec3 ezVec3::GetRefractedVector (const ezVec3& vNormal, float fRefIndex1, float fRefIndex2) const
{
  EZ_ASSERT (vNormal.IsNormalized(), "vNormal must be normalized.");

  const float n = fRefIndex1 / fRefIndex2;
  const float cosI = this->Dot (vNormal);
  const float sinT2 = n * n * (1.0f - (cosI * cosI));

  //invalid refraction
  if (sinT2 > 1.0f)
    return (*this);

  return ((n * (*this)) - (n + ezMath::Sqrt (1.0f - sinT2)) * vNormal);
}
