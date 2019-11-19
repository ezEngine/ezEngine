#include <BakingPluginPCH.h>

#include <BakingPlugin/Tasks/Utils.h>

ezVec3 ezBakingInternal::FibonacciSphere(int sampleIndex, int numSamples)
{
  float offset = 2.0f / numSamples;
  float increment = ezMath::Pi<float>() * (3.0f - ezMath::Sqrt(5.0f));

  float y = ((sampleIndex * offset) - 1) + (offset / 2);
  float r = ezMath::Sqrt(1 - y * y);

  ezAngle phi = ezAngle::Radian(((sampleIndex + 1) % numSamples) * increment);

  float x = ezMath::Cos(phi) * r;
  float z = ezMath::Sin(phi) * r;

  return ezVec3(x, y, z);
}
