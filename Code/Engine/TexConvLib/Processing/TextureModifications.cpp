#include <PCH.h>

#include <TexConvLib/Processing/Processor.h>

ezResult ezTexConvProcessor::ForceSRGBFormats()
{
  // if the output is going to be sRGB, assume the incoming RGB data is also already in sRGB
  if (m_Descriptor.m_Usage == ezTexConvUsage::Color || m_Descriptor.m_Usage == ezTexConvUsage::Compressed_4_Channel_sRGB ||
      m_Descriptor.m_Usage == ezTexConvUsage::Uncompressed_8_Bit_UNorm_4_Channel_SRGB)
  {
    for (const auto& mapping : m_Descriptor.m_ChannelMappings)
    {
      // do not enforce sRGB conversion for textures that are mapped to the alpha channel
      for (ezUInt32 i = 0; i < 3; ++i)
      {
        const ezInt32 iTex = mapping.m_Channel[i].m_iInputImageIndex;
        if (iTex != -1)
        {
          auto& img = m_Descriptor.m_InputImages[iTex];
          img.ReinterpretAs(ezImageFormat::AsSrgb(img.GetImageFormat()));
        }
      }
    }
  }

  return EZ_SUCCESS;
}

ezResult ezTexConvProcessor::GenerateMipmaps()
{
  ezImageUtils::MipMapOptions opt;

  ezImageFilterBox filterLinear;
  ezImageFilterSincWithKaiserWindow filterKaiser;

  switch (m_Descriptor.m_MipmapMode)
  {
    case ezTexConvMipmapMode::None:
      return EZ_SUCCESS;

    case ezTexConvMipmapMode::Linear:
      opt.m_filter = &filterLinear;
      break;

    case ezTexConvMipmapMode::Kaiser:
      opt.m_filter = &filterKaiser;
      break;
  }

  switch (m_Descriptor.m_WrapModes[0])
  {
    case ezTexConvWrapMode::Repeat:
      opt.m_addressModeU = ezImageAddressMode::WRAP;
      break;
    case ezTexConvWrapMode::Clamp:
      opt.m_addressModeU = ezImageAddressMode::CLAMP;
      break;
    case ezTexConvWrapMode::Mirror:
      opt.m_addressModeU = ezImageAddressMode::MIRROR;
      break;
  }

  switch (m_Descriptor.m_WrapModes[1])
  {
    case ezTexConvWrapMode::Repeat:
      opt.m_addressModeV = ezImageAddressMode::WRAP;
      break;
    case ezTexConvWrapMode::Clamp:
      opt.m_addressModeV = ezImageAddressMode::CLAMP;
      break;
    case ezTexConvWrapMode::Mirror:
      opt.m_addressModeV = ezImageAddressMode::MIRROR;
      break;
  }

  switch (m_Descriptor.m_WrapModes[2])
  {
    case ezTexConvWrapMode::Repeat:
      opt.m_addressModeW = ezImageAddressMode::WRAP;
      break;
    case ezTexConvWrapMode::Clamp:
      opt.m_addressModeW = ezImageAddressMode::CLAMP;
      break;
    case ezTexConvWrapMode::Mirror:
      opt.m_addressModeW = ezImageAddressMode::MIRROR;
      break;
  }

  opt.m_preserveCoverage = m_Descriptor.m_bPreserveMipmapCoverage;
  opt.m_alphaThreshold = m_Descriptor.m_fMipmapAlphaThreshold;

  opt.m_renormalizeNormals =
    m_Descriptor.m_Usage == ezTexConvUsage::NormalMap || m_Descriptor.m_Usage == ezTexConvUsage::NormalMap_Inverted;

  ezImageUtils::GenerateMipMaps(*m_pCurrentScratchImage, *m_pOtherScratchImage, opt);
  ezMath::Swap(m_pCurrentScratchImage, m_pOtherScratchImage);

  if (m_pCurrentScratchImage->GetNumMipLevels() <= 1)
  {
    ezLog::Error("Mipmap generation failed.");
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult ezTexConvProcessor::PremultiplyAlpha()
{
  if (!m_Descriptor.m_bPremultiplyAlpha)
    return EZ_SUCCESS;

  for (ezUInt32 slice = 0; slice < m_pCurrentScratchImage->GetNumArrayIndices(); ++slice)
  {
    for (ezUInt32 face = 0; face < m_pCurrentScratchImage->GetNumFaces(); ++face)
    {
      for (ezUInt32 mip = 0; mip < m_pCurrentScratchImage->GetNumMipLevels(); ++mip)
      {
        ezColor* pPixel = m_pCurrentScratchImage->GetPixelPointer<ezColor>(mip, face, slice);

        for (ezUInt32 y = 0; y < m_pCurrentScratchImage->GetHeight(mip); ++y)
        {
          for (ezUInt32 x = 0; x < m_pCurrentScratchImage->GetWidth(mip); ++x)
          {
            pPixel[x].r *= pPixel[x].a;
            pPixel[x].g *= pPixel[x].a;
            pPixel[x].b *= pPixel[x].a;
          }

          pPixel = ezMemoryUtils::AddByteOffset(pPixel, m_pCurrentScratchImage->GetRowPitch(mip));
        }
      }
    }
  }

  return EZ_SUCCESS;
}

ezResult ezTexConvProcessor::AdjustHdrExposure()
{
  ezImageUtils::ChangeExposure(*m_pCurrentScratchImage, m_Descriptor.m_fHdrExposureBias);
  return EZ_SUCCESS;
}
