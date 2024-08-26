#include <Texture/TexturePCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/ImageConversion.h>
#include <Texture/Image/ImageUtils.h>
#include <Texture/TexConv/TexComparer.h>

ezTexComparer::ezTexComparer() = default;

ezResult ezTexComparer::Compare()
{
  EZ_PROFILE_SCOPE("Compare");

  EZ_SUCCEED_OR_RETURN(LoadInputImages());

  if ((m_Descriptor.m_ActualImage.GetWidth() != m_Descriptor.m_ExpectedImage.GetWidth()) ||
      (m_Descriptor.m_ActualImage.GetHeight() != m_Descriptor.m_ExpectedImage.GetHeight()))
  {
    ezLog::Error("Image sizes are not identical: {}x{} != {}x{}", m_Descriptor.m_ActualImage.GetWidth(), m_Descriptor.m_ActualImage.GetHeight(), m_Descriptor.m_ExpectedImage.GetWidth(), m_Descriptor.m_ExpectedImage.GetHeight());
    return EZ_FAILURE;
  }

  EZ_SUCCEED_OR_RETURN(ComputeMSE());

  if (m_OutputMSE > m_Descriptor.m_MeanSquareErrorThreshold)
  {
    m_bExceededMSE = true;

    EZ_SUCCEED_OR_RETURN(ExtractImages());
  }

  return EZ_SUCCESS;
}

ezResult ezTexComparer::LoadInputImages()
{
  EZ_PROFILE_SCOPE("Load Images");

  if (!m_Descriptor.m_sActualFile.IsEmpty())
  {
    if (m_Descriptor.m_ActualImage.LoadFrom(m_Descriptor.m_sActualFile).Failed())
    {
      ezLog::Error("Could not load image file '{0}'.", ezArgSensitive(m_Descriptor.m_sActualFile, "File"));
      return EZ_FAILURE;
    }
  }

  if (!m_Descriptor.m_sExpectedFile.IsEmpty())
  {
    if (m_Descriptor.m_ExpectedImage.LoadFrom(m_Descriptor.m_sExpectedFile).Failed())
    {
      ezLog::Error("Could not load reference file '{0}'.", ezArgSensitive(m_Descriptor.m_sExpectedFile, "File"));
      return EZ_FAILURE;
    }
  }

  if (!m_Descriptor.m_ActualImage.IsValid())
  {
    ezLog::Error("No image available.");
    return EZ_FAILURE;
  }

  if (!m_Descriptor.m_ExpectedImage.IsValid())
  {
    ezLog::Error("No reference image available.");
    return EZ_FAILURE;
  }

  if (m_Descriptor.m_ActualImage.GetImageFormat() == ezImageFormat::UNKNOWN)
  {
    ezLog::Error("Unknown image format for '{}'", ezArgSensitive(m_Descriptor.m_sActualFile, "File"));
    return EZ_FAILURE;
  }

  if (m_Descriptor.m_ExpectedImage.GetImageFormat() == ezImageFormat::UNKNOWN)
  {
    ezLog::Error("Unknown image format for '{}'", ezArgSensitive(m_Descriptor.m_sExpectedFile, "File"));
    return EZ_FAILURE;
  }

  if (ezImageConversion::Convert(m_Descriptor.m_ActualImage, m_Descriptor.m_ActualImage, ezImageFormat::R8G8B8A8_UNORM).Failed())
  {
    ezLog::Error("Could not convert to RGBA8: '{}'", ezArgSensitive(m_Descriptor.m_sActualFile, "File"));
    return EZ_FAILURE;
  }

  if (ezImageConversion::Convert(m_Descriptor.m_ExpectedImage, m_Descriptor.m_ExpectedImage, ezImageFormat::R8G8B8A8_UNORM).Failed())
  {
    ezLog::Error("Could not convert to RGBA8: '{}'", ezArgSensitive(m_Descriptor.m_sExpectedFile, "File"));
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult ezTexComparer::ComputeMSE()
{
  EZ_PROFILE_SCOPE("ComputeMSE");

  if (m_Descriptor.m_bRelaxedComparison)
    ezImageUtils::ComputeImageDifferenceABSRelaxed(m_Descriptor.m_ActualImage, m_Descriptor.m_ExpectedImage, m_OutputImageDiff);
  else
    ezImageUtils::ComputeImageDifferenceABS(m_Descriptor.m_ActualImage, m_Descriptor.m_ExpectedImage, m_OutputImageDiff);

  m_OutputMSE = ezImageUtils::ComputeMeanSquareError(m_OutputImageDiff, 32);

  return EZ_SUCCESS;
}

ezResult ezTexComparer::ExtractImages()
{
  EZ_PROFILE_SCOPE("ExtractImages");

  ezImageUtils::Normalize(m_OutputImageDiff, m_uiOutputMinDiffRgb, m_uiOutputMaxDiffRgb, m_uiOutputMinDiffAlpha, m_uiOutputMaxDiffAlpha);

  EZ_SUCCEED_OR_RETURN(ezImageConversion::Convert(m_OutputImageDiff, m_OutputImageDiffRgb, ezImageFormat::R8G8B8_UNORM));

  ezImageUtils::ExtractAlphaChannel(m_OutputImageDiff, m_OutputImageDiffAlpha);

  EZ_SUCCEED_OR_RETURN(ezImageConversion::Convert(m_Descriptor.m_ActualImage, m_ExtractedActualRgb, ezImageFormat::R8G8B8_UNORM));
  ezImageUtils::ExtractAlphaChannel(m_Descriptor.m_ActualImage, m_ExtractedActualAlpha);

  EZ_SUCCEED_OR_RETURN(ezImageConversion::Convert(m_Descriptor.m_ExpectedImage, m_ExtractedExpectedRgb, ezImageFormat::R8G8B8_UNORM));
  ezImageUtils::ExtractAlphaChannel(m_Descriptor.m_ExpectedImage, m_ExtractedExpectedAlpha);

  return EZ_SUCCESS;
}
