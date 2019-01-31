#include <PCH.h>

#include <TexConvLib/Processing/Processor.h>

ezTexConvProcessor::ezTexConvProcessor()
{
  m_pCurrentScratchImage = &m_ScratchImage1;
  m_pOtherScratchImage = &m_ScratchImage2;
}

ezResult ezTexConvProcessor::Process()
{
  EZ_SUCCEED_OR_RETURN(LoadInputImages());

  EZ_SUCCEED_OR_RETURN(AdjustTargetFormat());

  EZ_SUCCEED_OR_RETURN(ChooseOutputFormat());

  EZ_SUCCEED_OR_RETURN(DetermineTargetResolution());

  EZ_SUCCEED_OR_RETURN(ConvertInputImagesToFloat32());

  EZ_SUCCEED_OR_RETURN(ResizeInputImagesToSameDimensions());

  EZ_SUCCEED_OR_RETURN(Assemble2DTexture());

  EZ_SUCCEED_OR_RETURN(GenerateMipmaps());

  EZ_SUCCEED_OR_RETURN(GenerateOutput());

  EZ_SUCCEED_OR_RETURN(GenerateThumbnailOutput());

  EZ_SUCCEED_OR_RETURN(GenerateLowResOutput());

  return EZ_SUCCESS;
}

ezResult ezTexConvProcessor::GenerateOutput()
{
  m_OutputImage.ResetAndMove(std::move(*m_pCurrentScratchImage));

  if (m_OutputImage.Convert(m_OutputImageFormat).Failed())
  {
    ezLog::Error("Failed to convert result image to final output format '{}'", ezImageFormat::GetName(m_OutputImageFormat));
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult ezTexConvProcessor::GenerateThumbnailOutput()
{
  const ezUInt32 uiTargetRes = m_Descriptor.m_uiThumbnailOutputResolution;
  if (uiTargetRes == 0)
    return EZ_SUCCESS;

  ezUInt32 uiBestMip = 0;

  for (ezUInt32 m = 0; m < m_OutputImage.GetNumMipLevels(); ++m)
  {
    if (m_OutputImage.GetWidth(m) <= uiTargetRes && m_OutputImage.GetHeight(m) <= uiTargetRes)
    {
      uiBestMip = m;
      break;
    }

    uiBestMip = m;
  }

  m_pCurrentScratchImage->ResetAndCopy(m_OutputImage.GetSubImageView(uiBestMip));

  if (m_pCurrentScratchImage->GetWidth() > uiTargetRes || m_pCurrentScratchImage->GetHeight() > uiTargetRes)
  {
    if (m_pCurrentScratchImage->GetWidth() > m_pCurrentScratchImage->GetHeight())
    {
      const float fAspectRatio = (float)m_pCurrentScratchImage->GetWidth() / (float)uiTargetRes;
      ezUInt32 uiTargetHeight = (ezUInt32)(m_pCurrentScratchImage->GetHeight() / fAspectRatio);

      uiTargetHeight = ezMath::Max(uiTargetHeight, 4U);

      if (ezImageUtils::Scale(*m_pCurrentScratchImage, *m_pOtherScratchImage, uiTargetRes, uiTargetHeight).Failed())
      {
        ezLog::Error("Failed to resize thumbnail image from {}x{} to {}x{}", m_pCurrentScratchImage->GetWidth(),
          m_pCurrentScratchImage->GetHeight(), uiTargetRes, uiTargetHeight);
        return EZ_FAILURE;
      }
    }
    else
    {
      const float fAspectRatio = (float)m_pCurrentScratchImage->GetHeight() / (float)uiTargetRes;
      ezUInt32 uiTargetWidth = (ezUInt32)(m_pCurrentScratchImage->GetWidth() / fAspectRatio);

      uiTargetWidth = ezMath::Max(uiTargetWidth, 4U);

      if (ezImageUtils::Scale(*m_pCurrentScratchImage, *m_pOtherScratchImage, uiTargetWidth, uiTargetRes).Failed())
      {
        ezLog::Error("Failed to resize thumbnail image from {}x{} to {}x{}", m_pCurrentScratchImage->GetWidth(),
          m_pCurrentScratchImage->GetHeight(), uiTargetWidth, uiTargetRes);
        return EZ_FAILURE;
      }
    }

    ezMath::Swap(m_pCurrentScratchImage, m_pOtherScratchImage);
  }

  m_ThumbnailOutputImage.ResetAndMove(std::move(*m_pCurrentScratchImage));
  if (m_ThumbnailOutputImage.Convert(ezImageFormat::R8G8B8A8_UNORM_SRGB).Failed())
  {
    ezLog::Error("Failed to convert thumbnail image to RGBA8.");
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult ezTexConvProcessor::GenerateLowResOutput()
{
  if (m_Descriptor.m_uiLowResMipmaps == 0)
    return EZ_SUCCESS;

  if (m_Descriptor.m_MipmapMode == ezTexConvMipmapMode::None)
  {
    ezLog::Error("LowRes data cannot be generated for images without mipmaps.");
    return EZ_FAILURE;
  }

  if (m_OutputImage.GetNumMipLevels() <= m_Descriptor.m_uiLowResMipmaps)
  {
    // probably just a low-resolution input image, do not generate output, but also do not fail
    ezLog::Warning("LowRes image not generated, original resolution is already below threshold.");
    return EZ_SUCCESS;
  }

  if (ezImageUtils::ExtractLowerMipChain(m_OutputImage, *m_pCurrentScratchImage, m_Descriptor.m_uiLowResMipmaps).Failed())
  {
    ezLog::Error("Failed to extract low-res mipmap chain from output image.");
    return EZ_FAILURE;
  }

  m_LowResOutputImage.ResetAndMove(std::move(*m_pCurrentScratchImage));

  return EZ_SUCCESS;
}
