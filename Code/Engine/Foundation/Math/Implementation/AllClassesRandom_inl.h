#pragma once

#include <Foundation/Math/Vec3.h>
#include <Foundation/Math/Angle.h>
#include <Foundation/Math/Random.h>

template<typename Type>
ezVec3Template<Type> ezVec3Template<Type>::CreateRandomPointInSphere(ezRandom& rng)
{
  /// \test This is new

  double px, py, pz;
  double len = 0.0;

  do
  {
    px = rng.DoubleMinMax(-1, 1);
    py = rng.DoubleMinMax(-1, 1);
    pz = rng.DoubleMinMax(-1, 1);

    len = (px * px) + (py * py) + (pz * pz);
  }
  while (len > 1.0 || len <= 0.000001); // prevent the exact center

  return ezVec3Template<Type>((Type)px, (Type)py, (Type)pz);
}

template<typename Type>
ezVec3Template<Type> ezVec3Template<Type>::CreateRandomDirection(ezRandom& rng)
{
  /// \test This is new

  ezVec3Template<Type> vec = CreateRandomPointInSphere(rng);
  vec.Normalize();
  return vec;
}

template<typename Type>
ezVec3Template<Type> ezVec3Template<Type>::CreateRandomDeviationX(ezRandom& rng, const ezAngle& maxDeviation)
{
  /// \test This is new

  EZ_ASSERT_DEBUG(maxDeviation.GetRadian() > 0.0, "Deviation must not be zero");

  // two coordinates are always on a unit circle, the distance of the third coordinate defines the opening angle
  const double dist = 1.0 / ezMath::Tan(maxDeviation);

  double lenSqr;
  double p1;
  double p2;

  // create a random sample inside a unit sphere
  do
  {
    p1 = rng.DoubleMinMax(-1.0, 1.0);
    p2 = rng.DoubleMinMax(-1.0, 1.0);

    lenSqr = (p1 * p1 + p2 * p2);
  }
  while (lenSqr > 1.0);

  ezVec3Template<Type> vec((Type)dist, (Type)p1, (Type)p2);
  vec.Normalize();

  return vec;
}

template<typename Type>
ezVec3Template<Type> ezVec3Template<Type>::CreateRandomDeviationY(ezRandom& rng, const ezAngle& maxDeviation)
{
  /// \test This is new
  ezVec3Template<Type> vec = CreateRandomDeviationX(rng, maxDeviation);
  ezMath::Swap(vec.x, vec.y);
  return vec;
}

template<typename Type>
ezVec3Template<Type> ezVec3Template<Type>::CreateRandomDeviationZ(ezRandom& rng, const ezAngle& maxDeviation)
{
  /// \test This is new
  ezVec3Template<Type> vec = CreateRandomDeviationX(rng, maxDeviation);
  ezMath::Swap(vec.x, vec.z);
  return vec;
}

template<typename Type>
ezVec3Template<Type> ezVec3Template<Type>::CreateRandomDeviation(ezRandom& rng, const ezAngle& maxDeviation, const ezVec3& vNormal)
{
  /// \test This is new

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

