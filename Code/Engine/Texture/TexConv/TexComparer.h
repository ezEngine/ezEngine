#pragma once

/// \brief Input options for ezTexComparer
class EZ_TEXTURE_DLL ezTexCompareDesc
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezTexCompareDesc);

public:
  ezTexCompareDesc() = default;

  /// Path to a file to load as a reference image. Optional, if m_ExpectedImage is already filled out.
  ezString m_sExpectedFile;

  /// Path to a file to load as the input image. Optional, if m_ActualImage is already filled out.
  ezString m_sActualFile;

  /// The reference image to compare. Ignored if m_sExpectedFile is filled out.
  ezImage m_ExpectedImage;

  /// The image to compare. Ignored if m_sActualFile is filled out.
  ezImage m_ActualImage;

  /// If enabled, the image comparison allows for more wiggle room.
  /// For images containing single-pixel rasterized lines.
  bool m_bRelaxedComparison = false;

  /// If the comparison yields a larger MSE than this, the images are considered to be too different.
  ezUInt32 m_MeanSquareErrorThreshold = 100;
};

/// \brief Compares two images and generates various outputs.
class EZ_TEXTURE_DLL ezTexComparer
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezTexComparer);

public:
  ezTexComparer();

  /// The input data to compare.
  ezTexCompareDesc m_Descriptor;

  /// Executes the comparison and fill out the public variables to describe the result.
  ezResult Compare();

  /// If true, the mean-square error of the difference was larger than the threshold.
  bool m_bExceededMSE = false;
  /// The MSE of the difference image.
  ezUInt32 m_OutputMSE = 0;

  /// The (normalized) difference image.
  ezImage m_OutputImageDiff;
  /// Only the RGB part of the (normalized) difference image.
  ezImage m_OutputImageDiffRgb;
  /// Only the Alpha part of the (normalized) difference image.
  ezImage m_OutputImageDiffAlpha;

  /// Only the RGB part of the actual input image.
  ezImage m_ExtractedActualRgb;
  /// Only the RGB part of the reference input image.
  ezImage m_ExtractedExpectedRgb;
  /// Only the Alpha part of the actual input image.
  ezImage m_ExtractedActualAlpha;
  /// Only the Alpha part of the reference input image.
  ezImage m_ExtractedExpectedAlpha;

  /// Min/Max difference of the RGB and Alpha images.
  ezUInt8 m_uiOutputMinDiffRgb = 0;
  ezUInt8 m_uiOutputMaxDiffRgb = 0;
  ezUInt8 m_uiOutputMinDiffAlpha = 0;
  ezUInt8 m_uiOutputMaxDiffAlpha = 0;

private:
  ezResult LoadInputImages();
  ezResult ComputeMSE();
  ezResult ExtractImages();
};
