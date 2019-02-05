#pragma once

#include <Foundation/SimdMath/SimdFloat.h>
#include <Foundation/Containers/HybridArray.h>

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
    ezSimdFloat m_width;
};

/// \brief Box filter
class EZ_TEXTURE_DLL ezImageFilterBox : public ezImageFilter
{
public:
    ezImageFilterBox(float width = 0.5f);

    virtual ezSimdFloat SamplePoint(const ezSimdFloat& x) const override;
};

/// \brief Triangle filter
class EZ_TEXTURE_DLL ezImageFilterTriangle : public ezImageFilter
{
public:
    ezImageFilterTriangle(float width = 1.0f);

    virtual ezSimdFloat SamplePoint(const ezSimdFloat& x) const override;
};

/// \brief Kaiser-windowed sinc filter
class EZ_TEXTURE_DLL ezImageFilterSincWithKaiserWindow : public ezImageFilter
{
public:
    /// \brief Construct a sinc filter with a Kaiser window of the given window width and beta parameter.
    /// Note that the beta parameter (equaling alpha * pi in the mathematical definition of the Kaiser window) is often incorrectly alpha by other filtering tools.
    ezImageFilterSincWithKaiserWindow(float windowWidth = 3.0f, float beta = 4.0f);

    virtual ezSimdFloat SamplePoint(const ezSimdFloat& x) const override;

private:
    ezSimdFloat m_beta;
    ezSimdFloat m_invBesselBeta;
};

/// \brief Pre-computes the required filter weights for rescaling a sequence of image samples.
class EZ_TEXTURE_DLL ezImageFilterWeights
{
public:
    /// \brief Pre-compute the weights for the given filter for scaling between the given number of samples.
    ezImageFilterWeights(const ezImageFilter& filter, ezUInt32 srcSamples, ezUInt32 dstSamples);

    /// \brief Returns the number of weights.
    ezUInt32 GetNumWeights() const;

    /// \brief Returns the weight used for the source sample GetFirstSourceSampleIndex(dstSampleIndex) + weightIndex
    ezSimdFloat GetWeight(ezUInt32 dstSampleIndex, ezUInt32 weightIndex) const;

    /// \brief Returns the index of the first source sample that needs to be weighted to evaluate the destination sample
    inline ezInt32 GetFirstSourceSampleIndex(ezUInt32 dstSampleIndex) const;

    ezArrayPtr<const float> ViewWeights() const;

private:
    ezHybridArray<float, 16> m_weights;
    ezSimdFloat m_widthInSourceSpace;
    ezSimdFloat m_sourceToDestScale;
    ezSimdFloat m_destToSourceScale;
    ezUInt32 m_numWeights;
    ezUInt32 m_dstSamplesReduced;
};

#include <Texture/Image/Implementation/ImageFilter_inl.h>

