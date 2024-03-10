#include <Texture/TexturePCH.h>

#include <Texture/Image/ImageFilter.h>

ezSimdFloat ezImageFilter::GetWidth() const
{
  return m_fWidth;
}

ezImageFilter::ezImageFilter(float width)
  : m_fWidth(width)
{
}

ezImageFilterBox::ezImageFilterBox(float fWidth)
  : ezImageFilter(fWidth)
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

ezImageFilterTriangle::ezImageFilterTriangle(float fWidth)
  : ezImageFilter(fWidth)
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
    return ezMath::Sin(ezAngle::MakeFromRadian(x)) / x;
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

ezImageFilterSincWithKaiserWindow::ezImageFilterSincWithKaiserWindow(float fWidth, float fBeta)
  : ezImageFilter(fWidth)
  , m_fBeta(fBeta)
  , m_fInvBesselBeta(1.0f / modifiedBessel0(m_fBeta))
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
    return sinc(x * ezSimdFloat(ezMath::Pi<float>())) * modifiedBessel0(m_fBeta * xSq.GetSqrt()) * m_fInvBesselBeta;
  }
}

ezImageFilterWeights::ezImageFilterWeights(const ezImageFilter& filter, ezUInt32 uiSrcSamples, ezUInt32 uiDstSamples)
{
  // Filter weights repeat after the common phase
  ezUInt32 commonPhase = ezMath::GreatestCommonDivisor(uiSrcSamples, uiDstSamples);

  uiSrcSamples /= commonPhase;
  uiDstSamples /= commonPhase;

  m_uiDstSamplesReduced = uiDstSamples;

  m_fSourceToDestScale = float(uiDstSamples) / float(uiSrcSamples);
  m_fDestToSourceScale = float(uiSrcSamples) / float(uiDstSamples);

  ezSimdFloat filterScale, invFilterScale;

  if (uiDstSamples > uiSrcSamples)
  {
    // When upsampling, reconstruct the source by applying the filter in source space and resampling
    filterScale = 1.0f;
    invFilterScale = 1.0f;
  }
  else
  {
    // When downsampling, widen the filter in order to narrow its frequency spectrum, which effectively combines reconstruction + low-pass
    // filter
    filterScale = m_fDestToSourceScale;
    invFilterScale = m_fSourceToDestScale;
  }

  m_fWidthInSourceSpace = filter.GetWidth() * filterScale;

  m_uiNumWeights = ezUInt32(ezMath::Ceil(m_fWidthInSourceSpace * ezSimdFloat(2.0f))) + 1;

  m_Weights.SetCountUninitialized(uiDstSamples * m_uiNumWeights);

  for (ezUInt32 dstSample = 0; dstSample < uiDstSamples; ++dstSample)
  {
    ezSimdFloat dstSampleInSourceSpace = (ezSimdFloat(dstSample) + ezSimdFloat(0.5f)) * m_fDestToSourceScale;

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

ezSimdFloat ezImageFilterWeights::GetWeight(ezUInt32 uiDstSampleIndex, ezUInt32 uiWeightIndex) const
{
  EZ_ASSERT_DEBUG(uiWeightIndex < m_uiNumWeights, "Invalid weight index {} (should be < {})", uiWeightIndex, m_uiNumWeights);

  return ezSimdFloat(m_Weights[(uiDstSampleIndex % m_uiDstSamplesReduced) * m_uiNumWeights + uiWeightIndex]);
}
