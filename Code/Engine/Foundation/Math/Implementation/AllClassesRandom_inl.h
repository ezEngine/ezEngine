#pragma once

#include <Foundation/Math/Angle.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Math/Vec3.h>

template <typename Type>
ezVec3Template<Type> ezVec3Template<Type>::CreateRandomPointInSphere(ezRandom& inout_rng)
{
  return MakeRandomPointInSphere(inout_rng);
}

template <typename Type>
ezVec3Template<Type> ezVec3Template<Type>::MakeRandomPointInSphere(ezRandom& inout_rng)
{
  double px, py, pz;
  double len = 0.0;

  do
  {
    px = inout_rng.DoubleMinMax(-1, 1);
    py = inout_rng.DoubleMinMax(-1, 1);
    pz = inout_rng.DoubleMinMax(-1, 1);

    len = (px * px) + (py * py) + (pz * pz);
  } while (len > 1.0 || len <= 0.000001); // prevent the exact center

  return ezVec3Template<Type>((Type)px, (Type)py, (Type)pz);
}

template <typename Type>
ezVec3Template<Type> ezVec3Template<Type>::CreateRandomDirection(ezRandom& inout_rng)
{
  return MakeRandomDirection(inout_rng);
}

template <typename Type>
ezVec3Template<Type> ezVec3Template<Type>::MakeRandomDirection(ezRandom& inout_rng)
{
  ezVec3Template<Type> vec = CreateRandomPointInSphere(inout_rng);
  vec.Normalize();
  return vec;
}

template <typename Type>
ezVec3Template<Type> ezVec3Template<Type>::CreateRandomDeviationX(ezRandom& inout_rng, const ezAngle& maxDeviation)
{
  return MakeRandomDeviationX(inout_rng, maxDeviation);
}

template <typename Type>
ezVec3Template<Type> ezVec3Template<Type>::MakeRandomDeviationX(ezRandom& inout_rng, const ezAngle& maxDeviation)
{
  const double twoPi = 2.0 * ezMath::Pi<double>();

  const double cosAngle = ezMath::Cos(maxDeviation);

  const double x = inout_rng.DoubleZeroToOneInclusive() * (1 - cosAngle) + cosAngle;
  const ezAngle phi = ezAngle::Radian((float)(inout_rng.DoubleZeroToOneInclusive() * twoPi));
  const double invSqrt = ezMath::Sqrt(1 - (x * x));
  const double y = invSqrt * ezMath::Cos(phi);
  const double z = invSqrt * ezMath::Sin(phi);

  return ezVec3Template<Type>((Type)x, (Type)y, (Type)z);
}

template <typename Type>
ezVec3Template<Type> ezVec3Template<Type>::CreateRandomDeviationY(ezRandom& inout_rng, const ezAngle& maxDeviation)
{
  return MakeRandomDeviationY(inout_rng, maxDeviation);
}

template <typename Type>
ezVec3Template<Type> ezVec3Template<Type>::MakeRandomDeviationY(ezRandom& inout_rng, const ezAngle& maxDeviation)
{
  ezVec3Template<Type> vec = CreateRandomDeviationX(inout_rng, maxDeviation);
  ezMath::Swap(vec.x, vec.y);
  return vec;
}

template <typename Type>
ezVec3Template<Type> ezVec3Template<Type>::CreateRandomDeviationZ(ezRandom& inout_rng, const ezAngle& maxDeviation)
{
  return MakeRandomDeviationZ(inout_rng, maxDeviation);
}

template <typename Type>
ezVec3Template<Type> ezVec3Template<Type>::MakeRandomDeviationZ(ezRandom& inout_rng, const ezAngle& maxDeviation)
{
  ezVec3Template<Type> vec = CreateRandomDeviationX(inout_rng, maxDeviation);
  ezMath::Swap(vec.x, vec.z);
  return vec;
}

template <typename Type>
ezVec3Template<Type> ezVec3Template<Type>::CreateRandomDeviation(ezRandom& inout_rng, const ezAngle& maxDeviation, const ezVec3Template<Type>& vNormal)
{
  return MakeRandomDeviation(inout_rng, maxDeviation, vNormal);
}

template <typename Type>
ezVec3Template<Type> ezVec3Template<Type>::MakeRandomDeviation(ezRandom& inout_rng, const ezAngle& maxDeviation, const ezVec3Template<Type>& vNormal)
{
  // If you need to do this very often:
  // *** Pre-compute this once: ***

  // how to get from the X axis to our desired basis
  ezQuatTemplate<Type> qRotXtoDir;
  qRotXtoDir.SetShortestRotation(ezVec3Template<Type>(1, 0, 0), vNormal);

  // *** Then call this with the precomputed value as often as needed: ***

  // create a random vector along X
  ezVec3Template<Type> vec = CreateRandomDeviationX(inout_rng, maxDeviation);
  // rotate from X to our basis
  return qRotXtoDir * vec;
}
