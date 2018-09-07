#pragma once

#include <Foundation/Math/Angle.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Math/Vec3.h>

template <typename Type>
ezVec3Template<Type> ezVec3Template<Type>::CreateRandomPointInSphere(ezRandom& rng)
{
  double px, py, pz;
  double len = 0.0;

  do
  {
    px = rng.DoubleMinMax(-1, 1);
    py = rng.DoubleMinMax(-1, 1);
    pz = rng.DoubleMinMax(-1, 1);

    len = (px * px) + (py * py) + (pz * pz);
  } while (len > 1.0 || len <= 0.000001); // prevent the exact center

  return ezVec3Template<Type>((Type)px, (Type)py, (Type)pz);
}

template <typename Type>
ezVec3Template<Type> ezVec3Template<Type>::CreateRandomDirection(ezRandom& rng)
{
  ezVec3Template<Type> vec = CreateRandomPointInSphere(rng);
  vec.Normalize();
  return vec;
}

template <typename Type>
ezVec3Template<Type> ezVec3Template<Type>::CreateRandomDeviationX(ezRandom& rng, const ezAngle& maxDeviation)
{
  const double twoPi = 2.0 * ezMath::BasicType<double>::Pi();

  const double cosAngle = ezMath::Cos(maxDeviation);

  const double x = rng.DoubleZeroToOneInclusive() * (1 - cosAngle) + cosAngle;
  const ezAngle phi = ezAngle::Radian((float)(rng.DoubleZeroToOneInclusive() * twoPi));
  const double invSqrt = ezMath::Sqrt(1 - (x * x));
  const double y = invSqrt * ezMath::Cos(phi);
  const double z = invSqrt * ezMath::Sin(phi);

  return ezVec3((float)x, (float)y, (float)z);
}

template <typename Type>
ezVec3Template<Type> ezVec3Template<Type>::CreateRandomDeviationY(ezRandom& rng, const ezAngle& maxDeviation)
{
  ezVec3Template<Type> vec = CreateRandomDeviationX(rng, maxDeviation);
  ezMath::Swap(vec.x, vec.y);
  return vec;
}

template <typename Type>
ezVec3Template<Type> ezVec3Template<Type>::CreateRandomDeviationZ(ezRandom& rng, const ezAngle& maxDeviation)
{
  ezVec3Template<Type> vec = CreateRandomDeviationX(rng, maxDeviation);
  ezMath::Swap(vec.x, vec.z);
  return vec;
}

template <typename Type>
ezVec3Template<Type> ezVec3Template<Type>::CreateRandomDeviation(ezRandom& rng, const ezAngle& maxDeviation, const ezVec3& vNormal)
{
  // If you need to do this very often:
  // *** Pre-compute this once: ***

  // how to get from the X axis to our desired basis
  ezQuatTemplate<Type> qRotXtoDir;
  qRotXtoDir.SetShortestRotation(ezVec3Template<Type>(1, 0, 0), vNormal);

  // *** Then call this with the precomputed value as often as needed: ***

  // create a random vector along X
  ezVec3Template<Type> vec = CreateRandomDeviationX(rng, maxDeviation);
  // rotate from X to our basis
  return qRotXtoDir * vec;
}
