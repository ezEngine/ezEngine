#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/BakedProbes/BakingUtils.h>

ezVec3 ezBakingUtils::FibonacciSphere(ezUInt32 uiSampleIndex, ezUInt32 uiNumSamples)
{
  float offset = 2.0f / uiNumSamples;
  float increment = ezMath::Pi<float>() * (3.0f - ezMath::Sqrt(5.0f));

  float y = ((uiSampleIndex * offset) - 1) + (offset / 2);
  float r = ezMath::Sqrt(1 - y * y);

  ezAngle phi = ezAngle::MakeFromRadian(((uiSampleIndex + 1) % uiNumSamples) * increment);

  float x = ezMath::Cos(phi) * r;
  float z = ezMath::Sin(phi) * r;

  return ezVec3(x, y, z);
}

static ezUInt32 s_BitsPerDir[ezAmbientCubeBasis::NumDirs] = {5, 5, 5, 5, 6, 6};

ezCompressedSkyVisibility ezBakingUtils::CompressSkyVisibility(const ezAmbientCube<float>& skyVisibility)
{
  ezCompressedSkyVisibility result = 0;
  ezUInt32 uiOffset = 0;
  for (ezUInt32 i = 0; i < ezAmbientCubeBasis::NumDirs; ++i)
  {
    float maxValue = static_cast<float>((1u << s_BitsPerDir[i]) - 1u);
    ezUInt32 compressedDir = static_cast<ezUInt8>(ezMath::Saturate(skyVisibility.m_Values[i]) * maxValue + 0.5f);
    result |= (compressedDir << uiOffset);
    uiOffset += s_BitsPerDir[i];
  }

  return result;
}

void ezBakingUtils::DecompressSkyVisibility(ezCompressedSkyVisibility compressedSkyVisibility, ezAmbientCube<float>& out_skyVisibility)
{
  ezUInt32 uiOffset = 0;
  for (ezUInt32 i = 0; i < ezAmbientCubeBasis::NumDirs; ++i)
  {
    ezUInt32 maxValue = (1u << s_BitsPerDir[i]) - 1u;
    out_skyVisibility.m_Values[i] = static_cast<float>((compressedSkyVisibility >> uiOffset) & maxValue) * (1.0f / maxValue);
    uiOffset += s_BitsPerDir[i];
  }
}


