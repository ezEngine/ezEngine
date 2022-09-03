#include "../Vec3.h"
#include "../../Basics/Checks.h"
#include "../Math.h"
#include "../Quaternion.h"

namespace AE_NS_FOUNDATION
{
  const aeVec3 aeVec3::GetRefractedVector(const aeVec3& vNormal, float fRefIndex1, float fRefIndex2) const
  {
    const float n = fRefIndex1 / fRefIndex2;
    const float cosI = this->Dot(vNormal);
    const float sinT2 = n * n * (1.0f - (cosI * cosI));

    //invalid refraction
    if (sinT2 > 1.0f)
      return (*this);

    return ((n * (*this)) - (n + aeMath::Sqrt(1.0f - sinT2)) * vNormal);
  }

} // namespace AE_NS_FOUNDATION
