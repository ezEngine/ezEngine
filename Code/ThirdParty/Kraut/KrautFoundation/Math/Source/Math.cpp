#include "../Math.h"

namespace AE_NS_FOUNDATION
{
  float aeMath::Round (float f)
  {
    return aeMath::Floor (f + 0.5f);
  }

  float aeMath::Round (float f, float fRoundTo)
  {
    return Round (f / fRoundTo) * fRoundTo;
  }

  bool aeMath::IsPowerOf (aeInt32 value, aeInt32 base)
  {
    if (value == 1)
      return true;

    while (value > base)
    {
      if (value % base == 0)
        value /= base;
      else
        return false;
    }

    return (value == base);
  }

  aeInt32 aeMath::PowerOfTwo_Floor (aeUInt32 npot)
  {
    if (IsPowerOf2 (npot))
      return (npot);

    for (aeInt32 i = 1; i <= (sizeof (npot) * 8); ++i)
    {
      npot >>= 1;

      if (npot == 1)
        return (npot << i);
    }

    return (1);
  }

  aeInt32 aeMath::PowerOfTwo_Ceil (aeUInt32 npot)
  {
    if (IsPowerOf2 (npot))
      return (npot);

    for (int i=1; i <= (sizeof (npot) * 8); ++i)
    {
      npot >>= 1;

      if (npot == 1)
        return (npot << (i + 1));
    }

    return (1);
  }
}



