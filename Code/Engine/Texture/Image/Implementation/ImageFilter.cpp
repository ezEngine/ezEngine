#include <Texture/TexturePCH.h>

#include <Texture/Image/ImageFilter.h>

ezSimdFloat ezImageFilter::GetWidth() const
{
  return m_Width;
}

ezImageFilter::ezImageFilter(float width)
  : m_Width(width)
{
}

ezImageFilterBox::ezImageFilterBox(float width)
  : ezImageFilter(width)
{
}

ezSimdFloat ezImageFilterBox::SamplePoint(const ezSimdFloat& x) const
{
  ezSimdFloat absX = x.Abs();

  if (absX <= GetWidth())
  {
    return 1.0f;
  }
  else
  {
    return 0.0f;
  }
}

ezImageFilterTriangle::ezImageFilterTriangle(float width)
  : ezImageFilter(width)
{
}

ezSimdFloat ezImageFilterTriangle::SamplePoint(const ezSimdFloat& x) const
{
  ezSimdFloat absX = x.Abs();

  ezSimdFloat width = GetWidth();

  if (absX <= width)
  {
    return width - absX;
  }
  else
  {
    return 0.0f;
  }
}

static ezSimdFloat sinc(const ezSimdFloat& x)
{
  ezSimdFloat absX = x.Abs();

  // Use Taylor expansion for small values to avoid division
  if (absX < 0.0001f)
  {
    // sin(x) / x = (x - x^3/6 + x^5/120 - ...) / x = 1 - x^2/6 + x^4/120 - ...
    return ezSimdFloat(1.0f) - x * x * ezSimdFloat(1.0f / 6.0f);
  }
  else
  {
    return ezMath::Sin(ezAngle::Radian(x)) / x;
  }
}

static ezSimdFloat modifiedBessel0(const ezSimdFloat& x)
{
  // Implementation as I0(x) = sum((1/4 * x * x) ^ k / (k!)^2, k, 0, inf), see
  // http://mathworld.wolfram.com/ModifiedBesselFunctionoftheFirstKind.html

  ezSimdFloat sum = 1.0f;

  ezSimdFloat xSquared = x * x * ezSimdFloat(0.25f);

  ezSimdFloat currentTerm = xSquared;

  for (ezUInt32 i = 2; currentTerm > 0.001f; ++i)
  {
    sum += currentTerm;
    currentTerm *= xSquared / ezSimdFloat(i * i);
  }

  return sum;
}

ezImageFilterSincWithKaiserWindow::ezImageFilterSincWithKaiserWindow(float width, float beta)
  : ezImageFilter(width)
  , m_Beta(beta)
  , m_InvBesselBeta(1.0f / modifiedBessel0(m_Beta))
{
}

ezSimdFloat ezImageFilterSincWithKaiserWindow::SamplePoint(const ezSimdFloat& x) const
{
  ezSimdFloat scaledX = x / GetWidth();

  ezSimdFloat xSq = 1.0f - scaledX * scaledX;

  if (xSq <= 0.0f)
  {
    return 0.0f;
  }
  else
  {
    return sinc(x * ezSimdFloat(ezMath::Pi<float>())) * modifiedBessel0(m_Beta * xSq.GetSqrt()) * m_InvBesselBeta;
  }
}

ezImageFilterWeights::ezImageFilterWeights(const ezImageFilter& filter, ezUInt32 srcSamples, ezUInt32 dstSamples)
{
  // Filter weights repeat after the common phase
  ezUInt32 commonPhase = ezMath::GreatestCommonDivisor(srcSamples, dstSamples);

  srcSamples /= commonPhase;
  dstSamples /= commonPhase;

  m_uiDstSamplesReduced = dstSamples;

  m_SourceToDestScale = float(dstSamples) / float(srcSamples);
  m_DestToSourceScale = float(srcSamples) / float(dstSamples);

  ezSimdFloat filterScale, invFilterScale;

  if (dstSamples > srcSamples)
  {
    // When upsampling, reconstruct the source by applying the filter in source space and resampling
    filterScale = 1.0f;
    invFilterScale = 1.0f;
  }
  else
  {
    // When downsampling, widen the filter in order to narrow its frequency spectrum, which effectively combines reconstruction + low-pass
    // filter
    filterScale = m_DestToSourceScale;
    invFilterScale = m_SourceToDestScale;
  }

  m_WidthInSourceSpace = filter.GetWidth() * filterScale;

  m_uiNumWeights = ezUInt32(ezMath::Ceil(m_WidthInSourceSpace * ezSimdFloat(2.0f))) + 1;

  m_Weights.SetCountUninitialized(dstSamples * m_uiNumWeights);

  for (ezUInt32 dstSample = 0; dstSample < dstSamples; ++dstSample)
  {
    ezSimdFloat dstSampleInSourceSpace = (ezSimdFloat(dstSample) + ezSimdFloat(0.5f)) * m_DestToSourceScale;

    ezInt32 firstSourceIdx = GetFirstSourceSampleIndex(dstSample);

    ezSimdFloat totalWeight = 0.0f;

    for (ezUInt32 weightIdx = 0; weightIdx < m_uiNumWeights; ++weightIdx)
    {
      ezSimdFloat sourceSample = ezSimdFloat(firstSourceIdx + ezInt32(weightIdx)) + ezSimdFloat(0.5f);

      ezSimdFloat weight = filter.SamplePoint((dstSampleInSourceSpace - sourceSample) * invFilterScale);
      totalWeight += weight;
      m_Weights[dstSample * m_uiNumWeights + weightIdx] = weight;
    }

    // Normalize weights
    ezSimdFloat invWeight = 1.0f / totalWeight;

    for (ezUInt32 weightIdx = 0; weightIdx < m_uiNumWeights; ++weightIdx)
    {
      m_Weights[dstSample * m_uiNumWeights + weightIdx] *= invWeight;
    }
  }
}

ezUInt32 ezImageFilterWeights::GetNumWeights() const
{
  return m_uiNumWeights;
}

ezSimdFloat ezImageFilterWeights::GetWeight(ezUInt32 dstSampleIndex, ezUInt32 weightIndex) const
{
  EZ_ASSERT_DEBUG(weightIndex < m_uiNumWeights, "Invalid weight index {} (should be < {})", weightIndex, m_uiNumWeights);

  return ezSimdFloat(m_Weights[(dstSampleIndex % m_uiDstSamplesReduced) * m_uiNumWeights + weightIndex]);
}



EZ_STATICLINK_FILE(Texture, Texture_Image_Implementation_ImageFilter);
