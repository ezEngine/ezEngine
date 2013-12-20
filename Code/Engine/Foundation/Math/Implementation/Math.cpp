#include <Foundation/PCH.h>
#include <Foundation/Math/Math.h>

bool ezMath::IsPowerOf(ezInt32 value, ezInt32 base)
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

ezInt32 ezMath::PowerOfTwo_Floor(ezUInt32 npot)
{
  if (IsPowerOf2 (npot))
    return (npot);

  for (ezInt32 i = 1; i <= (sizeof (npot) * 8); ++i)
  {
    npot >>= 1;

    if (npot == 1)
      return (npot << i);
  }

  return (1);
}

ezInt32 ezMath::PowerOfTwo_Ceil(ezUInt32 npot)
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






EZ_STATICLINK_REFPOINT(Foundation_Math_Implementation_Math);

