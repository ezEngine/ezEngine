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

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezTexConvUsage, 1)
  EZ_ENUM_CONSTANT(ezTexConvUsage::Auto), EZ_ENUM_CONSTANT(ezTexConvUsage::Color), EZ_ENUM_CONSTANT(ezTexConvUsage::Linear),
    EZ_ENUM_CONSTANT(ezTexConvUsage::Hdr), EZ_ENUM_CONSTANT(ezTexConvUsage::NormalMap),
    EZ_ENUM_CONSTANT(ezTexConvUsage::NormalMap_Inverted),
EZ_END_STATIC_REFLECTED_ENUM;
// clang=format on

ezTexConvProcessor::ezTexConvProcessor() = default;

ezResult ezTexConvProcessor::Process()
{
  if (m_Descriptor.m_OutputType == ezTexConvOutputType::DecalAtlas)
  {
    ezMemoryStreamWriter stream(&m_DecalAtlas);
    EZ_SUCCEED_OR_RETURN(GenerateDecalAtlas(stream));
  }
  else
  {
    ezUInt32 uiNumChannelsUsed = 0;
    EZ_SUCCEED_OR_RETURN(DetectNumChannels(m_Descriptor.m_ChannelMappings, uiNumChannelsUsed));

    EZ_SUCCEED_OR_RETURN(LoadInputImages());

    EZ_SUCCEED_OR_RETURN(AdjustUsage(m_Descriptor.m_InputFiles[0], m_Descriptor.m_InputImages[0], m_Descriptor.m_Usage));

    EZ_SUCCEED_OR_RETURN(ForceSRGBFormats());

    ezEnum<ezImageFormat> OutputImageFormat;

    EZ_SUCCEED_OR_RETURN(ChooseOutputFormat(OutputImageFormat, m_Descriptor.m_Usage, uiNumChannelsUsed));

    ezUInt32 uiTargetResolutionX = 0;
    ezUInt32 uiTargetResolutionY = 0;

    EZ_SUCCEED_OR_RETURN(
      DetermineTargetResolution(m_Descriptor.m_InputImages[0], OutputImageFormat, uiTargetResolutionX, uiTargetResolutionY));

    EZ_SUCCEED_OR_RETURN(ConvertAndScaleInputImages(uiTargetResolutionX, uiTargetResolutionY));

    ezImage assembledImg;
    if (m_Descriptor.m_OutputType == ezTexConvOutputType::Texture2D)
    {
      EZ_SUCCEED_OR_RETURN(Assemble2DTexture(m_Descriptor.m_InputImages[0].GetHeader(), assembledImg));
    }
    else if (m_Descriptor.m_OutputType == ezTexConvOutputType::TextureCube)
    {
      EZ_SUCCEED_OR_RETURN(AssembleCubemap(assembledImg));
    }

    EZ_SUCCEED_OR_RETURN(AdjustHdrExposure(assembledImg));

    EZ_SUCCEED_OR_RETURN(GenerateMipmaps(assembledImg));

    EZ_SUCCEED_OR_RETURN(PremultiplyAlpha(assembledImg));

    EZ_SUCCEED_OR_RETURN(GenerateOutput(std::move(assembledImg), m_OutputImage, OutputImageFormat));

    EZ_SUCCEED_OR_RETURN(GenerateThumbnailOutput(m_OutputImage, m_ThumbnailOutputImage, m_Descriptor.m_uiThumbnailOutputResolution));

    EZ_SUCCEED_OR_RETURN(GenerateLowResOutput(m_OutputImage, m_LowResOutputImage, m_Descriptor.m_uiLowResMipmaps));
  }

  return EZ_SUCCESS;
}

ezResult ezTexConvProcessor::DetectNumChannels(ezArrayPtr<const ezTexConvSliceChannelMapping> channelMapping, ezUInt32& uiNumChannels)
{
  uiNumChannels = 0;

  for (const auto& mapping : channelMapping)
  {
    for (ezUInt32 i = 0; i < 4; ++i)
    {
      if (mapping.m_Channel[i].m_iInputImageIndex != -1)
      {
        uiNumChannels = ezMath::Max(uiNumChannels, i + 1);
      }
    }
  }

  if (uiNumChannels == 0)
  {
    ezLog::Error("No proper channel mapping provided.");
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult ezTexConvProcessor::GenerateOutput(ezImage&& src, ezImage& dst, ezEnum<ezImageFormat> format)
{
  dst.ResetAndMove(std::move(src));

  if (dst.Convert(format).Failed())
  {
    ezLog::Error("Failed to convert result image to output format '{}'", ezImageFormat::GetName(format));
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult ezTexConvProcessor::GenerateThumbnailOutput(const ezImage& srcImg, ezImage& dstImg, ezUInt32 uiTargetRes)
{
  if (uiTargetRes == 0)
    return EZ_SUCCESS;

  ezUInt32 uiBestMip = 0;

  for (ezUInt32 m = 0; m < srcImg.GetNumMipLevels(); ++m)
  {
    if (srcImg.GetWidth(m) <= uiTargetRes && srcImg.GetHeight(m) <= uiTargetRes)
    {
      uiBestMip = m;
      break;
    }

    uiBestMip = m;
  }

  ezImage scratch1, scratch2;
  ezImage* pCurrentScratch = &scratch1;
  ezImage* pOtherScratch = &scratch2;

  pCurrentScratch->ResetAndCopy(srcImg.GetSubImageView(uiBestMip, 0));

  if (pCurrentScratch->GetWidth() > uiTargetRes || pCurrentScratch->GetHeight() > uiTargetRes)
  {
    if (pCurrentScratch->GetWidth() > pCurrentScratch->GetHeight())
    {
      const float fAspectRatio = (float)pCurrentScratch->GetWidth() / (float)uiTargetRes;
      ezUInt32 uiTargetHeight = (ezUInt32)(pCurrentScratch->GetHeight() / fAspectRatio);

      uiTargetHeight = ezMath::Max(uiTargetHeight, 4U);

      if (ezImageUtils::Scale(*pCurrentScratch, *pOtherScratch, uiTargetRes, uiTargetHeight).Failed())
      {
        ezLog::Error("Failed to resize thumbnail image from {}x{} to {}x{}", pCurrentScratch->GetWidth(), pCurrentScratch->GetHeight(),
          uiTargetRes, uiTargetHeight);
        return EZ_FAILURE;
      }
    }
    else
    {
      const float fAspectRatio = (float)pCurrentScratch->GetHeight() / (float)uiTargetRes;
      ezUInt32 uiTargetWidth = (ezUInt32)(pCurrentScratch->GetWidth() / fAspectRatio);

      uiTargetWidth = ezMath::Max(uiTargetWidth, 4U);

      if (ezImageUtils::Scale(*pCurrentScratch, *pOtherScratch, uiTargetWidth, uiTargetRes).Failed())
      {
        ezLog::Error("Failed to resize thumbnail image from {}x{} to {}x{}", pCurrentScratch->GetWidth(), pCurrentScratch->GetHeight(),
          uiTargetWidth, uiTargetRes);
        return EZ_FAILURE;
      }
    }

    ezMath::Swap(pCurrentScratch, pOtherScratch);
  }

  dstImg.ResetAndMove(std::move(*pCurrentScratch));

  // we want to write out the thumbnail unchanged, so make sure it has a non-sRGB format
  dstImg.ReinterpretAs(ezImageFormat::AsLinear(dstImg.GetImageFormat()));

  if (dstImg.Convert(ezImageFormat::R8G8B8A8_UNORM).Failed())
  {
    ezLog::Error("Failed to convert thumbnail image to RGBA8.");
    return EZ_FAILURE;
  }

  // generate alpha checkerboard pattern
  {
    const float fTileSize = 16.0f;

    ezColorLinearUB* pPixels = dstImg.GetPixelPointer<ezColorLinearUB>();
    const ezUInt32 rowPitch = dstImg.GetRowPitch();

    ezInt32 checkCounter = 0;
    ezColor tiles[2]{ezColor::LightGray, ezColor::DarkGray};


    for (ezUInt32 y = 0; y < dstImg.GetHeight(); ++y)
    {
      checkCounter = (ezInt32)ezMath::Floor(y / fTileSize);

      for (ezUInt32 x = 0; x < dstImg.GetWidth(); ++x)
      {
        ezColorLinearUB& col = pPixels[x];

        if (col.a < 255)
        {
          const ezColor colF = col;
          const ezInt32 tileIdx = (checkCounter + (ezInt32)ezMath::Floor(x / fTileSize)) % 2;

          col = ezMath::Lerp(tiles[tileIdx], colF, ezMath::Sqrt(colF.a)).WithAlpha(colF.a);
        }
      }

      pPixels = ezMemoryUtils::AddByteOffset(pPixels, rowPitch);
    }
  }

  return EZ_SUCCESS;
}

ezResult ezTexConvProcessor::GenerateLowResOutput(const ezImage& srcImg, ezImage& dstImg, ezUInt32 uiLowResMip)
{
  if (uiLowResMip == 0)
    return EZ_SUCCESS;

  if (srcImg.GetNumMipLevels() <= uiLowResMip)
  {
    // probably just a low-resolution input image, do not generate output, but also do not fail
    ezLog::Warning("LowRes image not generated, original resolution is already below threshold.");
    return EZ_SUCCESS;
  }

  if (ezImageUtils::ExtractLowerMipChain(srcImg, dstImg, uiLowResMip).Failed())
  {
    ezLog::Error("Failed to extract low-res mipmap chain from output image.");
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}



EZ_STATICLINK_FILE(Texture, Texture_TexConv_Implementation_Processor);
