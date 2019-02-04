#include <PCH.h>

#include <Texture/Image/ImageFilter.h>

ezSimdFloat ezImageFilter::GetWidth() const
{
  return m_width;
}

ezImageFilter::ezImageFilter(float width)
    : m_width(width)
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
    , m_beta(beta)
    , m_invBesselBeta(1.0f / modifiedBessel0(m_beta))
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
    return sinc(x * ezSimdFloat(ezMath::BasicType<float>::Pi())) * modifiedBessel0(m_beta * xSq.GetSqrt()) * m_invBesselBeta;
  }
}

ezImageFilterWeights::ezImageFilterWeights(const ezImageFilter& filter, ezUInt32 srcSamples, ezUInt32 dstSamples)
{
  // Filter weights repeat after the common phase
  ezUInt32 commonPhase = ezMath::GreatestCommonDivisor(srcSamples, dstSamples);

  srcSamples /= commonPhase;
  dstSamples /= commonPhase;

  m_dstSamplesReduced = dstSamples;

  m_sourceToDestScale = float(dstSamples) / float(srcSamples);
  m_destToSourceScale = float(srcSamples) / float(dstSamples);

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
    filterScale = m_destToSourceScale;
    invFilterScale = m_sourceToDestScale;
  }

  m_widthInSourceSpace = filter.GetWidth() * filterScale;

  m_numWeights = ezUInt32(ezMath::Ceil(m_widthInSourceSpace * ezSimdFloat(2.0f))) + 1;

  m_weights.SetCountUninitialized(dstSamples * m_numWeights);

  for (ezUInt32 dstSample = 0; dstSample < dstSamples; ++dstSample)
  {
    ezSimdFloat dstSampleInSourceSpace = (ezSimdFloat(dstSample) + ezSimdFloat(0.5f)) * m_destToSourceScale;

    ezInt32 firstSourceIdx = GetFirstSourceSampleIndex(dstSample);

    ezSimdFloat totalWeight = 0.0f;

    for (ezUInt32 weightIdx = 0; weightIdx < m_numWeights; ++weightIdx)
    {
      ezSimdFloat sourceSample = ezSimdFloat(firstSourceIdx + ezInt32(weightIdx)) + ezSimdFloat(0.5f);

      ezSimdFloat weight = filter.SamplePoint((dstSampleInSourceSpace - sourceSample) * invFilterScale);
      totalWeight += weight;
      m_weights[dstSample * m_numWeights + weightIdx] = weight;
    }

    // Normalize weights
    ezSimdFloat invWeight = 1.0f / totalWeight;

    for (ezUInt32 weightIdx = 0; weightIdx < m_numWeights; ++weightIdx)
    {
      m_weights[dstSample * m_numWeights + weightIdx] *= invWeight;
    }
  }
}

ezUInt32 ezImageFilterWeights::GetNumWeights() const
{
  return m_numWeights;
}

ezSimdFloat ezImageFilterWeights::GetWeight(ezUInt32 dstSampleIndex, ezUInt32 weightIndex) const
{
  EZ_ASSERT_DEBUG(weightIndex < m_numWeights, "Invalid weight index %i (should be < %i)", weightIndex, m_numWeights);

  return ezSimdFloat(m_weights[(dstSampleIndex % m_dstSamplesReduced) * m_numWeights + weightIndex]);
}

ezUInt32 ezImageAddressMode::GetSampleIndex(ezUInt32 numTexels, ezInt32 index, Enum addressMode, bool& outUseBorderColor) {
  outUseBorderColor = false;
  if (ezUInt32(index) >= numTexels)
  {
    switch (addressMode)
    {
      case ezImageAddressMode::WRAP:
        index %= numTexels;

        if (index < 0)
        {
          index += numTexels;
        }
        return index;

      case ezImageAddressMode::MIRROR:
      {
        if (index < 0)
        {
          index = -index - 1;
        }
        bool flip = (index / numTexels) & 1;
        index %= numTexels;
        if (flip)
        {
          index = numTexels - index - 1;
        }
        return index;
      }

      case ezImageAddressMode::CLAMP:
        return ezMath::Clamp<ezInt32>(index, 0, numTexels - 1);

      case ezImageAddressMode::MIRROR_ONCE:
        if (index < 0)
        {
          index = -index - 1;
        }
        index = ezMath::Clamp<ezInt32>(0, index, numTexels - 1);
        return index;

      case ezImageAddressMode::BORDER:
        outUseBorderColor = true;
        return 0;

      default:
        EZ_ASSERT_NOT_IMPLEMENTED
        return 0;
    }
  }
  return index;
}
