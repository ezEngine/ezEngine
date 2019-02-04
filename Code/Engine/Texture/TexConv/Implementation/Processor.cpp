#include <PCH.h>

#include <Texture/Image/ImageUtils.h>
#include <Texture/TexConv/TexConvProcessor.h>

// clang=format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezTexConvCompressionMode, 1)
  EZ_ENUM_CONSTANTS(ezTexConvCompressionMode::None, ezTexConvCompressionMode::Medium, ezTexConvCompressionMode::High)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezTexConvMipmapMode, 1)
  EZ_ENUM_CONSTANTS(ezTexConvMipmapMode::None, ezTexConvMipmapMode::Linear, ezTexConvMipmapMode::Kaiser)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezTexConvWrapMode, 1)
  EZ_ENUM_CONSTANTS(ezTexConvWrapMode::Repeat, ezTexConvWrapMode::Mirror, ezTexConvWrapMode::Clamp)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezTexConvFilterMode, 1)
  EZ_ENUM_CONSTANT(ezTexConvFilterMode::FixedNearest), EZ_ENUM_CONSTANT(ezTexConvFilterMode::FixedBilinear),
    EZ_ENUM_CONSTANT(ezTexConvFilterMode::FixedTrilinear), EZ_ENUM_CONSTANT(ezTexConvFilterMode::FixedAnisotropic2x),
    EZ_ENUM_CONSTANT(ezTexConvFilterMode::FixedAnisotropic4x), EZ_ENUM_CONSTANT(ezTexConvFilterMode::FixedAnisotropic8x),
    EZ_ENUM_CONSTANT(ezTexConvFilterMode::FixedAnisotropic16x), EZ_ENUM_CONSTANT(ezTexConvFilterMode::LowestQuality),
    EZ_ENUM_CONSTANT(ezTexConvFilterMode::LowQuality), EZ_ENUM_CONSTANT(ezTexConvFilterMode::DefaultQuality),
    EZ_ENUM_CONSTANT(ezTexConvFilterMode::HighQuality), EZ_ENUM_CONSTANT(ezTexConvFilterMode::HighestQuality),
EZ_END_STATIC_REFLECTED_ENUM;
// clang=format on

ezTexConvProcessor::ezTexConvProcessor()
{
  m_pCurrentScratchImage = &m_ScratchImage1;
  m_pOtherScratchImage = &m_ScratchImage2;
}

ezResult ezTexConvProcessor::Process()
{
  if (m_Descriptor.m_OutputType == ezTexConvOutputType::DecalAtlas)
  {
    EZ_SUCCEED_OR_RETURN(GenerateDecalAtlas());
  }
  else
  {
    EZ_SUCCEED_OR_RETURN(DetectNumChannels());

    EZ_SUCCEED_OR_RETURN(LoadInputImages());

    EZ_SUCCEED_OR_RETURN(AdjustTargetFormat());

    EZ_SUCCEED_OR_RETURN(ForceSRGBFormats());

    EZ_SUCCEED_OR_RETURN(ChooseOutputFormat());

    EZ_SUCCEED_OR_RETURN(DetermineTargetResolution());

    EZ_SUCCEED_OR_RETURN(ConvertInputImagesToFloat32());

    EZ_SUCCEED_OR_RETURN(ResizeInputImagesToSameDimensions());

    EZ_SUCCEED_OR_RETURN(Assemble2DTexture());

    EZ_SUCCEED_OR_RETURN(AdjustHdrExposure());

    EZ_SUCCEED_OR_RETURN(GenerateMipmaps());

    EZ_SUCCEED_OR_RETURN(PremultiplyAlpha());

    EZ_SUCCEED_OR_RETURN(GenerateOutput());

    EZ_SUCCEED_OR_RETURN(GenerateThumbnailOutput());

    EZ_SUCCEED_OR_RETURN(GenerateLowResOutput());
  }

  return EZ_SUCCESS;
}

ezResult ezTexConvProcessor::DetectNumChannels()
{
  m_uiNumChannels = 0;

  for (const auto& mapping : m_Descriptor.m_ChannelMappings)
  {
    for (ezUInt32 i = 0; i < 4; ++i)
    {
      if (mapping.m_Channel[i].m_iInputImageIndex != -1)
      {
        m_uiNumChannels = ezMath::Max(m_uiNumChannels, i + 1);
      }
    }
  }

  if (m_uiNumChannels == 0)
  {
    ezLog::Error("No proper channel mapping provided.");
    return EZ_FAILURE;
  }

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



EZ_STATICLINK_FILE(Texture, Texture_TexConv_Implementation_Processor);
