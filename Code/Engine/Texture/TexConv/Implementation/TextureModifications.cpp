#include <TexturePCH.h>

#include <Texture/Image/ImageUtils.h>
#include <Texture/TexConv/TexConvProcessor.h>

ezResult ezTexConvProcessor::ForceSRGBFormats()
{
  // if the output is going to be sRGB, assume the incoming RGB data is also already in sRGB
  if (m_Descriptor.m_Usage == ezTexConvUsage::Color)
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

ezResult ezTexConvProcessor::GenerateMipmaps(ezImage& img, ezUInt32 uiNumMips) const
{
  ezImageUtils::MipMapOptions opt;
  opt.m_numMipMaps = uiNumMips;

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

  opt.m_addressModeU = m_Descriptor.m_AddressModeU;
  opt.m_addressModeV = m_Descriptor.m_AddressModeV;
  opt.m_addressModeW = m_Descriptor.m_AddressModeW;

  opt.m_preserveCoverage = m_Descriptor.m_bPreserveMipmapCoverage;
  opt.m_alphaThreshold = m_Descriptor.m_fMipmapAlphaThreshold;

  opt.m_renormalizeNormals =
    m_Descriptor.m_Usage == ezTexConvUsage::NormalMap || m_Descriptor.m_Usage == ezTexConvUsage::NormalMap_Inverted;

  ezImage scratch;
  ezImageUtils::GenerateMipMaps(img, scratch, opt);
  img.ResetAndMove(std::move(scratch));

  if (img.GetNumMipLevels() <= 1)
  {
    ezLog::Error("Mipmap generation failed.");
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult ezTexConvProcessor::PremultiplyAlpha(ezImage& image) const
{
  if (!m_Descriptor.m_bPremultiplyAlpha)
    return EZ_SUCCESS;

  for (ezColor& col : image.GetArrayPtr<ezColor>())
  {
    col.r *= col.a;
    col.g *= col.a;
    col.b *= col.a;
  }

  return EZ_SUCCESS;
}

ezResult ezTexConvProcessor::AdjustHdrExposure(ezImage& img) const
{
  ezImageUtils::ChangeExposure(img, m_Descriptor.m_fHdrExposureBias);
  return EZ_SUCCESS;
}



EZ_STATICLINK_FILE(Texture, Texture_TexConv_Implementation_TextureModifications);
