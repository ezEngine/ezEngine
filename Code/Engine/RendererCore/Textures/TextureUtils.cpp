#include <RendererCorePCH.h>

#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Textures/TextureUtils.h>

bool ezTextureUtils::s_bForceFullQualityAlways = false;

ezGALResourceFormat::Enum ezTextureUtils::ImageFormatToGalFormat(ezImageFormat::Enum format, bool bSRGB)
{
  switch (format)
  {
    case ezImageFormat::R8G8B8A8_UNORM:
      if (bSRGB)
        return ezGALResourceFormat::RGBAUByteNormalizedsRGB;
      else
        return ezGALResourceFormat::RGBAUByteNormalized;

      // case ezImageFormat::R8G8B8A8_TYPELESS:
    case ezImageFormat::R8G8B8A8_UNORM_SRGB:
      return ezGALResourceFormat::RGBAUByteNormalizedsRGB;

    case ezImageFormat::R8G8B8A8_UINT:
      return ezGALResourceFormat::RGBAUInt;

    case ezImageFormat::R8G8B8A8_SNORM:
      return ezGALResourceFormat::RGBAByteNormalized;

    case ezImageFormat::R8G8B8A8_SINT:
      return ezGALResourceFormat::RGBAInt;

    case ezImageFormat::B8G8R8A8_UNORM:
      if (bSRGB)
        return ezGALResourceFormat::BGRAUByteNormalizedsRGB;
      else
        return ezGALResourceFormat::BGRAUByteNormalized;

    case ezImageFormat::B8G8R8X8_UNORM:
      if (bSRGB)
        return ezGALResourceFormat::BGRAUByteNormalizedsRGB;
      else
        return ezGALResourceFormat::BGRAUByteNormalized;

      // case ezImageFormat::B8G8R8A8_TYPELESS:
    case ezImageFormat::B8G8R8A8_UNORM_SRGB:
      return ezGALResourceFormat::BGRAUByteNormalizedsRGB;

      // case ezImageFormat::B8G8R8X8_TYPELESS:
    case ezImageFormat::B8G8R8X8_UNORM_SRGB:
      return ezGALResourceFormat::BGRAUByteNormalizedsRGB;

      // case ezImageFormat::B8G8R8_UNORM:

      // case ezImageFormat::BC1_TYPELESS:
    case ezImageFormat::BC1_UNORM:
      if (bSRGB)
        return ezGALResourceFormat::BC1sRGB;
      else
        return ezGALResourceFormat::BC1;

    case ezImageFormat::BC1_UNORM_SRGB:
      return ezGALResourceFormat::BC1sRGB;

      // case ezImageFormat::BC2_TYPELESS:
    case ezImageFormat::BC2_UNORM:
      if (bSRGB)
        return ezGALResourceFormat::BC2sRGB;
      else
        return ezGALResourceFormat::BC2;

    case ezImageFormat::BC2_UNORM_SRGB:
      return ezGALResourceFormat::BC2sRGB;

      // case ezImageFormat::BC3_TYPELESS:
    case ezImageFormat::BC3_UNORM:
      if (bSRGB)
        return ezGALResourceFormat::BC3sRGB;
      else
        return ezGALResourceFormat::BC3;

    case ezImageFormat::BC3_UNORM_SRGB:
      return ezGALResourceFormat::BC3sRGB;

      // case ezImageFormat::BC4_TYPELESS:
    case ezImageFormat::BC4_UNORM:
      return ezGALResourceFormat::BC4UNormalized;

    case ezImageFormat::BC4_SNORM:
      return ezGALResourceFormat::BC4Normalized;

      // case ezImageFormat::BC5_TYPELESS:
    case ezImageFormat::BC5_UNORM:
      return ezGALResourceFormat::BC5UNormalized;

    case ezImageFormat::BC5_SNORM:
      return ezGALResourceFormat::BC5Normalized;

      // case ezImageFormat::BC6H_TYPELESS:
    case ezImageFormat::BC6H_UF16:
      return ezGALResourceFormat::BC6UFloat;

    case ezImageFormat::BC6H_SF16:
      return ezGALResourceFormat::BC6Float;

      // case ezImageFormat::BC7_TYPELESS:
    case ezImageFormat::BC7_UNORM:
      if (bSRGB)
        return ezGALResourceFormat::BC7UNormalizedsRGB;
      else
        return ezGALResourceFormat::BC7UNormalized;

    case ezImageFormat::BC7_UNORM_SRGB:
      return ezGALResourceFormat::BC7UNormalizedsRGB;

    case ezImageFormat::B5G6R5_UNORM:
      return ezGALResourceFormat::B5G6R5UNormalized; /// \todo Not supported by some GPUs ?

    case ezImageFormat::R16_FLOAT:
      return ezGALResourceFormat::RHalf;

    case ezImageFormat::R32_FLOAT:
      return ezGALResourceFormat::RFloat;

    case ezImageFormat::R16G16_FLOAT:
      return ezGALResourceFormat::RGHalf;

    case ezImageFormat::R32G32_FLOAT:
      return ezGALResourceFormat::RGFloat;

    case ezImageFormat::R32G32B32_FLOAT:
      return ezGALResourceFormat::RGBFloat;

    case ezImageFormat::R16G16B16A16_FLOAT:
      return ezGALResourceFormat::RGBAHalf;

    case ezImageFormat::R32G32B32A32_FLOAT:
      return ezGALResourceFormat::RGBAFloat;

    case ezImageFormat::R16G16B16A16_UNORM:
      return ezGALResourceFormat::RGBAUShortNormalized;

    case ezImageFormat::R8_UNORM:
      return ezGALResourceFormat::RUByteNormalized;

    case ezImageFormat::R8G8_UNORM:
      return ezGALResourceFormat::RGUByteNormalized;

    case ezImageFormat::R16G16_UNORM:
      return ezGALResourceFormat::RGUShortNormalized;

    case ezImageFormat::R11G11B10_FLOAT:
      return ezGALResourceFormat::RG11B10Float;

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
      break;
  }

  return ezGALResourceFormat::Invalid;
}

void ezTextureUtils::ConfigureSampler(ezTextureFilterSetting::Enum filter, ezGALSamplerStateCreationDescription& out_Sampler)
{
  const ezTextureFilterSetting::Enum thisFilter = ezRenderContext::GetDefaultInstance()->GetSpecificTextureFilter(filter);

  out_Sampler.m_MinFilter = ezGALTextureFilterMode::Linear;
  out_Sampler.m_MagFilter = ezGALTextureFilterMode::Linear;
  out_Sampler.m_MipFilter = ezGALTextureFilterMode::Linear;
  out_Sampler.m_uiMaxAnisotropy = 1;

  switch (thisFilter)
  {
    case ezTextureFilterSetting::FixedNearest:
      out_Sampler.m_MinFilter = ezGALTextureFilterMode::Point;
      out_Sampler.m_MagFilter = ezGALTextureFilterMode::Point;
      out_Sampler.m_MipFilter = ezGALTextureFilterMode::Point;
      break;
    case ezTextureFilterSetting::FixedBilinear:
      out_Sampler.m_MipFilter = ezGALTextureFilterMode::Point;
      break;
    case ezTextureFilterSetting::FixedTrilinear:
      break;
    case ezTextureFilterSetting::FixedAnisotropic2x:
      out_Sampler.m_MinFilter = ezGALTextureFilterMode::Anisotropic;
      out_Sampler.m_MagFilter = ezGALTextureFilterMode::Anisotropic;
      out_Sampler.m_uiMaxAnisotropy = 2;
      break;
    case ezTextureFilterSetting::FixedAnisotropic4x:
      out_Sampler.m_MinFilter = ezGALTextureFilterMode::Anisotropic;
      out_Sampler.m_MagFilter = ezGALTextureFilterMode::Anisotropic;
      out_Sampler.m_uiMaxAnisotropy = 4;
      break;
    case ezTextureFilterSetting::FixedAnisotropic8x:
      out_Sampler.m_MinFilter = ezGALTextureFilterMode::Anisotropic;
      out_Sampler.m_MagFilter = ezGALTextureFilterMode::Anisotropic;
      out_Sampler.m_uiMaxAnisotropy = 8;
      break;
    case ezTextureFilterSetting::FixedAnisotropic16x:
      out_Sampler.m_MinFilter = ezGALTextureFilterMode::Anisotropic;
      out_Sampler.m_MagFilter = ezGALTextureFilterMode::Anisotropic;
      out_Sampler.m_uiMaxAnisotropy = 16;
      break;
    default:
      break;
  }
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Textures_TextureUtils);

