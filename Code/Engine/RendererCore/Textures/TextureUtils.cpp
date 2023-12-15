#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Reflection/ReflectionUtils.h>
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

ezImageFormat::Enum ezTextureUtils::GalFormatToImageFormat(ezGALResourceFormat::Enum format)
{
  switch (format)
  {
    case ezGALResourceFormat::RGBAFloat:
      return ezImageFormat::R32G32B32A32_FLOAT;
    case ezGALResourceFormat::RGBAUInt:
      return ezImageFormat::R32G32B32A32_UINT;
    case ezGALResourceFormat::RGBAInt:
      return ezImageFormat::R32G32B32A32_SINT;
    case ezGALResourceFormat::RGBFloat:
      return ezImageFormat::R32G32B32_FLOAT;
    case ezGALResourceFormat::RGBUInt:
      return ezImageFormat::R32G32B32_UINT;
    case ezGALResourceFormat::RGBInt:
      return ezImageFormat::R32G32B32_SINT;
    case ezGALResourceFormat::B5G6R5UNormalized:
      return ezImageFormat::B5G6R5_UNORM;
    case ezGALResourceFormat::BGRAUByteNormalized:
      return ezImageFormat::B8G8R8A8_UNORM;
    case ezGALResourceFormat::BGRAUByteNormalizedsRGB:
      return ezImageFormat::B8G8R8A8_UNORM_SRGB;
    case ezGALResourceFormat::RGBAHalf:
      return ezImageFormat::R16G16B16A16_FLOAT;
    case ezGALResourceFormat::RGBAUShort:
      return ezImageFormat::R16G16B16A16_UINT;
    case ezGALResourceFormat::RGBAUShortNormalized:
      return ezImageFormat::R16G16B16A16_UNORM;
    case ezGALResourceFormat::RGBAShort:
      return ezImageFormat::R16G16B16A16_SINT;
    case ezGALResourceFormat::RGBAShortNormalized:
      return ezImageFormat::R16G16B16A16_SNORM;
    case ezGALResourceFormat::RGFloat:
      return ezImageFormat::R32G32_FLOAT;
    case ezGALResourceFormat::RGUInt:
      return ezImageFormat::R32G32_UINT;
    case ezGALResourceFormat::RGInt:
      return ezImageFormat::R32G32_SINT;
    case ezGALResourceFormat::RG11B10Float:
      return ezImageFormat::R11G11B10_FLOAT;
    case ezGALResourceFormat::RGBAUByteNormalized:
      return ezImageFormat::R8G8B8A8_UNORM;
    case ezGALResourceFormat::RGBAUByteNormalizedsRGB:
      return ezImageFormat::R8G8B8A8_UNORM_SRGB;
    case ezGALResourceFormat::RGBAUByte:
      return ezImageFormat::R8G8B8A8_UINT;
    case ezGALResourceFormat::RGBAByteNormalized:
      return ezImageFormat::R8G8B8A8_SNORM;
    case ezGALResourceFormat::RGBAByte:
      return ezImageFormat::R8G8B8A8_SINT;
    case ezGALResourceFormat::RGHalf:
      return ezImageFormat::R16G16_FLOAT;
    case ezGALResourceFormat::RGUShort:
      return ezImageFormat::R16G16_UINT;
    case ezGALResourceFormat::RGUShortNormalized:
      return ezImageFormat::R16G16_UNORM;
    case ezGALResourceFormat::RGShort:
      return ezImageFormat::R16G16_SINT;
    case ezGALResourceFormat::RGShortNormalized:
      return ezImageFormat::R16G16_SNORM;
    case ezGALResourceFormat::RGUByte:
      return ezImageFormat::R8G8_UINT;
    case ezGALResourceFormat::RGUByteNormalized:
      return ezImageFormat::R8G8_UNORM;
    case ezGALResourceFormat::RGByte:
      return ezImageFormat::R8G8_SINT;
    case ezGALResourceFormat::RGByteNormalized:
      return ezImageFormat::R8G8_SNORM;
    case ezGALResourceFormat::DFloat:
      return ezImageFormat::R32_FLOAT;
    case ezGALResourceFormat::RFloat:
      return ezImageFormat::R32_FLOAT;
    case ezGALResourceFormat::RUInt:
      return ezImageFormat::R32_UINT;
    case ezGALResourceFormat::RInt:
      return ezImageFormat::R32_SINT;
    case ezGALResourceFormat::RHalf:
      return ezImageFormat::R16_FLOAT;
    case ezGALResourceFormat::RUShort:
      return ezImageFormat::R16_UINT;
    case ezGALResourceFormat::RUShortNormalized:
      return ezImageFormat::R16_UNORM;
    case ezGALResourceFormat::RShort:
      return ezImageFormat::R16_SINT;
    case ezGALResourceFormat::RShortNormalized:
      return ezImageFormat::R16_SNORM;
    case ezGALResourceFormat::RUByte:
      return ezImageFormat::R8_UINT;
    case ezGALResourceFormat::RUByteNormalized:
      return ezImageFormat::R8_UNORM;
    case ezGALResourceFormat::RByte:
      return ezImageFormat::R8_SINT;
    case ezGALResourceFormat::RByteNormalized:
      return ezImageFormat::R8_SNORM;
    case ezGALResourceFormat::AUByteNormalized:
      return ezImageFormat::R8_UNORM;
    case ezGALResourceFormat::D16:
      return ezImageFormat::R16_UINT;
    case ezGALResourceFormat::BC1:
      return ezImageFormat::BC1_UNORM;
    case ezGALResourceFormat::BC1sRGB:
      return ezImageFormat::BC1_UNORM_SRGB;
    case ezGALResourceFormat::BC2:
      return ezImageFormat::BC2_UNORM;
    case ezGALResourceFormat::BC2sRGB:
      return ezImageFormat::BC2_UNORM_SRGB;
    case ezGALResourceFormat::BC3:
      return ezImageFormat::BC3_UNORM;
    case ezGALResourceFormat::BC3sRGB:
      return ezImageFormat::BC3_UNORM_SRGB;
    case ezGALResourceFormat::BC4UNormalized:
      return ezImageFormat::BC4_UNORM;
    case ezGALResourceFormat::BC4Normalized:
      return ezImageFormat::BC4_SNORM;
    case ezGALResourceFormat::BC5UNormalized:
      return ezImageFormat::BC5_UNORM;
    case ezGALResourceFormat::BC5Normalized:
      return ezImageFormat::BC5_SNORM;
    case ezGALResourceFormat::BC6UFloat:
      return ezImageFormat::BC6H_UF16;
    case ezGALResourceFormat::BC6Float:
      return ezImageFormat::BC6H_SF16;
    case ezGALResourceFormat::BC7UNormalized:
      return ezImageFormat::BC7_UNORM;
    case ezGALResourceFormat::BC7UNormalizedsRGB:
      return ezImageFormat::BC7_UNORM_SRGB;
    case ezGALResourceFormat::RGB10A2UInt:
    case ezGALResourceFormat::RGB10A2UIntNormalized:
    case ezGALResourceFormat::D24S8:
    default:
    {
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
      ezStringBuilder sFormat;
      EZ_ASSERT_DEBUG(ezReflectionUtils::EnumerationToString(ezGetStaticRTTI<ezGALResourceFormat>(), format, sFormat, ezReflectionUtils::EnumConversionMode::ValueNameOnly), "Cannot convert GAL format '{}' to string", format);
      EZ_ASSERT_DEBUG(false, "The GL format: '{}' does not have a matching image format.", sFormat);
#endif
    }
  }
  return ezImageFormat::UNKNOWN;
}

ezImageFormat::Enum ezTextureUtils::GalFormatToImageFormat(ezGALResourceFormat::Enum format, bool bRemoveSRGB)
{
  ezImageFormat::Enum imageFormat = GalFormatToImageFormat(format);
  if (bRemoveSRGB)
  {
    imageFormat = ezImageFormat::AsLinear(imageFormat);
  }
  return imageFormat;
}

void ezTextureUtils::ConfigureSampler(ezTextureFilterSetting::Enum filter, ezGALSamplerStateCreationDescription& out_sampler)
{
  const ezTextureFilterSetting::Enum thisFilter = ezRenderContext::GetDefaultInstance()->GetSpecificTextureFilter(filter);

  out_sampler.m_MinFilter = ezGALTextureFilterMode::Linear;
  out_sampler.m_MagFilter = ezGALTextureFilterMode::Linear;
  out_sampler.m_MipFilter = ezGALTextureFilterMode::Linear;
  out_sampler.m_uiMaxAnisotropy = 1;

  switch (thisFilter)
  {
    case ezTextureFilterSetting::FixedNearest:
      out_sampler.m_MinFilter = ezGALTextureFilterMode::Point;
      out_sampler.m_MagFilter = ezGALTextureFilterMode::Point;
      out_sampler.m_MipFilter = ezGALTextureFilterMode::Point;
      break;
    case ezTextureFilterSetting::FixedBilinear:
      out_sampler.m_MipFilter = ezGALTextureFilterMode::Point;
      break;
    case ezTextureFilterSetting::FixedTrilinear:
      break;
    case ezTextureFilterSetting::FixedAnisotropic2x:
      out_sampler.m_MinFilter = ezGALTextureFilterMode::Anisotropic;
      out_sampler.m_MagFilter = ezGALTextureFilterMode::Anisotropic;
      out_sampler.m_uiMaxAnisotropy = 2;
      break;
    case ezTextureFilterSetting::FixedAnisotropic4x:
      out_sampler.m_MinFilter = ezGALTextureFilterMode::Anisotropic;
      out_sampler.m_MagFilter = ezGALTextureFilterMode::Anisotropic;
      out_sampler.m_uiMaxAnisotropy = 4;
      break;
    case ezTextureFilterSetting::FixedAnisotropic8x:
      out_sampler.m_MinFilter = ezGALTextureFilterMode::Anisotropic;
      out_sampler.m_MagFilter = ezGALTextureFilterMode::Anisotropic;
      out_sampler.m_uiMaxAnisotropy = 8;
      break;
    case ezTextureFilterSetting::FixedAnisotropic16x:
      out_sampler.m_MinFilter = ezGALTextureFilterMode::Anisotropic;
      out_sampler.m_MagFilter = ezGALTextureFilterMode::Anisotropic;
      out_sampler.m_uiMaxAnisotropy = 16;
      break;
    default:
      break;
  }
}


