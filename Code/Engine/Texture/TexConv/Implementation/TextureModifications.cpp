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
    m_Descriptor.m_Usage == ezTexConvUsage::NormalMap || m_Descriptor.m_Usage == ezTexConvUsage::NormalMap_Inverted || m_Descriptor.m_Usage == ezTexConvUsage::BumpMap;

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

  for (ezColor& col : image.GetBlobPtr<ezColor>())
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

ezResult ezTexConvProcessor::ConvertToNormalMap(ezArrayPtr<ezImage> imgs) const
{
  for (ezImage& img : imgs)
  {
    EZ_SUCCEED_OR_RETURN(ConvertToNormalMap(img));
  }

  return EZ_SUCCESS;
}

ezResult ezTexConvProcessor::ConvertToNormalMap(ezImage& bumpMap) const
{
  ezImageHeader newImageHeader = bumpMap.GetHeader();
  newImageHeader.SetNumMipLevels(1);
  ezImage newImage;
  newImage.ResetAndAlloc(newImageHeader);

  struct Accum
  {
    float x = 0.f;
    float y = 0.f;
  };
  ezDelegate<Accum(ezUInt32, ezUInt32)> filterKernel;

  // we'll assume that both the input bump map and the new image are using
  // RGBA 32 bit floating point as an internal format which should be tightly packed
  EZ_ASSERT_DEV(bumpMap.GetImageFormat() == ezImageFormat::R32G32B32A32_FLOAT && bumpMap.GetRowPitch() % sizeof(ezColor) == 0, "");

  const ezColor* bumpPixels = bumpMap.GetPixelPointer<ezColor>(0, 0, 0, 0, 0, 0);
  const auto getBumpPixel = [&](ezUInt32 x, ezUInt32 y) -> float
  {
    const ezColor* ptr = bumpPixels + y * bumpMap.GetWidth() + x;
    return ptr->r;
  };

  ezColor* newPixels = newImage.GetPixelPointer<ezColor>(0, 0, 0, 0, 0, 0);
  auto getNewPixel = [&](ezUInt32 x, ezUInt32 y) -> ezColor&
  {
    ezColor* ptr = newPixels + y * newImage.GetWidth() + x;
    return *ptr;
  };

  switch (m_Descriptor.m_BumpMapFilter)
  {
  case ezTexConvBumpMapFilter::Finite:
    filterKernel = [&](ezUInt32 x, ezUInt32 y)
    {
      constexpr float linearKernel[3] = {-1, 0, 1};

      Accum accum;
      for (int i = -1; i <= 1; ++i)
      {
        const ezInt32 rx = ezMath::Clamp(i + static_cast<ezInt32>(x), 0, static_cast<ezInt32>(newImage.GetWidth()) - 1);
        const ezInt32 ry = ezMath::Clamp(i + static_cast<ezInt32>(y), 0, static_cast<ezInt32>(newImage.GetHeight()) - 1);

        const float depthX = getBumpPixel(rx, y);
        const float depthY = getBumpPixel(x, ry);

        accum.x += depthX * linearKernel[i + 1];
        accum.y += depthY * linearKernel[i + 1];
      }

      return accum;
    };
    break;
  case ezTexConvBumpMapFilter::Sobel:
    filterKernel = [&](ezUInt32 x, ezUInt32 y)
    {
      constexpr float kernel[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
      constexpr float weight = 1.f / 4.f;

      Accum accum;
      for (ezInt32 i = -1; i <= 1; ++i)
      {
        for (ezInt32 j = -1; j <= 1; ++j)
        {
          const ezInt32 rx = ezMath::Clamp(j + static_cast<ezInt32>(x), 0, static_cast<ezInt32>(newImage.GetWidth()) - 1);
          const ezInt32 ry = ezMath::Clamp(i + static_cast<ezInt32>(y), 0, static_cast<ezInt32>(newImage.GetHeight()) - 1);
      
          const float depth = getBumpPixel(rx, ry);
      
          accum.x += depth * kernel[i + 1][j + 1];
          accum.y += depth * kernel[j + 1][i + 1];
        }
      }

      accum.x *= weight;
      accum.y *= weight;

      return accum;
    };
    break;
  case ezTexConvBumpMapFilter::Scharr:
    filterKernel = [&](ezUInt32 x, ezUInt32 y)
    {
      constexpr float kernel[3][3] = {{-3, 0, 3}, {-10, 0, 10}, {-3, 0, 3}};
      constexpr float weight = 1.f / 16.f;

      Accum accum;
      for (ezInt32 i = -1; i <= 1; ++i)
      {
        for (ezInt32 j = -1; j <= 1; ++j)
        {
          const ezInt32 rx = ezMath::Clamp(j + static_cast<ezInt32>(x), 0, static_cast<ezInt32>(newImage.GetWidth()) - 1);
          const ezInt32 ry = ezMath::Clamp(i + static_cast<ezInt32>(y), 0, static_cast<ezInt32>(newImage.GetHeight()) - 1);

          const float depth = getBumpPixel(rx, ry);

          accum.x += depth * kernel[i + 1][j + 1];
          accum.y += depth * kernel[j + 1][i + 1];
        }
      }

      accum.x *= weight;
      accum.y *= weight;

      return accum;
    };
    break;
  };

  for (ezUInt32 y = 0; y < bumpMap.GetHeight(); ++y)
  {
    for (ezUInt32 x = 0; x < bumpMap.GetWidth(); ++x)
    {
      Accum accum = filterKernel(x, y);
      
      ezVec3 normal = ezVec3(1.f, 0.f, accum.x).CrossRH(ezVec3(0.f, 1.f, accum.y));
      normal.NormalizeIfNotZero(ezVec3(0, 0, 1), 0.001f);
      normal.y = -normal.y;

      normal = normal * 0.5f + ezVec3(0.5f);

      ezColor& newPixel = getNewPixel(x, y);
      newPixel.SetRGBA(normal.x, normal.y, normal.z, 0.f);
    }
  }

  bumpMap.ResetAndMove(std::move(newImage));

  return EZ_SUCCESS;
}

ezResult ezTexConvProcessor::ClampInputValues(ezArrayPtr<ezImage> images) const
{
  for (ezImage& image : images)
  {
    EZ_SUCCEED_OR_RETURN(ClampInputValues(image));
  }

  return EZ_SUCCESS;
}

ezResult ezTexConvProcessor::ClampInputValues(ezImage& image) const
{
  // we'll assume that at this point in the processing pipeline, the format is
  // RGBA32F which should result in tightly packed mipmaps.
  EZ_ASSERT_DEV(image.GetImageFormat() == ezImageFormat::R32G32B32A32_FLOAT && image.GetRowPitch() % sizeof(float[4]) == 0, "");

  for (auto& value : image.GetBlobPtr<float>())
  {
    constexpr float maxVal = 64000.f;
    if (ezMath::IsNaN(value))
    {
      value = 0.f;
    }
    else
    {
      value = ezMath::Clamp(value, -maxVal, maxVal);
    }
  }

  return EZ_SUCCESS;
}

EZ_STATICLINK_FILE(Texture, Texture_TexConv_Implementation_TextureModifications);
