#pragma once

#include "../Common/ConstantBufferMacros.h"
#include "../Common/Platforms.h"

#define TONEMAPMODE_NONE 0
#define TONEMAPMODE_LINEAR 1
#define TONEMAPMODE_REINHARD 2
#define TONEMAPMODE_FILMIC 3
#define TONEMAPMODE_ACES 4
#define TONEMAPMODE_AGX 5

CONSTANT_BUFFER(ezTonemapConstants, 3)
{
  FLOAT4(AutoExposureParams);

  COLOR4F(MoodColor);
  FLOAT1(MoodStrength);
  FLOAT1(Saturation);
  FLOAT1(Lut1Strength);
  FLOAT1(Lut2Strength);
  FLOAT4(ContrastParams);

  INT1(TonemapMode);
  FLOAT1(WhitePoint);
  FLOAT1(TonemapNormalization); // Derived on the CPU from the tonemap mode and white point

  FLOAT1(FilmicToe);
  FLOAT1(FilmicLinearStrength);
  FLOAT1(FilmicLinearAngle);
  FLOAT1(FilmicShoulder);

  FLOAT1(AcesToe);
  FLOAT1(AcesSlope);
  FLOAT1(AcesShoulder);
};

#if EZ_ENABLED(PLATFORM_SHADER)
#  define COLOR_TYPE float3
#else
#  define COLOR_TYPE float
#  define saturate(x) ezMath::Saturate(x)
#  define lerp(a, b, t) ezMath::Lerp(a, b, t)
#  define max(a, b) ezMath::Max(a, b)
#endif

COLOR_TYPE ReinhardFunction(COLOR_TYPE color, float normalization)
{
  // Could be set to greater than 1.0 for HDR output
  float OutputMaxValue = 1.0;
  return color * (1.0f + color * normalization) / (1.0f + color / OutputMaxValue);
}

// From http://filmicgames.com/archives/75
COLOR_TYPE HableFunction(COLOR_TYPE x, float toe, float linearStrength, float linearAngle, float shoulder)
{
  float A = shoulder;
  float B = linearStrength;
  float C = linearAngle;
  float D = toe;
  float E = 0.02;
  float F = 0.30;

  return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

COLOR_TYPE ACESFunction(COLOR_TYPE x, float toe, float slope, float shoulder)
{
#if 0
  // Adapted from https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl
  const float A = 0.0245786f;
  const float B = 0.000090537f;
  const float C = 0.983729f;
  const float D = 0.432951f;
  const float E = 0.238081f;

  COLOR_TYPE toneColor = (x * (x + A) - B) / (x * (C * x + D) + E);

#else
  // Adapted from https://www.desmos.com/calculator/h8rbdpawxj
  const float blackClip = 0.0;
  const float whiteClip = 0.04;

  const float toeScale = 1 + blackClip - toe;
  const float shoulderScale = 1 + whiteClip - shoulder;

  const float inMatch = 0.18;
  const float outMatch = 0.18;

  float toeCutoff;
  if (toe > 0.8)
  {
    // 0.18 will be on straight segment
    toeCutoff = (1 - toe - outMatch) / slope + log10(inMatch);
  }
  else
  {
    // 0.18 will be on toe segment

    // Solve for toeCutoff such that input of inMatch gives output of outMatch.
    const float bt = (outMatch + blackClip) / toeScale - 1;
    toeCutoff = log10(inMatch) - 0.5 * log((1 + bt) / (1 - bt)) * (toeScale / slope);
  }

  float StraightMatch = (1 - toe) / slope - toeCutoff;
  float ShoulderMatch = shoulder / slope - StraightMatch;

  COLOR_TYPE logColor = log10(x * 0.7);
  COLOR_TYPE StraightColor = slope * (logColor + StraightMatch);

  COLOR_TYPE toeColor = (-blackClip) + (2 * toeScale) / (1 + exp((-2 * slope / toeScale) * (logColor - toeCutoff)));
  COLOR_TYPE shoulderColor = (1 + whiteClip) - (2 * shoulderScale) / (1 + exp((2 * slope / shoulderScale) * (logColor - ShoulderMatch)));

  toeColor = logColor < toeCutoff ? toeColor : StraightColor;
  shoulderColor = logColor > ShoulderMatch ? shoulderColor : StraightColor;

  COLOR_TYPE t = saturate((logColor - toeCutoff) / (ShoulderMatch - toeCutoff));
  t = ShoulderMatch < toeCutoff ? 1 - t : t;
  t = (3 - 2 * t) * t * t;
  COLOR_TYPE toneColor = lerp(toeColor, shoulderColor, t);
#endif

  return max(toneColor, 0.0f);
}

// Undefine macros that are only used in this file
#undef COLOR_TYPE

#ifdef saturate
#  undef saturate
#endif

#ifdef lerp
#  undef lerp
#endif

#ifdef max
#  undef max
#endif
