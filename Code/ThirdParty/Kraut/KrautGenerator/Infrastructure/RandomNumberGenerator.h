#pragma once

#include <KrautGenerator/KrautGeneratorDLL.h>

namespace Kraut
{
  /// \brief A very simple random number generator, that works like the standard "rand" function.
  class KRAUT_DLL RandomNumberGenerator
  {
  public:
    RandomNumberGenerator();
    ~RandomNumberGenerator();

    aeUInt32 m_uiSeedValue = 0;

    //! Returns a new random number. Changes the current seed value.
    aeUInt32 GetRandomNumber()
    {
      m_uiSeedValue = 214013L * m_uiSeedValue + 2531011L;
      return ((m_uiSeedValue >> 16) & 0x7FFFF);
    }

    //!  Returns a pseudo-random integer in range [0; max-possible].
    aeUInt32 Rand()
    {
      return GetRandomNumber();
    }

    //!  Returns a pseudo-random integer in range [0; max-1].
    aeUInt32 Rand(aeUInt32 max)
    {
      return GetRandomNumber() % max;
    }

    //!  Returns a pseudo-random integer in range [min; max-1].
    aeInt32 Rand(aeInt32 min, aeInt32 max)
    {
      return Rand_Range(min, max - min);
    }

    //!  Returns a pseudo-random integer in range [min; min + range-1].
    aeInt32 Rand_Range(aeInt32 min, aeUInt32 range)
    {
      if (range == 0)
        return min;

      return min + Rand(range);
    }

    //!  Returns a pseudo-random float in range [0; max). All values for "max" are allowed, even 0 and negative numbers.
    float Randf(float max)
    {
      return ((float(GetRandomNumber() % RAND_MAX - 1) / float(RAND_MAX - 2)) * max);
    }

    //!  Returns a pseudo-random float in range [min; max).
    float Randf(float min, float max)
    {
      return Rand_Rangef(min, max - min);
    }

    //!  Returns a pseudo-random float in range [min; min + range).
    float Rand_Rangef(float min, float range)
    {
      return min + Randf(range);
    }
  };

} // namespace Kraut
