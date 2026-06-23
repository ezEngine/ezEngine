#if MSAA
Texture2DMS<float4> SourceTexture BIND_GROUP(BG_DRAW_CALL);
#else
Texture2D SourceTexture BIND_GROUP(BG_DRAW_CALL);
#endif

float4 ReadTexel(int2 pixelPosition, int sampleIndex, uint sampleCount)
{
#if MSAA
  if (sampleIndex >= 0)
  {
    return SourceTexture.Load(pixelPosition, sampleIndex);
  }

  float4 result = 0.0f;
  for (uint sample = 0; sample < sampleCount; ++sample)
  {
    result += SourceTexture.Load(pixelPosition, int(sample));
  }
  return result / float(sampleCount);
#else
  return SourceTexture.Load(int3(pixelPosition, 0));
#endif
}

float4 ApplyValueRange(float4 value, float rangeMin, float rangeMax)
{
  float invRange = 1.0f / max(rangeMax - rangeMin, 0.0000001f);
  return (value - rangeMin) * invRange;
}