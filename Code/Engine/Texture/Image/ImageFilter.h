#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/SimdMath/SimdFloat.h>
#include <Texture/TextureDLL.h>

/// \brief Represents a function used for filtering an image.
class EZ_TEXTURE_DLL ezImageFilter
{
public:
  /// \brief Samples the filter function at a single point. Note that the distribution isn't necessarily normalized.
  virtual ezSimdFloat SamplePoint(const ezSimdFloat& x) const = 0;

  /// \brief Returns the width of the filter; outside of the interval [-width, width], the filter function is always zero.
  ezSimdFloat GetWidth() const;

protected:
  ezImageFilter(float width);

private:
  ezSimdFloat m_fWidth;
};

/// \brief Box filter
class EZ_TEXTURE_DLL ezImageFilterBox : public ezImageFilter
{
public:
  ezImageFilterBox(float fWidth = 0.5f);

  virtual ezSimdFloat SamplePoint(const ezSimdFloat& x) const override;
};

/// \brief Triangle filter
class EZ_TEXTURE_DLL ezImageFilterTriangle : public ezImageFilter
{
public:
  ezImageFilterTriangle(float fWidth = 1.0f);

  virtual ezSimdFloat SamplePoint(const ezSimdFloat& x) const override;
};

/// \brief Kaiser-windowed sinc filter
class EZ_TEXTURE_DLL ezImageFilterSincWithKaiserWindow : public ezImageFilter
{
public:
  /// \brief Construct a sinc filter with a Kaiser window of the given window width and beta parameter.
  /// Note that the beta parameter (equaling alpha * pi in the mathematical definition of the Kaiser window) is often incorrectly alpha by other
  /// filtering tools.
  ezImageFilterSincWithKaiserWindow(float fWindowWidth = 3.0f, float fBeta = 4.0f);

  virtual ezSimdFloat SamplePoint(const ezSimdFloat& x) const override;

private:
  ezSimdFloat m_fBeta;
  ezSimdFloat m_fInvBesselBeta;
};

/// \brief Pre-computes the required filter weights for rescaling a sequence of image samples.
class EZ_TEXTURE_DLL ezImageFilterWeights
{
public:
  /// \brief Pre-compute the weights for the given filter for scaling between the given number of samples.
  ezImageFilterWeights(const ezImageFilter& filter, ezUInt32 uiSrcSamples, ezUInt32 uiDstSamples);

  /// \brief Returns the number of weights.
  ezUInt32 GetNumWeights() const;

  /// \brief Returns the weight used for the source sample GetFirstSourceSampleIndex(dstSampleIndex) + weightIndex
  ezSimdFloat GetWeight(ezUInt32 uiDstSampleIndex, ezUInt32 uiWeightIndex) const;

  /// \brief Returns the index of the first source sample that needs to be weighted to evaluate the destination sample
  inline ezInt32 GetFirstSourceSampleIndex(ezUInt32 uiDstSampleIndex) const;

  ezArrayPtr<const float> ViewWeights() const;

private:
  ezHybridArray<float, 16> m_Weights;
  ezSimdFloat m_fWidthInSourceSpace;
  ezSimdFloat m_fSourceToDestScale;
  ezSimdFloat m_fDestToSourceScale;
  ezUInt32 m_uiNumWeights;
  ezUInt32 m_uiDstSamplesReduced;
};

#include <Texture/Image/Implementation/ImageFilter_inl.h>
